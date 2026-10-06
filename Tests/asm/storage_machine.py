"""Real final IAR machine, modeled NOR boundary, real allocator and caller ABI."""
import sys,struct,re,os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'Tests'))
from iar_irq_support import *
from iar_layout import fields
from functools import lru_cache

@lru_cache(None)
def layout(module,name):
    with (Path(os.environ.get('DOMOVOI_LAYOUT_OBJECTS',ROOT/'Release/task-asm-storage/export'))/f'{module}.o').open('rb') as f:
        e=ELFFile(f)
        for cu in e.get_dwarf_info().iter_CUs():
            for die in cu.iter_DIEs():
                a=die.attributes.get('DW_AT_name')
                if die.tag!='DW_TAG_variable' or not a or a.value!=name.encode():continue
                typ=die.get_DIE_from_attribute('DW_AT_type');out={}
                for member in typ.iter_children():
                    if 'DW_AT_name' not in member.attributes:continue
                    off=member.attributes['DW_AT_data_member_location'].value
                    if not isinstance(off,int):off=DWARFExprParser(cu.structs).parse_expr(off)[0].args[0]
                    out[member.attributes['DW_AT_name'].value.decode()]=off
                if out:return out
    raise AssertionError((module,name))

def crc16(b,c=65535):
    for x in b:
        c^=x
        for _ in range(8):c=(c>>1)^(0xa001 if c&1 else 0)
    return c

class StorageMachine(Machine):
    def hook(self,u,a,n,unused):
        if a in self.stubs:
            assert not u.reg_read(UC_ARM_REG_SP)&7,('AAPCS call alignment',hex(a))
        super().hook(u,a,n,unused)
    def __init__(self,flash=None):
        super().__init__();self.init()
        self.flash=bytearray(b'\xff'*0x400000) if flash is None else bytearray(flash)
        self.now=0;self.io=None;self.changes=[];self.calls=[];self.answers=[];self.timestamps=[]
        self.counts={'read':0,'start':0,'poll':0};self.fault=None;self.io_budget=0;self.read_budget=0
        self.costs=[];self.stack_peak=0;self.limit_reads=128;self.record_changes=False
        self.d=fields('device_s');self.device=self.addr('device')
        for name,fn in [('w25_read_data',self.read),('w25_async_start',self.start),
          ('w25_async_poll',self.poll),('w25_async_active',lambda:self.ret(self.io is not None)),
          ('w25_read_available',lambda:self.ret(self.io is None)),
          ('w25_write_data',self.sync_write),('w25_erase_sector',self.sync_erase),
          ('timers_getMillis',lambda:self.ret(self.now)),('wdt_reload',self.ret),
          ('user_wait_stable_power',self.ret),('comm_tx_busy',lambda:self.ret(0)),
          ('user_send_answer_comand',self.answer),('user_send_timeshtamp',self.timestamp),
          ('user_debug',self.ret)]:
            self.stubs[self.addr(name)]=fn
        self.write(self.addr('store_runtime'),1,1);self.write(self.addr('mem_storage_ready'),1,1)
        self.write(self.device+self.d['userAutorized'],1,1)
        self.write(self.device+self.d['memID'],0x17c2,2)
        for mb,base,size in [(2,0x41000,8192),(3,0x1c9000,2048),(4,0x1ca800,2048),(5,0x1cc000,1024),
                             (7,0x1d0000,98304),(8,0x1e8000,53248),(9,0x200000,0x100000)]:
            self.u.mem_write(self.addr('mem')+mb*8,struct.pack('<II',base,size))
    def args(self):return [self.u.reg_read(r) for r in (UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3)]
    def failed(self,kind):
        self.counts[kind]+=1
        return self.fault==(kind,self.counts[kind])
    def read(self):
        a,p,n,_=self.args();self.read_budget+=n
        assert 0<n<=256 and a+n<=len(self.flash),(hex(a),n)
        if self.failed('read'):self.ret(0);return
        self.u.mem_write(p,bytes(self.flash[a:a+n]));self.ret(1)
    def change(self,kind,a,b):
        if self.record_changes:self.changes.append((kind,a,bytes(b)))
        if kind=='erase':self.flash[a:a+4096]=b'\xff'*4096
        else:
            for i,x in enumerate(b):self.flash[a+i]&=x
    def start(self):
        a,p,n,now=self.args();self.io_budget+=1
        assert self.io is None and a<len(self.flash)
        assert (p and 0<n<=64 and (a&255)+n<=256) or (not p and not a%4096)
        if self.failed('start'):self.ret(0);return
        data=bytes(self.u.mem_read(p,n)) if p else b'\xff'*4096
        self.change('write' if p else 'erase',a,data)
        self.io=(a,p,n,data,0);self.ret(1)
    def poll(self):
        self.io_budget+=1;assert self.io is not None
        a,p,n,data,step=self.io
        if p:assert bytes(self.u.mem_read(p,n))==data,'in-flight buffer reused'
        if self.failed('poll'):self.io=None;self.ret(3);return
        if step<2:self.io=(a,p,n,data,step+1);self.ret(1);return
        assert self.flash[a:a+len(data)]==data,'verify failed'
        self.io=None;self.ret(2)
    def sync_write(self):
        a,p,n,_=self.args();assert self.io is None
        self.change('write',a,self.u.mem_read(p,n));self.ret(1)
    def sync_erase(self):
        a=self.args()[0];assert self.io is None
        self.change('erase',a,b'\xff'*4096);self.ret(1)
    def answer(self):self.answers.append(tuple(self.args()));self.ret()
    def timestamp(self):
        extra=int.from_bytes(self.u.mem_read(self.u.reg_read(UC_ARM_REG_SP),4),'little')
        self.timestamps.append(tuple(self.args())+(extra,));self.ret()
    def checked(self,name,*args,budget=100000):
        saved=(UC_ARM_REG_R4,UC_ARM_REG_R5,UC_ARM_REG_R6,UC_ARM_REG_R7,UC_ARM_REG_R8,UC_ARM_REG_R9,UC_ARM_REG_R10,UC_ARM_REG_R11)
        values=[0x13570000+i*123 for i in range(8)]
        for r,v in zip(saved,values):self.u.reg_write(r,v)
        self.read_budget=self.io_budget=0
        r=self.call(name,*args,budget=budget)
        assert [self.u.reg_read(x) for x in saved]==values,(name,'AAPCS')
        assert self.u.reg_read(UC_ARM_REG_SP)==self.top,(name,'stack balance')
        self.stack_peak=max(self.stack_peak,self.top-self.minimum)
        self.costs.append(self.steps);return r
    def command(self,cmd,rows,count,channel=0,uniq=7,stamp=123456):
        p=struct.pack('<HBBBIB',8+len(rows),0,uniq,cmd,stamp,count)+rows
        p+=struct.pack('<H',crc16(p));self.u.mem_write(0x21000000,p)
        self.checked('user_parse_comand',0x21000000,channel)
        self.u.mem_write(0x21000000,b'\xA5'*len(p)) # receiver immediately reuses it
    def active(self):return self.call('table_runtime_active')!=0
    def tick(self):
        crc_phase = bytes(self.u.mem_read(self.addr('table_job')+50,1)) == b'\x03'
        self.now+=1;self.checked('user_storage_poll')
        assert self.read_budget <= max(self.limit_reads,256 if crc_phase else 0),('read budget',self.read_budget)
        assert self.io_budget<=1,('one async action',self.io_budget)
    def drain(self,limit=10000):
        for _ in range(limit):
            if not self.active():
                self.settle_crc();return
            self.tick()
        raise AssertionError('operation did not terminate')

    def settle_crc(self):
        if "idcrc_io_active" not in self.symbols:return
        for _ in range(1000):
            if not self.checked("idcrc_io_active"):return
            self.now+=1;self.checked("idcrc_background")
        raise AssertionError("CRC metadata did not finish")
