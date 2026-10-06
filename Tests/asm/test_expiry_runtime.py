from test_work_memory import take, drop, owned, PHOTO, PHOTO_PACKET, REPLY, CONFIG, FW_CONTEXT, FW_DATA, TABLE, UP_CONTEXT, UP_PACKET, SIZES
"""Bounded background expiry against the preserved historical IAR body."""
import hashlib,json,struct
from storage_machine import *

BASE=0x41000;NOW=1700000400

class ExpiryMachine(StorageMachine):
    def __init__(self,flash=None):
        super().__init__(flash);self.expiry_events=[];self.runtime_costs=[];self.runtime_reads=[]
        self.stubs[self.addr('log_addControlEvent')]=self.expiry_event
        self.write(self.device+self.d['datetime'],NOW)
        self.write(self.addr('id_cells_end'),0,2)
        self.write(self.addr('id_endScan'),1,1)
        self.write(self.addr('mem')+2*8+4,8192)
        self.legacy=False
    def read(self):
        a,p,n,_=self.args();self.read_budget+=n
        assert 0<n<=1024 and a+n<=len(self.flash),(hex(a),n)
        if self.failed('read'):self.ret(0);return
        self.u.mem_write(p,bytes(self.flash[a:a+n]));self.ret(1)
    def expiry_event(self):
        code,p,value,_=self.args();self.expiry_events.append((code,bytes(self.u.mem_read(p,10)).hex(),value));self.ret(1)
    def install(self,records,physical=None):
        cells=range(len(records)) if physical is None else physical
        for i,b in zip(cells,records):self.flash[BASE+i*32:BASE+(i+1)*32]=b
        self.write(self.addr('id_cells_end'),max(cells,default=-1)+1,2)
    def start_expiry(self):return self.checked('expiry_runtime_begin')
    def expiry_active(self):return self.checked('expiry_runtime_active')!=0
    def expiry_tick(self):
        self.now+=1;self.checked('expiry_runtime_poll',self.now)
        assert self.read_budget<=128 and self.io_budget<=1
        self.runtime_costs.append(self.steps);self.runtime_reads.append(self.read_budget)
    def expiry_drain(self,limit=10000):
        for _ in range(limit):
            if not self.expiry_active():return
            self.expiry_tick()
        raise AssertionError('expiry stuck')
    def legacy_run(self):
        # The four-byte manual branch selects the new runtime implementation.
        self.symbols['expiry_legacy']=self.addr('user_checkIdTimeEnd')+4
        self.symboltypes['expiry_legacy']='STT_FUNC'
        self.checked('expiry_legacy',budget=5000000)
    def result(self):
        return (bytes(self.flash[BASE:BASE+8192]),self.expiry_events,
                bytes(self.u.mem_read(self.addr('id_nextEnd'),4)),
                bytes(self.u.mem_read(self.addr('id_endScan'),1)))

def record(index=1,first=NOW-120,minutes=1,done=0,tag=b'abcdefghij'):
    b=bytearray(32);struct.pack_into('<H',b,0,index);b[2:12]=tag
    struct.pack_into('<H',b,17,minutes);struct.pack_into('<I',b,22,first);b[28]=done
    return bytes(b)

def differential():
    scenarios=[([record()],None),([record(first=NOW,minutes=2)],None),
       ([record(done=56),record(2,first=NOW-86400)],None),
       ([record(9)],(4,)),([record(2),record(2,tag=b'0123456789')],(0,1)),
       ([record(8),record(8,tag=b'0123456789')],(1,4)),
       ([record(0)],(3,)),([record(minutes=0),record(2,first=0xffffffff)],None),
       ([record(128),record(129)],(127,128))]
    out=[]
    for rows,cells in scenarios:
        old=ExpiryMachine();old.install(rows,cells);old.legacy_run()
        new=ExpiryMachine();new.install(rows,cells);assert new.start_expiry();new.expiry_drain()
        assert new.result()==old.result(),(rows,cells,new.expiry_events,old.expiry_events)
        out.append({'records':len(rows),'positions':list(cells) if cells else None,
                    'passes':len(new.runtime_costs),'max_instructions':max(new.runtime_costs),
                    'total_instructions':sum(new.runtime_costs),'max_reads':max(new.runtime_reads),
                    'stack':new.stack_peak,'legacy_instructions':old.costs[-1],'legacy_stack':old.stack_peak})
    return out

def cancel_and_sessions():
    count=0
    # Cancel scan/lookup before any journal work and every durability phase.
    for phase in (0,16,1,2,3,5,6,8,9,10,12,14,15,30):
        m=ExpiryMachine();m.install([record()]);assert m.start_expiry()
        if phase:
            for _ in range(3000):
                if m.u.mem_read(m.addr('sjr')+54,1)[0]==phase:break
                m.expiry_tick()
            else:raise AssertionError(phase)
        m.checked('expiry_runtime_cancel');m.expiry_drain()
        assert m.io is None and m.u.mem_read(m.addr('id_endScan'),1)==b'\1'
        if phase in (9,10,12,14,15,30):assert m.flash[BASE+28]==56 and len(m.expiry_events)==1
        else:assert m.flash[BASE+28]==0 and not m.expiry_events
        assert not m.checked('mem_config_result')
        count+=1
    m=ExpiryMachine();m.install([record()]);assert m.start_expiry()
    m.checked('user_comm_session',0);m.write(m.addr('usb_bus_resets'),1)
    m.expiry_drain();assert len(m.expiry_events)==1;count+=1
    m=ExpiryMachine();m.install([record()]);assert m.start_expiry()
    m.checked('user_savereset');m.expiry_drain();assert not m.expiry_events
    assert m.u.mem_read(m.addr('store_reset_requested'),1)==b'\1';count+=1
    return count

def faults():
    count=0
    for kind,points in [('read',(1,2,3,65,130,192,257)),('start',(1,2,3,66,68,100)),('poll',(1,10,200,390))]:
        for point in points:
            m=ExpiryMachine();m.install([record()]);m.fault=(kind,point);assert m.start_expiry();m.expiry_drain()
            if m.counts[kind]<point:continue
            assert not m.expiry_events and m.io is None
            assert m.u.mem_read(m.addr('id_endScan'),1)==b'\1'
            assert not m.checked('expiry_runtime_active');count+=1
    m=ExpiryMachine();m.install([record()]);p=take(m,REPLY);assert p
    before=bytes(m.flash);assert not m.start_expiry()
    assert bytes(m.flash)==before and not any(m.counts.values()) and not m.expiry_events;count+=1
    # Число ячеек задаёт проверенный скан основной базы, внутренний вызов не повторяет границу.
    m=ExpiryMachine();m.install([record()])
    used=int.from_bytes(m.u.mem_read(m.addr('id_cells_end'),2),'little')*32
    size=int.from_bytes(m.u.mem_read(m.addr('mem')+20,4),'little')
    assert used<=size and not used&31;count+=1
    m=ExpiryMachine();m.install([record()]);m.write(m.addr('store_save_requested'),1,1)
    assert not m.start_expiry();count+=1
    return count

def power():
    m=ExpiryMachine();m.install([record()]);initial=bytes(m.flash);m.record_changes=True
    assert m.start_expiry();m.expiry_drain();final=bytes(m.flash[BASE:BASE+4096])
    state=bytearray(initial);count=0
    for kind,a,data in m.changes:
        for n in sorted(set((0,1,len(data)//2,len(data)-1,len(data)))):
            cut=bytearray(state)
            for i,v in enumerate(data[:n]):cut[a+i]=v if kind=='erase' else cut[a+i]&v
            r=ExpiryMachine(cut);assert r.checked('sj_recover',budget=2000000)
            assert bytes(r.flash[BASE:BASE+4096]) in (initial[BASE:BASE+4096],final)
            count+=1
        if kind=='erase':state[a:a+4096]=b'\xff'*4096
        else:
            for i,v in enumerate(data):state[a+i]&=v
    return count

def full_scan_and_heap():
    from test_memory_runtime import heap_stats
    m=ExpiryMachine();m.write(m.addr('mem')+20,1601536)
    m.install([record(50048,first=NOW,minutes=1)],(50047,))
    live=[(slot,take(m,slot)) for slot in (PHOTO,PHOTO_PACKET)]
    assert all(live) and m.start_expiry()
    ctx=int.from_bytes(m.u.mem_read(m.addr('expiry_job'),4),'little')
    assert ctx==m.addr('work_memory')+184
    peak=heap_stats(m);assert owned(m)==7
    m.expiry_drain(80000)
    assert not m.expiry_events and m.io is None
    assert int.from_bytes(m.u.mem_read(m.addr('id_nextEnd'),4),'little')==NOW+60
    assert not m.start_expiry(),'future minimum must suppress a repeat sweep'
    for slot,p in live:drop(m,slot,p)
    assert owned(m)==0
    return {'id_bytes':1601536,'passes':len(m.runtime_costs),'max_instructions':max(m.runtime_costs),
            'total_instructions':sum(m.runtime_costs),'max_read_bytes':max(m.runtime_reads),
            'stack':m.stack_peak,'context_request':132,'workspace_bytes':2224,
            'live_workspace_payload':180+534+132,'free_at_overlap':peak}

if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser();p.add_argument('--power',action='store_true');a=p.parse_args()
    out={'differential':differential()};print('legacy/canonical/fallback/boundaries PASS',flush=True)
    out['cancel_reset_sessions']=cancel_and_sessions();out['faults_oom']=faults()
    print('cancel/reset/session/I/O/OOM/priority PASS',flush=True)
    out['full_scan_heap']=full_scan_and_heap();print('full ID/heap/fragmentation/coalescing PASS',flush=True)
    if a.power:out['power_cuts']=power();print('power cuts',out['power_cuts'],'PASS',flush=True)
    if not a.power:
        previous=ROOT/'Tests/asm/expiry_runtime_results.json'
        if previous.exists():
            old=json.loads(previous.read_text())
            if old.get('power_cuts'):
                out['power_cuts']=old['power_cuts']
                out['power_elf_sha256']=old.get('power_elf_sha256',old.get('elf_sha256'))
    out['elf_sha256']=hashlib.sha256((ROOT/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest()
    (ROOT/'Tests/asm/expiry_runtime_results.json').write_text(json.dumps(out,indent=2)+'\n')
