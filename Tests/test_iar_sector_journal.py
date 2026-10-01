"""B24: машинный код IAR приложения/загрузчика, настоящий heap; модель NOR, без платы."""
import struct, zlib
from iar_irq_support import Machine, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2

TARGET, DATA, META = 0x41000, 0x1F9000, 0x1FA000
MAGIC, COMMIT = 0x314A4353, 0x314B4F53

class Fixture:
    def __init__(self, boot=False):
        self.m=Machine(boot=boot);self.flash=bytearray(b'\xff'*0x200000)
        self.failure=False;self.operations=0;self.validated=0;self.services=0
        for name, fn in [(('storage_read' if boot else 'w25_read_data'),self.read),
                         (('storage_write' if boot else 'w25_write_data'),self.write),
                         (('storage_erase' if boot else 'w25_erase_sector'),self.erase)]:
            self.m.stubs[self.m.addr(name)]=fn
        if boot:
            self.m.stubs[self.m.addr('boot_service')]=self.service
            self.m.stubs[self.m.addr('boot_validate')]=self.validate
        else:
            if 'user_wait_stable_power' in self.m.symbols:
                self.m.stubs[self.m.addr('user_wait_stable_power')]=lambda:self.m.ret()
            self.m.stubs[self.m.addr('wdt_reload')]=self.service
        self.m.init()
    def args(self):return [self.m.u.reg_read(r) for r in (UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2)]
    def read(self):
        a,p,n=self.args();assert a+n<=len(self.flash)
        if self.failure:self.m.ret(0);return
        self.m.u.mem_write(p,bytes(self.flash[a:a+n]));self.m.ret(1)
    def write(self):
        a,p,n=self.args();assert a+n<=len(self.flash) and n<=256 and (a&255)+n<=256
        payload=self.m.u.mem_read(p,n)
        for i,v in enumerate(payload):self.flash[a+i]&=v
        self.operations+=1;self.m.ret(1)
    def erase(self):
        a=self.args()[0];assert a%4096==0 and a+4096<=len(self.flash)
        self.flash[a:a+4096]=b'\xff'*4096;self.operations+=1;self.m.ret(1)
    def service(self):self.services+=1;self.m.ret()
    def validate(self):self.validated+=1;self.m.ret(0)
    def call(self,name,*args):return self.m.call(name,*args,budget=14000000)
    def publish(self,payload):
        crc=zlib.crc32(payload)
        header=struct.pack('<6I',MAGIC,1,TARGET,4096,crc,crc^0xffffffff)
        header+=struct.pack('<2I',zlib.crc32(header),COMMIT)
        self.flash[DATA:DATA+4096]=payload;self.flash[META:META+32]=header
        self.flash[TARGET:TARGET+4096]=b'\xff'*2000+payload[2000:]
    def check(self,payload):
        assert self.flash[TARGET:TARGET+4096]==payload
        assert self.flash[META:META+4096]==b'\xff'*4096
        assert self.flash[TARGET-4096:TARGET]==b'\xff'*4096
        assert self.flash[TARGET+4096:TARGET+8192]==b'\xff'*4096
        assert self.services>0

def largest(m):
    lo,hi=0,m.heap_bytes
    while lo<hi:
        mid=(lo+hi+1)//2;p=m.call('__basic_malloc',mid)
        if p:m.call('__basic_free',p);lo=mid
        else:hi=mid-1
    return lo

payload=bytes((i*17+23)&255 for i in range(4096))
for boot in (False,True):
    f=Fixture(boot);f.publish(payload)
    assert f.call('sj_recover');peak=f.m.top-f.m.minimum;f.check(payload)
    f.m.init();assert f.call('sj_recover') and f.operations==18
    if boot:
        f.call('boot_start_app');assert f.validated==1
        f.publish(payload);f.flash[DATA+255]^=1;f.validated=f.operations=0
        f.call('boot_start_app');assert not f.validated and not f.operations
        f.failure=True;f.call('boot_start_app');assert not f.validated
        assert not f.call('boot_ext_erase',DATA)
        assert not f.call('boot_ext_write',DATA,0x21000000,32)
        assert not f.call('sj_io_read',0xffffffff,0x21000000,2)
    else:
        f.publish(payload);f.flash[DATA+255]^=1;f.operations=0
        assert not f.call('mem_recover')
        assert not f.call('mem_read',TARGET,0x21000000,32)
        f.m.u.mem_write(0x21000000,b'1234567890\0')
        f.call('user_checkIdAndControl',0x21000000,0)
        assert not f.operations
    print(f"IAR {'boot' if boot else 'app'}: confirmed recovery, restart, corrupt scratch guard PASS; tested stack {peak} bytes")

f=Fixture();m=f.m
base=bytearray(payload);base[:2]=b'\x07\x00'
f.flash[TARGET:TARGET+4096]=base
cell=bytearray(base[:32]);cell[22]^=0x21
m.u.mem_write(0x21000000,bytes(cell));m.write(m.addr('mem')+2*8,TARGET);m.write(m.addr('mem')+2*8+4,64)
cold=largest(m);assert cold>=4096
assert f.call('user_updateID',0x21000000);peak=m.top-m.minimum
expected=bytes(cell)+bytes(base[32:]);f.check(expected)
assert largest(m)==cold,'user_updateID leaked heap'
held=m.call('__basic_malloc',cold);assert held
f.operations=0;assert not f.call('user_updateID',0x21000000) and not f.operations
m.call('__basic_free',held);assert largest(m)==cold
print(f'IAR app: real user_updateID/4096 allocation/OOM/free PASS; tested stack {peak} bytes; heap reserve {m.heap_bytes} bytes')
