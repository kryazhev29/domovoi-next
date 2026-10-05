"""Final IAR RAM ownership, real allocator and cached config reads. No hardware."""
import hashlib,json,struct,zlib
from storage_machine import *

LEGACY=0x1cd400;SLOT0=0x1f5000;SLOT1=0x1f7000

def free_blocks(m):
    p=int.from_bytes(m.u.mem_read(m.addr('Aldata')+4,4),'little');out=[];seen=set()
    while p:
        assert p not in seen;seen.add(p)
        n,pnext=struct.unpack('<II',m.u.mem_read(p,8))
        assert m.addr('HEAP$$Base')<=p<p+n<=m.addr('HEAP$$Limit') and n%8==0
        out.append(n);p=pnext
    return out

def heap_stats(m):
    b=free_blocks(m);total=sum(b);largest=max(b,default=0)
    if largest:
        p=m.checked('__basic_malloc',largest-8);assert p
        m.checked('__basic_free',p)
        assert not m.checked('__basic_malloc',largest-7)
    return {'free_chunk_bytes':total,'largest_chunk':largest,
            'largest_request':max(0,largest-8),'blocks':b,
            'fragmentation':0 if not total else 1-largest/total}

def allocator():
    m=StorageMachine();live=[];chunks=[]
    for n in (180,1040,534,2016):
        p=m.checked('__basic_malloc',n);assert p;live.append(p)
        chunks.append(int.from_bytes(m.u.mem_read(p-8,4),'little'))
    peak=heap_stats(m);assert sum(chunks)==3808 and peak['largest_request']==280
    # Reject before any destructive table operation, under actual fragmentation.
    for i in (0,2):m.checked('__basic_free',live[i])
    fragmented=heap_stats(m)
    for i in (1,3):m.checked('__basic_free',live[i])
    recovered=heap_stats(m);assert recovered['largest_request']==4088
    return {'scope':'Synthetic allocator pressure, not a proved production lifetime','live_chunks':chunks,'peak':peak,'fragmented':fragmented,'coalesced':recovered}

def config_machine():
    m=StorageMachine();old=bytes((i*7+13)&255 for i in range(4096))
    generation=struct.pack('<I',17)
    crc=zlib.crc32(generation+old)^0xffffffff
    m.flash[SLOT0:SLOT0+4096]=old
    m.flash[SLOT0+4096:SLOT0+4112]=struct.pack('<IIII',0x32474643,17,crc,crc^0xffffffff)
    m.flash[LEGACY:LEGACY+4096]=old
    assert m.checked('cfg_select',0x21000f00,budget=1000000)==SLOT0
    m.write(m.addr('cfg_legacy_checked'),1,1)
    data=bytes((i*11+91)&255 for i in range(1024))
    m.u.mem_write(0x21000000,data)
    return m,old,data

def run_config(m):
    costs=[];reads=[]
    for _ in range(2000):
        if m.checked('mem_config_result')!=1:break
        m.now+=1;m.checked('mem_config_poll',m.now)
        costs.append(m.steps);reads.append(m.read_budget)
        assert m.io_budget<=1 and m.read_budget<=192
        # Shared scratch cannot be granted to a direct table or camera entrant.
        assert not m.checked('sj_runtime_begin',0x41000,0x08010001,0)
        assert not m.checked('user_storage_command_ready')
    else:raise AssertionError('config did not terminate')
    return {'passes':len(costs),'maximum_instructions':max(costs),'total_instructions':sum(costs),
            'maximum_read_bytes':max(reads),'stack':m.stack_peak}

def config_ownership():
    m,old,new=config_machine();m.record_changes=True
    assert m.checked('mem_config_begin',0x21000000,0x21000200)
    result=run_config(m);assert m.checked('mem_config_result')==2
    assert m.flash[SLOT1:SLOT1+4096]==new+old[1024:]
    assert m.io is None and m.checked('mem_config_snapshot')
    # The snapshot remains owned through terminal result, until explicit reset.
    assert not m.checked('sj_runtime_begin',0x41000,0x08010001,0)
    m.checked('mem_config_reset');assert not m.checked('mem_config_snapshot')
    assert heap_stats(m)['largest_request']==4088
    from test_table_runtime import rows
    m.command(13,rows([1]),1);assert m.active()
    page=bytes(m.u.mem_read(m.addr('sjr')+61,64));before=bytes(m.flash)
    assert not m.checked('mem_config_begin',0x21000000,0x21000200)
    assert page==bytes(m.u.mem_read(m.addr('sjr')+61,64)) and before==bytes(m.flash)
    m.drain();assert m.timestamps and not m.io
    m.u.mem_write(0x21000000,new)
    assert m.checked('mem_config_begin',0x21000000,0x21000200)
    run_config(m);assert m.checked('mem_config_result')==2
    m.checked('mem_config_reset')
    m.write(m.addr('sjr')+59,1,1)
    assert not m.checked('mem_config_begin',0x21000000,0x21000200)
    # An active camera also owns the page across all storage polls.
    from test_photo_runtime import PhotoMachine
    p=PhotoMachine();p.boot_media();p.begin_camera(58,drain=False)
    assert p.checked('photo_storage_busy')
    assert not p.checked('mem_config_begin',0x21000000,0x21000200)
    return result

def config_faults():
    cases=0
    for kind,points in [('read',(1,64,150,300)),('start',(1,2,10,65)),('poll',(1,10,128))]:
        for point in points:
            m,old,new=config_machine();m.counts={'read':0,'start':0,'poll':0};m.fault=(kind,point)
            assert m.checked('mem_config_begin',0x21000000,0x21000200)
            run_config(m)
            if m.counts[kind]>=point:
                assert m.checked('mem_config_result')==3 and m.io is None
                cases+=1
            m.checked('mem_config_reset');assert heap_stats(m)['largest_request']==4088
    m,_,_=config_machine();p=m.checked('__basic_malloc',3500);assert p
    before=bytes(m.flash);counts=dict(m.counts)
    assert not m.checked('mem_config_begin',0x21000000,0x21000200)
    assert bytes(m.flash)==before and m.counts==counts
    return cases+1

def cached_reads():
    out=[]
    for active in (LEGACY,SLOT0,SLOT1):
        for result in (0,1,2,3):
            m=StorageMachine();m.write(m.addr('cfg_cached_active'),active)
            m.write(m.addr('cfg_job')+35,result,1)
            expected=bytes((i+result)&255 for i in range(64));m.flash[active+128:active+192]=expected
            # Neither full slot validation nor migration is legal at runtime.
            def forbidden():raise AssertionError('runtime config rescan')
            m.stubs[m.addr('cfg_select')]=forbidden;m.stubs[m.addr('cfg_prepare')]=forbidden
            assert m.checked('mem_read',LEGACY+128,0x21000000,64)
            assert bytes(m.u.mem_read(0x21000000,64))==expected and m.read_budget==64
            out.append({'active':active,'result':result,'instructions':m.steps,'stack':m.stack_peak})
            m.fault=('read',2);assert not m.checked('mem_read',LEGACY+128,0x21000000,64)
    return out

def power_cuts():
    m,old,new=config_machine();initial=bytes(m.flash);m.record_changes=True
    assert m.checked('mem_config_begin',0x21000000,0x21000200);run_config(m)
    state=bytearray(initial);count=0
    for kind,a,data in m.changes:
        for n in sorted(set((0,1,len(data)//2,len(data)-1,len(data)))):
            cut=bytearray(state)
            for i,v in enumerate(data[:n]):cut[a+i]=v if kind=='erase' else cut[a+i]&v
            r=StorageMachine(cut);chosen=r.checked('cfg_select',0x21000f00,budget=1000000)
            assert bytes(r.flash[chosen:chosen+4096]) in (old,new+old[1024:])
            count+=1
        if kind=='erase':state[a:a+4096]=b'\xff'*4096
        else:
            for i,v in enumerate(data):state[a+i]&=v
    return count

if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser();parser.add_argument('--power',action='store_true');args=parser.parse_args()
    out={'allocator':allocator(),'cached_reads':cached_reads()}
    print('allocator/cached bounded reads PASS',flush=True)
    out['config']=config_ownership();out['faults']=config_faults()
    print('shared scratch/ownership/real heap/I/O/OOM PASS',flush=True)
    if args.power:out['config_power_cuts']=power_cuts();print('config cuts',out['config_power_cuts'],'PASS',flush=True)
    out['elf_sha256']=hashlib.sha256((ROOT/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest()
    m=StorageMachine();out['static_gap']=m.addr('fault_record')-m.addr('HEAP$$Limit')
    out['heap_bytes']=m.heap_bytes;out['stack_bytes']=m.top-m.bottom
    assert m.map_size('sjr')==128 and m.map_size('cfg_job')==40
    (ROOT/'Tests/asm/memory_runtime_results.json').write_text(json.dumps(out,indent=2)+'\n')
