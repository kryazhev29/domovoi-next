from test_work_memory import take, drop, owned, PHOTO, PHOTO_PACKET, REPLY, CONFIG, FW_CONTEXT, FW_DATA, TABLE, UP_CONTEXT, UP_PACKET, SIZES
"""Handwritten command19/start/rollback in final IAR ELF; no hardware timing claims."""
import hashlib,json,struct
from test_upload_runtime import UploadMachine,largest_free
from storage_machine import *
from unicorn import UC_HOOK_CODE

def fwcrc(data):
    c=0xffffffff
    for (v,) in struct.iter_unpack('<I',data):
        c^=v
        for _ in range(32):c=((c<<1)^ (0x04c11db7 if c&0x80000000 else 0))&0xffffffff
    return c

def header(image,number=4):
    b=bytearray(b'\xff'*32);b[4]=3
    struct.pack_into('<H',b,5,number);struct.pack_into('<II',b,8,len(image),fwcrc(image));struct.pack_into('<I',b,20,123456)
    return bytes(b)

class FirmwareMachine(UploadMachine):
    def __init__(self):
        super().__init__();self.stubs[self.addr('usb_try_send')]=self.usb
        self.u.mem_write(self.addr('mem'),struct.pack('<IIII',0x1000,0x19000,0x1a000,0x19000))
        self.write(0x08007ffc,0x40,1);self.write(self.device+19,3,1)
        self.poll_costs=[];self.max_fw_read=0;self.logs=[];self.nor_phases=[];self.alloc_trace=[];self.phase_costs={};self.snapshot_ctx=None;self.snapshot_cfg=None
        self.stubs[self.addr('log_addUpdateEvent')]=lambda:(self.logs.append(tuple(self.args()[:2])),self.ret())[-1]
        self.stubs[self.addr('w25_erase_sector')]=lambda:(_ for _ in ()).throw(AssertionError('synchronous erase'))
        self.stubs[self.addr('w25_write_data')]=lambda:(_ for _ in ()).throw(AssertionError('synchronous write'))
        self.u.hook_add(UC_HOOK_CODE,self.allocator_trace)
    def allocator_trace(self,u,a,n,x):
        if a==self.addr('work_acquire'):self.alloc_trace.append(('malloc',self.args()[0]))
        if a==self.addr('work_release'):self.alloc_trace.append(('free',self.args()[0]))
        if a==self.addr('store_snapshot'):
            self.alloc_trace.append(('snapshot',0));self.snapshot_ctx=self.read_word(self.addr('fw_service_context'))
        if a==self.addr('mem_config_begin'):self.alloc_trace.append(('config_begin',0))
    def ptr(self):return self.read_word(self.addr('fw_service_context'))
    def phase(self):return self.read_word(self.ptr()+120,1) if self.ptr() else 0
    def fixture(self,size=132):
        self.image=bytes((i*17+3)&255 for i in range(size));self.head=header(self.image)
        self.flash[0x1a000:0x1a020]=self.head
        self.flash[0x1a200:0x1a200+size]=self.image
        self.flash[0x1000:0x1a000]=b'\x5a'*0x19000
    def begin(self,number=1,ch=0,parser=False):
        req=struct.pack('<HIIIH',number,123456,len(self.image),fwcrc(self.image),(len(self.image)+1023)//1024)
        if parser:
            p=struct.pack('<HBBB',len(req)+3,0,7,19)+req;p+=struct.pack('<H',crc16(p));self.u.mem_write(0x21000000,p)
            self.checked('user_parse_comand',0x21000000,ch)
        else:
            self.u.mem_write(0x21000000,req);assert self.checked('fw_service_begin',0x21000000,ch,7)==1
        self.u.mem_write(0x21000000,b'\xa5'*32)
    def tick_fw(self):
        phase=self.phase();owner=self.checked('fw_service_active');self.now+=1
        before=self.u.reg_read(UC_ARM_REG_PRIMASK)
        self.checked('user_storage_poll');self.poll_costs.append(self.steps)
        assert self.u.reg_read(UC_ARM_REG_PRIMASK)==before
        assert self.io_budget<=1
        assert self.read_budget <= (64 if owner else 256),(phase,self.read_budget)
        if owner:self.max_fw_read=max(self.max_fw_read,self.read_budget)
        assert not self.lease
        self.phase_costs[phase]=max(self.phase_costs.get(phase,0),self.steps)
    def until(self,test,limit=50000):
        for i in range(limit):
            if test():return i
            self.tick_fw()
        raise AssertionError(('stuck',self.phase(),self.read_word(self.addr('store')+18,1),self.read_word(self.ptr()+127,1) if self.ptr() else 0))
    def drain(self):return self.until(lambda:not self.ptr())

def normal():
    out={}
    for number,size,ch,parser in [(2,4,0,True),(4,1028,1,True),(1,4,0,True),(1,132,1,False),(1,4096,0,False),(1,97792,0,False)]:
        m=FirmwareMachine();m.fixture(size);m.begin(number,ch,parser);assert m.ptr(),(number,size,m.answers)
        n=m.drain();assert len(m.sent)==1,(number,size,m.sent,m.logs)
        assert m.sent[0][9:11]==bytes((19,0));assert crc16(m.sent[0][4:-2])==int.from_bytes(m.sent[0][-2:],'little')
        assert m.flash[0x1000:0x1020]==m.head if number==1 else m.flash[0x1000+8:0x1000+24]==m.head[8:24]
        if number==1:
            assert m.flash[0x1200:0x1200+size]==m.image
            assert m.read_word(m.device+171,1)==1
            assert not m.read_word(m.addr('fw_rollback_end'))
        assert largest_free(m)==2032
        out[f'{number}_{size}_{ch}']={'polls':n,'max_poll_instructions':max(m.poll_costs),'total_poll_instructions':sum(m.poll_costs),'max_instructions':max(m.costs[:-26]),'total_instructions':sum(m.costs[:-26]),'stack':m.stack_peak,'max_fw_read':m.max_fw_read,'phase_maximum':m.phase_costs}
    return out


def heap_reservation():
    m=FirmwareMachine();m.fixture();photo=take(m,PHOTO);assert photo;m.begin(2)
    ctx=m.ptr();reserve=m.read_word(ctx+96);assert reserve
    m.until(lambda:m.phase()==6);trace=len(m.alloc_trace);m.tick_fw()
    events=m.alloc_trace[trace:]
    assert events[:4]==[('free',reserve),('snapshot',0),('config_begin',0),('malloc',1024)],events
    assert m.snapshot_ctx==0 and m.read_word(m.addr('cfg_job'))==reserve
    assert owned(m)==(1<<(PHOTO-1))|(1<<(FW_CONTEXT-1))|(1<<(CONFIG-1))
    m.drain();drop(m,PHOTO,photo);assert owned(m)==0
    for slot in (REPLY,CONFIG):
        m=FirmwareMachine();m.fixture();held=take(m,slot);assert held
        before=bytes(m.flash);m.begin(2)
        assert not m.ptr() and bytes(m.flash)==before and not m.counts['start'] and not m.sent
        drop(m,slot,held);assert owned(m)==0
    return {'context_config_handoff':True,'same_buffer':True,'conflict_cases':2,'storage_bytes':2224}

def boundaries_errors():
    count=0
    for number in (0,1,2):
        for size in (0,2,97796):
            m=FirmwareMachine();m.fixture(4);m.image=bytes(size);m.head=header(m.image[:size-size%4])
            # For malformed backup headers preserve a bounded byte fixture.
            if number==1:
                h=bytearray(m.head);struct.pack_into('<I',h,8,size);m.flash[0x1a000:0x1a020]=h
            req=struct.pack('<HIIIH',number,1,size,1,(size+1023)//1024)
            m.u.mem_write(0x21000000,req);m.checked('fw_service_begin',0x21000000,0,7)
            if m.ptr():m.drain();assert m.sent[-1][10]==(2 if number==1 else 1)
            else:assert m.answers and m.answers[-1][1]==1
            assert not m.counts['start'];count+=1
    # Valid header with incorrect complete CRC, or erased header, never erases stage.
    for erased in (False,True):
        m=FirmwareMachine();m.fixture();m.flash[0x1a200]^=1
        if erased:m.flash[0x1a000:0x1a020]=b'\xff'*32
        m.begin();m.drain();assert m.sent[-1][10]==2 and not m.counts['start'];count+=1
    # Existing download retransmission preserves staging and publishes one verified ACK.
    m=FirmwareMachine();m.fixture();m.flash[0x1000:0x1020]=m.head
    m.write(m.device+164,1,1);m.write(m.device+178,1,1);m.write(m.device+148,1,2)
    before=bytes(m.flash[0x1000:0x1a000]);m.begin(4,1);m.drain()
    assert m.sent[-1][10]==0 and bytes(m.flash[0x1000:0x1a000])==before;count+=1
    return count

def io_faults():
    count=0
    # Fail each FW operation category, including verified completion errors.
    for mode,phase,kind in [(2,4,'start'),(2,4,'poll'),(2,5,'start'),(2,5,'poll'),
                             (1,1,'read'),(1,2,'read'),(1,10,'start'),(1,10,'poll'),
                             (1,11,'read'),(1,12,'start'),(1,12,'poll'),(1,13,'read'),(1,14,'read')]:
        m=FirmwareMachine();m.fixture();m.begin(mode)
        m.until(lambda:m.phase()==phase)
        if kind=='poll':m.tick_fw();assert m.io
        m.fault=(kind,m.counts[kind]+1);m.drain()
        assert not m.read_word(m.device+171,1) and not m.read_word(m.addr('fw_rollback_end'))
        assert largest_free(m)==2032;count+=1
    return count

def busy_cancel():
    count=0
    for number in (1,2):
        for channel in (0,1):
            m=FirmwareMachine();m.fixture();m.busy=True;m.begin(number,channel)
            m.until(lambda:m.phase()==16);stage=bytes(m.flash[0x1000:0x1a000]);ctx=m.ptr()
            for _ in range(5):m.tick_fw();assert m.ptr()==ctx and not m.sent
            assert bytes(m.flash[0x1000:0x1a000])==stage
            m.busy=False;m.drain();assert len(m.sent)==1;count+=1
    for phase in (1,2,4,5,6,8,10,11,12,13,14,16):
        m=FirmwareMachine();m.fixture();m.begin(2 if phase in (4,5,6) else 1)
        m.until(lambda:m.phase()==phase)
        if phase in (4,5,10,12):m.tick_fw();assert m.io
        m.checked('fw_cancel',0)
        m.until(lambda:not m.ptr());assert m.io is None and not m.read_word(m.addr('fw_rollback_end'))
        # A cancelled config job has independent storage and must drain separately.
        m.until(lambda:m.read_word(m.addr('store')+18,1)==0)
        assert largest_free(m)==2032;count+=1
    return count


def power_cuts():
    m=FirmwareMachine();m.fixture();m.flash[0x1000:0x1004]=b'\xff'*4
    # A valid legacy configuration remains selectable until the successor commits.
    for name,off in [('config',0),('device',512)]:
        data=bytearray(m.u.mem_read(m.addr(name),512));data[510:512]=struct.pack('<H',crc16(data[:510]))
        m.flash[0x1cd400+off:0x1cd600+off]=data
    backup=bytes(m.flash[0x1a000:0x33000]);m.record_changes=True;m.begin()
    boot=FirmwareMachine();ram=bytes(boot.u.mem_read(0x20000000,0x5000));seen=count=0
    for _ in range(3000):
        if not m.ptr():break
        before=bytes(m.flash);m.tick_fw()
        if len(m.changes)==seen:continue
        seen=len(m.changes);kind,a,data=m.changes[-1]
        images=[bytes(m.flash)]
        for cut in (1,len(data)//2,len(data)-1):
            torn=bytearray(before);torn[a:a+cut]=m.flash[a:a+cut];images.append(bytes(torn))
        for flash in images:
            boot.u.mem_write(0x20000000,ram);boot.flash=bytearray(flash)
            selected=boot.checked('cfg_select',0x21000000)
            cfg=flash[selected:selected+1024]
            assert crc16(cfg[:510])==int.from_bytes(cfg[510:512],'little')
            assert crc16(cfg[512:1022])==int.from_bytes(cfg[1022:1024],'little')
            assert flash[0x1a000:0x33000]==backup,'rollback source destroyed'
            assert flash[0x1000:0x1004]==b'\xff'*4,'incomplete image got install marker'
            assert not boot.read_word(boot.device+171,1),'reset resumed unverified install'
            # Copy destruction is strictly after durable command19 success.
            if 0x1000<=a<0x1a000:assert m.sent and m.sent[0][10]==0
            count+=1
    else:raise AssertionError('power fixture stuck')
    assert m.read_word(m.device+171,1)==1
    return count


def real_spi():
    from unicorn import UC_HOOK_MEM_WRITE
    m=FirmwareMachine();m.fixture();m.write(0x40013000,0x40,2)
    wire=dict(cmd=0,address=0,address_bytes=0,wel=0,busy=0,read=0,status=0)
    for name in ('w25_read_data','w25_async_start','w25_async_poll','w25_async_active','w25_read_available'):
        del m.stubs[m.addr(name)]
    def cs(u,access,a,n,value,unused):
        if a==0x40010c14 and value==4096:wire.update(cmd=0,address=0,address_bytes=0)
        if a==0x40010c10 and value==4096:
            if wire['cmd']==6:wire['wel']=2
            elif wire['cmd']==2:wire.update(wel=0,busy=3)
            elif wire['cmd']==0x20:
                assert wire['wel'] and not wire['address']&4095
                a=wire['address'];m.flash[a:a+4096]=b'\xff'*4096;wire.update(wel=0,busy=3)
    def send_byte():
        b=m.args()[0]&255
        if not wire['cmd']:wire['cmd']=b
        elif wire['cmd'] in (2,3,0x20) and wire['address_bytes']<3:
            wire['address']=(wire['address']<<8)|b;wire['address_bytes']+=1
        m.ret()
    def read_byte():
        assert wire['cmd']==5
        wire['status']+=1;v=wire['wel']|bool(wire['busy'])
        if wire['busy']:wire['busy']-=1
        m.ret(v)
    def send():
        p,n,_,_=m.args();assert wire['cmd']==2 and wire['wel'] and n<=64
        for b in m.u.mem_read(p,n):m.flash[wire['address']]&=b;wire['address']+=1
        m.ret()
    def read():
        p,n,_,_=m.args();assert wire['cmd']==3 and n<=256
        a=wire['address'];m.u.mem_write(p,bytes(m.flash[a:a+n]));wire['address']+=n;wire['read']+=n;m.ret()
    m.u.hook_add(UC_HOOK_MEM_WRITE,cs,begin=0x40010c10,end=0x40010c14)
    for name,fn in [('spi_send_byte',send_byte),('spi_read_byte',read_byte),('spi_send',send),('spi_read',read)]:m.stubs[m.addr(name)]=fn
    m.begin();m.costs=[];mxread=mxstatus=fwread=0
    for n in range(10000):
        if not m.ptr():break
        active=m.checked('fw_service_active');wire['read']=wire['status']=0;m.tick_fw()
        mxread=max(mxread,wire['read']);mxstatus=max(mxstatus,wire['status'])
        if active:fwread=max(fwread,wire['read']);assert wire['read']<=64
        assert wire['status']<=4
    else:raise AssertionError('real SPI stuck')
    assert m.flash[0x1200:0x1200+len(m.image)]==m.image and m.sent[0][10]==0
    assert m.read_word(m.device+171,1)==1
    return dict(polls=n,max_poll_instructions=max(m.poll_costs),total_poll_instructions=sum(m.poll_costs),max_instructions=max(m.costs),total_instructions=sum(m.costs),stack=m.stack_peak,spi_read_max=mxread,fw_spi_read_max=fwread,spi_status_max=mxstatus,phase_maximum=m.phase_costs)

def exact_failure_logs():
    cases=[(1,1,'read',None),(1,2,'read',(133,7)),(2,4,'start',None),
           (1,11,'read',(133,30)),(1,13,'read',(133,8)),(1,14,'read',(133,8))]
    for number,phase,kind,expected in cases:
        m=FirmwareMachine();m.fixture();m.begin(number);m.until(lambda:m.phase()==phase)
        old=len(m.logs);m.fault=(kind,m.counts[kind]+1);m.drain()
        assert m.logs[old:]==([] if expected is None else [expected]),(number,phase,m.logs[old:],expected)
        assert not m.read_word(m.device+171,1)
    return len(cases)

if __name__=='__main__':
    r={'normal':normal(),'heap':heap_reservation(),'boundaries':boundaries_errors(),'io_faults':io_faults(),'busy_cancel':busy_cancel(),'power_cuts':power_cuts(),'real_spi':real_spi(),'exact_failure_logs':exact_failure_logs()}
    r['elf_sha256']=hashlib.sha256((ROOT/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest();r['status']='PASS'
    (ROOT/'Tests/asm/fw_service_runtime_results.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r,indent=2))
