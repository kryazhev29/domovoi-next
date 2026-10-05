from test_work_memory import take, drop, owned, PHOTO, PHOTO_PACKET, REPLY, CONFIG, FW_CONTEXT, FW_DATA, TABLE, UP_CONTEXT, UP_PACKET, SIZES
"""Commands 13/14/15/16 -> main-loop -> real ASM journal, no hardware."""
import argparse,json,struct,hashlib,random
from storage_machine import *

def rows(indices,size=32):
    out=bytearray()
    for i,index in enumerate(indices):
        b=bytearray((x+i*7)&255 for x in range(size));b[0]=index&255
        if size==32:b[1]=index>>8
        b[12]=1;out+=b
    return bytes(out)

def test_normal(cmd=13,indices=(1,),repeat=False):
    m=StorageMachine();size=16 if cmd==15 else 32;data=rows(indices,size)
    mb=2 if cmd==13 else 4 if cmd==14 else 3 if cmd==15 else 5
    base={2:0x41000,4:0x1ca800,3:0x1c9000,5:0x1cc000}[mb]
    if cmd==16:data=b'\x01\x02\x03\x04'*2;indices=(1,2)
    before=bytes(m.flash);m.command(cmd,data,len(indices));assert m.active() and not m.answers and not m.timestamps
    assert bytes(m.flash)==before,'begin destroyed flash'
    m.drain();assert not m.answers and len(m.timestamps)==1,(m.answers,m.timestamps)
    expected=bytearray(before[base:base+(8192 if cmd==13 else 1024 if cmd==16 else 2048)])
    if cmd==16:expected[:1024]=data+b'\xff'*(1024-len(data))
    else:
        for i,index in enumerate(indices):
            b=bytearray(data[i*size:(i+1)*size])
            if cmd==13:b[26:28]=b'\0\0' # captured day with no valid clock
            expected[(index-1)*size:index*size]=b
    assert m.flash[base:base+len(expected)]==expected,(cmd,'image')
    assert m.timestamps[0][2]==crc16(expected),(cmd,'crc')
    assert m.flash[0x1fa000:0x1fb000]==b'\xff'*4096
    result={'max_instructions':max(m.costs),'total_instructions':sum(m.costs),'passes':len(m.costs),
            'stack':m.stack_peak,'begin_instructions':m.costs[0],'max_poll_instructions':max(m.costs[1:])}
    if repeat:
        m.record_changes=True;m.changes=[];m.command(cmd,data,len(indices));m.drain()
        if len(set(indices))==len(indices):assert not m.changes,'identical retry wears journal'
        assert m.flash[base:base+len(expected)]==expected,'retry changed image'
    return result

def test_cancel_and_reconnect():
    results=0
    for stage in (1,2,3,5,6,8,9,10,12,14,15):
        m=StorageMachine();data=rows([1,129]);m.command(13,data,2,1)
        sj=fields('sj_runtime_state_s');at=m.addr('sjr')
        for _ in range(3000):
            if m.u.mem_read(at+sj['phase'],1)[0]==stage:break
            m.tick()
        else:raise AssertionError(stage)
        m.checked('table_runtime_cancel');m.drain()
        assert not m.timestamps and not m.answers and m.io is None
        assert m.flash[0x42000:0x42020]==b'\xff'*32,'cancel applied another sector'
        if stage<=8:assert m.flash[0x41000:0x41020]==b'\xff'*32
        else:assert m.flash[0x41000:0x41002]==b'\1\0'
        results+=1
    m=StorageMachine();m.command(13,rows([1]),1,1);m.checked('user_comm_session',0);m.drain()
    assert not m.timestamps and m.flash[0x41000:0x41002]==b'\1\0';results+=1
    m=StorageMachine();m.command(13,rows([1]),1);m.write(m.addr('usb_bus_resets'),1);m.drain()
    assert not m.timestamps;results+=1
    return results

def test_faults():
    count=0
    for kind,points in [('start',(1,2,3,66,67,68,69,70,133,134)),
                        ('read',(1,64,129,190,256,257,320,400)),('poll',(1,6,9,200,204,207,210,390))]:
        for point in points:
            m=StorageMachine();m.fault=(kind,point);m.command(13,rows([1]),1);m.drain()
            if m.counts[kind]<point:continue
            assert not m.timestamps and m.answers[-1][1]==2,(kind,point,m.timestamps,m.answers)
            assert m.io is None
            count+=1
    m=StorageMachine();m.stubs[m.addr('work_acquire')]=lambda:m.ret(0)
    m.command(13,rows([1]),1);assert not m.active() and m.answers[-1][1]==2 and not any(m.counts.values())
    return count+1

def test_corrupt():
    m=StorageMachine();m.command(13,rows([1]),1)
    offset=fields('sj_runtime_state_s')['phase'];at=m.addr('sjr')
    for _ in range(2000):
        if m.u.mem_read(at+offset,1)[0]==5:break
        m.tick()
    else:raise AssertionError('backup CRC phase missing')
    m.flash[0x1f9000]^=1;m.drain()
    assert not m.timestamps and m.answers[-1][1]==2
    assert m.flash[0x41000:0x42000]==b'\xff'*4096
    return 1

def test_interaction_and_heap():
    m=StorageMachine();live=[(PHOTO,take(m,PHOTO))]
    assert all(p for slot,p in live)
    m.command(13,rows(range(1,64)),63);assert m.active()
    assert m.checked('user_storage_command_ready')==0 and m.checked('user_storage_flash_ready')==0
    assert m.checked('mem_tables_ready')==0
    m.command(19,b'',0);assert m.answers[-1][1]==2 and m.active()
    m.checked('user_savereset');m.drain();assert not m.timestamps
    assert m.u.mem_read(m.addr('store_reset_requested'),1)==b'\1'
    for slot,p in live:drop(m,slot,p)
    m=StorageMachine();m.write(m.addr('mem')+2*8+4,1601536)
    m.command(13,rows([1,50048]),2);m.drain(30000)
    assert len(m.timestamps)==1 and m.flash[0x1c7fe0:0x1c7fe2]==struct.pack('<H',50048)
    assert m.timestamps[0][2]==crc16(m.flash[0x41000:0x1c8000])
    return {'full_id_passes':len(m.costs),'full_id_max_poll':max(m.costs[1:]),
            'full_id_total_instructions':sum(m.costs),'full_id_stack':m.stack_peak,
            'simultaneous_workspace_payload':180+2016}

def test_power():
    m=StorageMachine();m.record_changes=True;initial=bytes(m.flash);m.command(13,rows([1]),1);m.drain()
    final=bytes(m.flash[0x41000:0x42000]);flash=bytearray(initial);count=0
    for kind,a,b in m.changes:
        for n in sorted(set((0,1,len(b)//2,len(b)-1,len(b)))):
            cut=bytearray(flash)
            for i,x in enumerate(b[:n]):cut[a+i]=x if kind=='erase' else cut[a+i]&x
            r=StorageMachine(cut);r.limit_reads=1000000
            assert r.checked('sj_recover',budget=2000000),(kind,hex(a),n)
            assert bytes(r.flash[0x41000:0x42000]) in (initial[0x41000:0x42000],final)
            count+=1
        if kind=='erase':flash[a:a+4096]=b'\xff'*4096
        else:
            for i,x in enumerate(b):flash[a+i]&=x
    return count

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--power',action='store_true');a=p.parse_args()
    out={'units':'Unicorn instructions, modeled NOR, NOT cycles or milliseconds','normal':{}}
    for cmd,idx in [(13,[1]),(13,list(range(1,64))),(13,[8,9,128,129,1,129]),(14,[8,9,64]),(15,[16,17,128]),(16,[1,2])]:
        out['normal'][str(cmd)+'/'+str(len(idx))]=test_normal(cmd,idx,True);print('normal',cmd,len(idx),'PASS',flush=True)
    out['cancel_reconnect']=test_cancel_and_reconnect();print('cancel/reconnect PASS',flush=True)
    out['faults']=test_faults();print('faults/heap PASS',flush=True)
    out['corruption']=test_corrupt();print('backup corruption PASS',flush=True)
    out['interaction_heap']=test_interaction_and_heap();print('full ID/ownership/reset/real heap PASS',flush=True)
    if a.power:out['power_cuts']=test_power();print('power',out['power_cuts'],'PASS',flush=True)
    else:
        previous=ROOT/'Tests/asm/table_runtime_results.json'
        if previous.exists():
            old=json.loads(previous.read_text())
            if old.get('power_cuts'):out['power_cuts']=old['power_cuts'];out['power_elf_sha256']=old.get('power_elf_sha256',old.get('elf_sha256'))
    out['elf_sha256']=hashlib.sha256((ROOT/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest()
    (ROOT/'Tests/asm/table_runtime_results.json').write_text(json.dumps(out,indent=2)+'\n')
    print('PASS table runtime: actual callers, ownership, CRC, ABI, boundaries, finite steps')
