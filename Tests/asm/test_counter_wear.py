"""Точный слой ID: настоящий ELF и NOR с подтверждением адресом, переносом страницы и SJ."""
import struct
from collections import Counter
from storage_machine import StorageMachine, crc16
from test_flash_wear import fast
from test_id_sync_fast import cell, BASE, SIZE

class CounterMachine(StorageMachine):
    def __init__(self, image=None):
        super().__init__(bytearray(b'\xff'*0x200000) if image is None else image)
        self.write(self.addr('mem')+20,SIZE)
        if len(self.flash)==0x200000:
            self.write(self.device+self.d['memID'],0x14c2,2)
            self.u.mem_write(self.addr('mem')+9*8,bytes(8))
        self.write(self.addr('trial'),0,1)
        fast(self)
        self.erases=Counter()
    def checked(self,name,*args,budget=50000000):
        return super().checked(name,*args,budget=budget)
    def change(self,kind,a,b):
        super().change(kind,a,b)
        if kind=='erase': self.erases[a]+=1
    def boot(self):
        assert self.checked('counter_boot',budget=10000000)
    def logical(self,key):
        assert self.checked('mem_read',BASE+32*(key-1),0x21000300,32,budget=1000000)
        return bytes(self.u.mem_read(0x21000300,32))
    def update(self,key,value):
        address=BASE+32*(key-1)
        self.u.mem_write(0x21000000,value)
        assert self.checked('store_id_begin',address,0x21000000)
        for _ in range(100000):
            self.now+=1
            self.checked('user_storage_poll',budget=50000000)
            if self.exec_state()[0]!=12: break
        else: raise AssertionError(('counter stuck',key,self.exec_state()))
        assert self.io is None and self.flash_ready(),self.exec_state()
        assert self.logical(key)==value,(key,self.logical(key).hex(),value.hex())
    def flash_ready(self):return self.u.mem_read(self.addr('mem_storage_ready'),1)==b'\x01'
    def seed(self,count):
        for k in range(1,count+1):self.flash[BASE+32*(k-1):BASE+32*k]=cell(k)
        self.write(self.addr('id_used_bytes'),count*32)
        self.write(self.addr('id_cells_end'),count,2)
        self.boot()
        self.erases.clear()
    def check_crc(self,expected):
        self.checked('idcrc_unknown')
        for _ in range(30000):
            self.checked('idcrc_background_begin')
            self.checked('idcrc_background_poll',budget=1000000)
            self.checked('flash_exec_take_result')
            if self.checked('idcrc_get',0x21000500): break
        else: raise AssertionError('CRC scan stalled')
        actual=int.from_bytes(self.u.mem_read(0x21000500,2),'little')
        assert actual==crc16(expected),(hex(actual),hex(crc16(expected)))


def smoke():
    m=CounterMachine();m.seed(2);m.record_changes=True
    expected=bytearray(m.flash[BASE:BASE+SIZE]);m.check_crc(expected);v=bytearray(cell(1));v[21:29]=b'\x09\x01\x02\x03\x04\x05\x06\x07'
    m.update(1,bytes(v));expected[:32]=v
    assert not m.erases and m.flash[BASE:BASE+32]==cell(1)
    assert m.checked('idcrc_get',0x21000500)
    assert int.from_bytes(m.u.mem_read(0x21000500,2),'little')==crc16(expected)
    m.check_crc(expected)
    r=CounterMachine(m.flash);r.boot();assert r.logical(1)==v;r.check_crc(expected)
    return {'programs':len(m.changes),'erases':sum(m.erases.values()),'crc_exact':True,'reboot':True}

def ring(events=1600, active=1, hot=False, spread=False):
    m=CounterMachine()
    keys=[(i*47)%50048+1 if spread else i+1 for i in range(active)]
    for k in keys:m.flash[BASE+32*(k-1):BASE+32*k]=cell(k)
    m.write(m.addr('id_used_bytes'),max(keys)*32);m.write(m.addr('id_cells_end'),max(keys),2);m.boot()
    expected=bytearray(m.flash[BASE:BASE+SIZE])
    for i in range(events):
        key=keys[0 if hot and i>=active else i%active];v=bytearray(cell(key));v[21:29]=struct.pack('<Q',i+1)
        m.update(key,bytes(v));expected[(key-1)*32:key*32]=v
        # Ограничиваем внутренний кэш Unicorn, сохраняя RAM без сброса прошивки.
        if i%128==127:
            ram=bytes(m.u.mem_read(0x20000000,0x5000));q=CounterMachine(m.flash)
            q.u.mem_write(0x20000000,ram);q.now=m.now;q.erases=m.erases;m=q
    m.check_crc(expected)
    r=CounterMachine(m.flash);r.boot();r.check_crc(expected)
    for key in keys:assert r.logical(key)==expected[(key-1)*32:key*32],key
    return {'events':events,'active':active,'hot':hot,'spread':spread,'max_erases':max(m.erases.values(),default=0),'erases':{hex(k):v for k,v in m.erases.items()},'reboot_crc_exact':True}


def recover(image):
    m=CounterMachine(image)
    assert m.checked('mem_recover',budget=10000000)
    m.boot()
    return m

def cut_trace(before,changes,expected,key,new,exhaustive=False,select=None):
    image=bytearray(before);cuts=0
    for kind,a,data in changes:
        positions=range(len(data)+1) if exhaustive and len(data)<=32 else sorted({0,1,len(data)//2,len(data)-1,len(data)})
        if select and not select(kind,a,data): positions=[]
        for n in positions:
            cut=bytearray(image)
            for j,b in enumerate(data[:n]):cut[a+j]=b if kind=='erase' else cut[a+j]&b
            r=recover(cut)
            for k,value in expected.items():
                got=r.logical(k)
                assert got==value or (k==key and got==new),(kind,hex(a),n,k,got.hex(),value.hex())
            cuts+=1
        for j,b in enumerate(data):image[a+j]=b if kind=='erase' else image[a+j]&b
    r=recover(image);assert r.logical(key)==new
    return cuts

def append_power():
    m=CounterMachine();m.seed(2);before=bytes(m.flash)
    old=cell(1);new=bytearray(old);new[21:29]=struct.pack('<Q',0x12345678)
    m.record_changes=True;m.update(1,bytes(new))
    return {'cuts':cut_trace(before,m.changes,{1:old,2:cell(2)},1,bytes(new),True)}

def fold_power():
    m=CounterMachine();m.seed(2)
    v=bytearray(cell(1));v[21]=7;m.update(1,bytes(v))
    w=bytearray(cell(2));w[21]=8;m.update(2,bytes(w))
    before=bytes(m.flash);m.record_changes=True
    assert m.checked('counter_drain_begin')
    drain_sj(m)
    assert m.flash[BASE:BASE+64]==v+w
    cuts=cut_trace(before,m.changes,{1:bytes(v),2:bytes(w)},1,bytes(v))
    # Замена целой ячейки идёт штатным SJ и обновляет только её запись слоя.
    new=bytearray(cell(1));new[2:12]=b'9876543210';m.update(1,bytes(new))
    q=recover(m.flash);assert q.logical(1)==new and q.logical(2)==w
    return {'cuts':cuts,'writes':len(m.changes),'replacement_no_resurrection':True}


def drain_sj(m):
    for _ in range(3000):
        m.now+=1;m.checked('counter_poll',m.now)
        if m.checked('counter_busy'):continue
        assert m.flash_ready();return
    raise AssertionError('SJ did not finish')



def gc_power():
    m=CounterMachine();m.seed(2)
    a=bytearray(cell(1));a[21]=7;m.update(1,bytes(a))
    for i in range(407):
        b=bytearray(cell(2));b[21:29]=struct.pack('<Q',i+1);m.update(2,bytes(b))
    before=bytes(m.flash);m.record_changes=True
    new=bytearray(b);new[21:29]=struct.pack('<Q',999999);m.update(2,bytes(new))
    assert m.erases=={0x1fb000:1},m.erases
    cuts=cut_trace(before,m.changes,{1:bytes(a),2:bytes(b)},2,bytes(new),True)
    return {'cuts':cuts,'commands':len(m.changes),'page_transfer':True}


def access_days():
    from unicorn import UC_HOOK_CODE
    from storage_machine import fields
    def setup(image=None):
        m=CounterMachine(image);m.outputs=[]
        for name,fn in [('out_getmodestate',lambda:m.ret(0)),('out_load',lambda:(m.outputs.append(m.args()),m.ret())),('log_addControlEvent',lambda:m.ret(1))]:
            a=m.addr(name);m.stubs[a]=fn;m.u.hook_add(UC_HOOK_CODE,m.hook,begin=a,end=a)
        if image is None:
            m.seed(1);v=bytearray(cell(1));v[20]=v[21]=2;struct.pack_into('<H',v,17,60)
            m.flash[BASE:BASE+32]=v
        else:
            m.write(m.addr('id_cells_end'),1,2);m.write(m.addr('id_used_bytes'),32);m.boot()
        return m
    def attempt(m,time):
        m.write(m.device+m.d['datetime'],time);m.u.mem_write(0x21000400,cell(1)[2:12]+b'\0')
        before=len(m.outputs);accepted=m.checked('user_activateOut',1,0,2,0x21000400,budget=1000000)
        for _ in range(2000):
            if m.exec_state()[0]!=12:break
            m.now+=1;m.checked('user_storage_poll',budget=1000000)
        assert (len(m.outputs)>before)==bool(accepted)
        return bool(accepted)
    t=1790000000;m=setup();assert attempt(m,t);assert attempt(m,t);assert not attempt(m,t)
    assert m.logical(1)[21]==0 and m.logical(1)[22:26]==struct.pack('<I',t)
    for _ in range(3):
        m=setup(m.flash);assert not attempt(m,0);assert m.logical(1)[21]==0
    assert attempt(m,t+86400) and m.logical(1)[21]==1
    assert m.logical(1)[22:26]==struct.pack('<I',t)
    # Точное прежнее правило: известный день != сохранённому, в том числе назад.
    assert attempt(m,t) and m.logical(1)[21]==1
    return {'same_day_limit':2,'unknown_time_reboots':3,'first_useTime_preserved':True,'backwards_day':'matches unchanged id_new_day (resets on inequality)'}


def server_crc():
    from test_id_sync_fast import SyncMachine,fixture
    from unicorn import UC_HOOK_CODE
    class M(CounterMachine,SyncMachine):pass
    m=M();fixture(m,130);m.boot()
    a=bytearray(cell(1));a[21]=7;m.update(1,bytes(a))
    b=bytearray(cell(130));b[21:29]=struct.pack('<Q',123456);m.update(130,bytes(b))
    cached=bytearray(m.flash[BASE:BASE+SIZE]);cached[:32]=a;cached[129*32:130*32]=b
    m.check_crc(cached)
    # Команда 13 заменяет первый сектор; состояние 130 остаётся только в слое.
    m.command(13,cell(1,1),1)
    peak=0
    for _ in range(20000):
        if not m.active():break
        m.now+=1;m.checked('user_storage_poll',budget=1000000);peak=max(peak,m.read_budget)
        assert m.io_budget<=1
    else:raise AssertionError('table did not finish')
    expected=bytearray(m.flash[BASE:BASE+SIZE]);expected[129*32:130*32]=b
    assert m.sent[-1][14:16]==struct.pack('<H',crc16(expected)),(m.sent[-1].hex(),hex(crc16(expected)))
    assert m.logical(1)==cell(1,1) and m.logical(130)==b
    # 101 не содержит CRC, но число ID и его проводной формат должны остаться прежними.
    m.sent.clear();body=struct.pack('<HBBB',3,0,7,101);body+=struct.pack('<H',crc16(body))
    m.u.mem_write(0x21000000,body);m.receive_command(0x21000000,0)
    for _ in range(20000):
        if m.sent:break
        m.now+=1;m.checked('user_storage_poll',budget=1000000);peak=max(peak,m.read_budget)
    assert m.sent and m.sent[-1][8]==101,m.exec_state()
    assert len(m.sent[-1])==42 and m.sent[-1][37:39]==struct.pack('<H',130)
    status=m.sent[-1].hex();m.sent.clear()
    assert m.checked('service_crc_begin_id',1,0,8)
    for _ in range(20000):
        if m.sent:break
        m.now+=1;m.checked('user_storage_poll',budget=1000000);peak=max(peak,m.read_budget)
    assert m.sent and m.sent[-1][10:12]==struct.pack('<H',crc16(expected[:16000]))
    return {'command13_crc_exact':True,'command18_crc_exact':True,'command101_hex':status,'max_read_bytes_per_poll':peak}

# Замена сектора без строк слоя сохраняет кэш CRC; сектор со строкой даёт точную CRC после сброса кэша.
def replace_keeps_crc():
    from test_id_sync_fast import SyncMachine,fixture
    class M(CounterMachine,SyncMachine):pass
    m=M();fixture(m,130);m.boot()
    b=bytearray(cell(130));b[21:29]=struct.pack('<Q',123456);m.update(130,bytes(b))
    cached=bytearray(m.flash[BASE:BASE+SIZE]);cached[129*32:130*32]=b
    m.check_crc(cached)
    def table(data):
        m.command(13,data,1)
        for _ in range(20000):
            if not m.active():break
            m.now+=1;m.checked('user_storage_poll',budget=1000000)
        else:raise AssertionError('table did not finish')
    # Сектор 0 без строк слоя: кэш остаётся готовым, ответ команды берёт CRC из него.
    table(cell(1,1))
    expected=bytearray(m.flash[BASE:BASE+SIZE]);expected[129*32:130*32]=b
    assert m.checked('idcrc_get',0x21000500),'CRC cache dropped by a sector without layer rows'
    assert int.from_bytes(m.u.mem_read(0x21000500,2),'little')==crc16(expected)
    assert m.sent[-1][14:16]==struct.pack('<H',crc16(expected)),m.sent[-1].hex()
    # Сектор 1 со строкой 130: кэш сбрасывается, CRC считается по базе с согласованным слоем.
    table(cell(130,1))
    expected=bytearray(m.flash[BASE:BASE+SIZE])
    assert m.sent[-1][14:16]==struct.pack('<H',crc16(expected)),m.sent[-1].hex()
    assert m.logical(1)==cell(1,1) and m.logical(130)==cell(130,1)
    return {'cache_kept_without_rows':True,'crc_exact_with_rows':True}

if __name__=='__main__':
    import sys
    print(ring(int(sys.argv[1]),int(sys.argv[2]),len(sys.argv)>3) if len(sys.argv)>1 else smoke())


def overflow():
    m=CounterMachine();m.seed(409)
    expected={}
    for key in range(1,409):
        value=bytearray(cell(key));value[21:29]=struct.pack('<Q',key)
        m.update(key,bytes(value));expected[key]=bytes(value)
    before=bytes(m.flash);m.record_changes=True
    new=bytearray(cell(409));new[21]=17;m.update(409,bytes(new))
    assert sum(m.erases[BASE+i*4096] for i in range(4))==1,m.erases
    assert m.erases[0x1fa000]==2 and m.erases[0x1f9000]==1,m.erases
    for key,value in expected.items():assert m.logical(key)==value
    image=bytearray(before);cuts=0
    # Между секторными SJ и публикацией страницы: сохраняются все прежние значения.
    for kind,address,data in m.changes:
        if (kind=='erase' and address<0x1c8000) or address==0x1fc008:
            for count in (0,len(data)//2,len(data)):
                cut=bytearray(image)
                for j,b in enumerate(data[:count]):cut[address+j]=b if kind=='erase' else cut[address+j]&b
                r=recover(cut)
                for key in (1,128,129,256,384,408):assert r.logical(key)==expected[key]
                assert r.logical(409) in (cell(409),bytes(new))
                cuts+=1
        for j,b in enumerate(data):image[address+j]=b if kind=='erase' else image[address+j]&b
    return {'live_records':408,'base_sectors':1,'SJ_META_erases':2,'boundary_cuts':cuts}
