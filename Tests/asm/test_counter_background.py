"""Фоновый перенос: тот же ELF, NOR 2 МиБ, реле и время SPI/BUSY."""
import math
import struct
from unicorn import UC_HOOK_CODE
from test_counter_wear import CounterMachine,cell,BASE,SIZE,recover,cut_trace
from storage_machine import fields


def value(key,n):
    v=bytearray(cell(key));v[21:29]=struct.pack('<Q',n);return bytes(v)


def fill(m,keys=408):
    m.seed(keys+1)
    for k in range(1,keys+1):m.update(k,value(k,k))


def background_power():
    m=CounterMachine();fill(m,307)
    before=bytes(m.flash);m.record_changes=True
    m.checked('user_storage_poll')
    assert m.checked('counter_busy') and m.exec_state()[0]==0,m.exec_state()
    polls=0
    while m.checked('counter_busy'):
        m.now+=1;m.checked('user_storage_poll');polls+=1
        assert m.io_budget<=1,'more than one asynchronous command in a loop'
    assert m.erases[0x1fa000]==2 and m.erases[0x1f9000]==1
    assert sum(n for a,n in m.erases.items() if BASE<=a<BASE+SIZE)==1
    expected={k:value(k,k) for k in (1,128,129,256,257,307)}
    cuts=cut_trace(before,m.changes,expected,307,expected[307],select=lambda k,a,d:k=='erase' or a==0x1fa01c)
    # После отпускания одного SJ следующий проход сразу принимается.
    m.update(308,value(308,999));assert m.logical(308)==value(308,999)
    assert not m.checked('counter_pending')
    return dict(cuts=cuts,polls=polls,one_sector=True,access_between_sectors=True)


def external_power():
    m=CounterMachine();m.seed(130)
    m.update(1,value(1,111));m.update(2,value(2,222));m.update(130,value(130,333))
    before=bytes(m.flash);m.record_changes=True
    new=bytearray(cell(1));new[2:12]=b'9876543210';new[21:29]=struct.pack('<Q',444)
    m.update(1,bytes(new))
    cuts=cut_trace(before,m.changes,{1:value(1,111),2:value(2,222),130:value(130,333)},1,bytes(new))
    assert m.erases[0x1fa000]==2 and m.erases[0x1f9000]==1
    assert m.flash[BASE+129*32:BASE+130*32]==cell(130),'external write flushed unrelated sector'
    return dict(cuts=cuts,unrelated_overlay_preserved=True)


class Timed(CounterMachine):
    def __init__(self,image=None,erase_ms=40,program_ms=1):
        self.us=0;self.ready=0;self.verify=0;self.erase_us=erase_ms*1000;self.program_us=program_ms*1000
        self.relay=[];self.max_poll_us=0
        super().__init__(image)
        for name,fn in [('out_load',self.output),('log_addControlEvent',lambda:self.ret(1))]:
            self.stubs[self.addr(name)]=fn
            self.u.hook_add(UC_HOOK_CODE,self.hook,begin=self.addr(name),end=self.addr(name))
    def spi(self,n):self.us+=math.ceil((n+4)*8/18) # SPI 18 МГц
    def read(self):self.spi(self.args()[2]);super().read()
    def sync_write(self):
        a,_,n,_=self.args();self.spi(n);self.us+=self.program_us*((a%256+n+255)//256)
        self.spi(n);super().sync_write()
    def sync_erase(self):self.us+=self.erase_us;self.spi(4096);super().sync_erase()
    def start(self):
        _,p,n,_=self.args();self.spi(n if p else 0);super().start()
        self.ready=self.us+(self.program_us if p else self.erase_us);self.verify=0
    def poll(self):
        self.spi(1)
        if self.us<self.ready:self.ret(1);return
        a,p,n,data,_=self.io
        got=min(64,len(data)-self.verify);self.spi(got);self.verify+=got
        assert self.flash[a:a+len(data)]==data
        if self.verify<len(data):self.ret(1);return
        self.io=None;self.ret(2)
    def tick(self):
        self.us+=1000;self.now=self.us//1000
        start=self.us;self.checked('user_storage_poll');self.max_poll_us=max(self.max_poll_us,self.us-start)
    def output(self):self.relay.append(self.us);self.ret()
    def pass_to_relay(self,key,n):
        start=self.us
        self.u.mem_write(0x21000000,value(key,n))
        while not self.checked('store_id_begin',BASE+32*(key-1),0x21000000):self.tick()
        offsets=fields('store_io_s')
        for name,v in [('outputs',1),('type',6),('event',74)]:self.write(self.job+offsets[name],v,2 if name=='event' else 1)
        while not self.relay:self.tick()
        return round((self.relay[0]-start)/1000,3)


def latency():
    result={}
    for label,erase,program in [('ordinary',40,1),('slow',400,3)]:
        timings={}
        for scenario in ('plain','append','compact','full_distinct','background','background_busy','background_full','full_spread'):
            m=Timed(erase_ms=erase,program_ms=program)
            if scenario=='full_spread':
                m.seed(50048)
                for k in list(range(1,19))+list(range(129,50049,128)):m.update(k,value(k,k))
            elif scenario in ('full_distinct','background_full'):fill(m)
            elif scenario in ('background','background_busy'):fill(m,307)
            else:
                m.seed(2)
                if scenario=='compact':
                    for i in range(408):m.update(1,value(1,i+1))
            if scenario=='plain':m.stubs[m.addr('counter_write')]=lambda:m.ret(0);m.u.hook_add(UC_HOOK_CODE,m.hook,begin=m.addr('counter_write'),end=m.addr('counter_write'))
            if scenario.startswith('background'):
                m.tick();assert m.checked('counter_busy')
                if scenario=='background_busy':
                    while not (m.io and m.io[0]==BASE+8192 and not m.io[1]):m.tick()
            m.erases.clear();m.max_poll_us=0
            key=50048 if scenario=='full_spread' else 409 if scenario in ('full_distinct','background_full') else 308 if scenario.startswith('background') else 1
            ms=m.pass_to_relay(key,999999)
            targets=sum(n for a,n in m.erases.items() if BASE<=a<BASE+SIZE)
            assert targets<=(1 if scenario in ('plain','full_distinct','background_full','full_spread') else 0)
            timings[scenario]=dict(relay_ms=ms,max_poll_ms=round(m.max_poll_us/1000,3),target_erases=targets)
            if scenario in ('background','background_busy'):
                assert m.checked('counter_busy'),'pass waited for complete background SJ'
                assert ms<timings['plain']['relay_ms'],(scenario,ms,timings)
        # Одна SJ и максимум одна страница; здесь измеряется конечный путь до out_load.
        assert timings['full_distinct']['target_erases']==1
        for name in ('append','compact','full_distinct','background','background_busy','background_full','full_spread'):
            assert timings[name]['max_poll_ms']<=timings['plain']['relay_ms'],(label,name,timings)
        result[label]=timings
    return result


def ring407(events=10000):
    m=CounterMachine();m.seed(407)
    for i in range(events):
        k=i%407+1;m.update(k,value(k,i+1))
        # Между проездами фон успевает закончить один сектор. Отдельный latency
        # проверяет непрерывный поток без этого зазора.
        if m.checked('counter_pending'):
            m.checked('user_storage_poll')
            while m.checked('counter_busy'):m.now+=1;m.checked('user_storage_poll')
        if i%128==127:
            ram=bytes(m.u.mem_read(0x20000000,0x5000));q=CounterMachine(m.flash)
            q.u.mem_write(0x20000000,ram);q.erases=m.erases;q.now=m.now;m=q
    r=recover(m.flash)
    for k in range(1,408):
        last=events-1-(events-k)%407
        assert r.logical(k)==value(k,last+1)
    return dict(events=events,active=407,max_erases=max(m.erases.values()),erases={hex(a):n for a,n in m.erases.items()})


def interleaved_power():
    m=CounterMachine();m.seed(2)
    for i in range(407):m.update(1,value(1,i+1))
    m.update(2,value(2,222))
    expected={1:value(1,407),2:value(2,222)}
    image=bytes(m.flash);m.record_changes=True;start=0;cuts=0
    assert m.checked('counter_drain_begin')
    sf=fields('sj_runtime_state_s');sj=m.addr('sjr')
    seen=set()
    for _ in range(3000):
        phase=int.from_bytes(m.u.mem_read(sj+sf['phase'],1),'little')
        offset=int.from_bytes(m.u.mem_read(sj+sf['offset'],2),'little')
        # SOURCE уже записал первые ячейки; COPY читает образ вместо стёртой базы.
        point=1 if phase==3 and offset>=64 else 2 if phase==12 and offset==0 else 3 if phase==15 else 0
        if point and point not in seen and not m.io:
            seen.add(point)
            changes=m.changes[start:]
            cuts+=cut_trace(image,changes,expected,1,expected[1]);image=bytes(m.flash);start=len(m.changes)
            key=2 if point==3 else 1
            new=cell(1) if point==2 else value(key,700+point)
            assert m.checked('flash_exec_read_allowed')
            assert all(m.logical(k)==v for k,v in expected.items())
            m.update(key,new)
            assert m.checked('counter_busy'),'append completed only after SJ'
            cuts+=cut_trace(image,m.changes[start:],expected,key,new,True)
            expected[key]=new;image=bytes(m.flash);start=len(m.changes)
        if not m.checked('counter_busy'):break
        m.now+=1;m.checked('user_storage_poll')
    else:raise AssertionError('background did not finish')
    assert seen=={1,2,3},seen
    cuts+=cut_trace(image,m.changes[start:],expected,1,expected[1])
    r=recover(m.flash)
    assert all(r.logical(k)==v for k,v in expected.items())
    logical=bytearray(m.flash[BASE:BASE+SIZE]);logical[:32]=expected[1];logical[32:64]=expected[2]
    m.check_crc(logical)
    return dict(cuts=cuts,appends_before_SJ_finished=3,erased_target_reads=True,page_transfer_during_SJ=True)


def burst_limit():
    # Непрерывно разные ID, без иных работ; измеряем минимальную частоту,
    # при которой 102-й новый запрос после старта при 307 ждёт конца SJ.
    seed=CounterMachine();seed.seed(512)
    for k in range(1,308):seed.update(k,value(k,k))
    image=bytes(seed.flash)
    result={}
    for label,erase,program in [('ordinary',40,1),('slow',400,3)]:
        def run(period):
            m=Timed(image,erase,program);m.boot();m.us=0
            assert m.checked('counter_drain_begin')
            start=m.us;next_request=start;key=308;pending=False;waiting=False
            sf=fields('store_io_s')
            for _ in range(10000):
                if not m.checked('counter_busy'):break
                if not pending and key<=409 and m.us>=next_request:
                    m.u.mem_write(0x21000000,value(key,9999+key))
                    if m.checked('store_id_begin',BASE+32*(key-1),0x21000000):pending=True
                m.tick()
                if pending:
                    if m.exec_state()[0]==12 and m.u.mem_read(m.job+sf['mode'],1)==b'\x02':waiting=True;break
                    if m.exec_state()[0]!=12:pending=False;key+=1;next_request+=period*1000
            else:raise AssertionError('burst did not finish')
            return waiting,round((m.us-start)/1000,3),key-308
        lo,hi=0,100
        assert run(lo)[0] and not run(hi)[0]
        while hi-lo>1:
            mid=(lo+hi)//2
            if run(mid)[0]:lo=mid
            else:hi=mid
        wait=run(lo);no_wait=run(hi)
        result[label]=dict(wait_period_ms=lo,wait_ids_per_minute=60000/lo if lo else None,
                           no_wait_period_ms=hi,no_wait_ids_per_minute=60000/hi,
                           waiting=wait,completed=no_wait)
    return result
