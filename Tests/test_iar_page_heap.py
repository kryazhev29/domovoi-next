"""Страницы вместо сектора: настоящий код IAR, NOR и контроль heap/стека."""
import struct,re
from iar_irq_support import Machine,UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2
from unicorn import UC_HOOK_CODE

m=Machine();m.init();flash=bytearray(b'\xff'*0x200000);requests=[];services=[]
def args():return [m.u.reg_read(r) for r in (UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2)]
def read():
    a,p,n=args();assert a+n<=len(flash);m.u.mem_write(p,bytes(flash[a:a+n]));m.ret(1)
def write():
    a,p,n=args();assert a+n<=len(flash) and n<=256 and (a&255)+n<=256
    for i,v in enumerate(m.u.mem_read(p,n)):flash[a+i]&=v
    m.ret(1)
def erase():
    a=args()[0];assert a%4096==0;flash[a:a+4096]=b'\xff'*4096;m.ret(1)
def observe(u,a,n,_):
    if a==m.addr('__basic_malloc'):
        request=u.reg_read(UC_ARM_REG_R0);requests.append(request)
        assert request<=256,('sector heap allocation',request)
m.u.hook_add(UC_HOOK_CODE,observe)
for name,fn in (('w25_read_data',read),('w25_write_data',write),('w25_erase_sector',erase),('user_wait_stable_power',lambda:m.ret()),('wdt_reload',lambda:(services.append(1),m.ret()))):
    if name in m.symbols:m.stubs[m.addr(name)]=fn
mem=m.addr('mem');m.u.mem_write(mem+6*8,struct.pack('<II',0x1cd400,4096));m.u.mem_write(mem+2*8,struct.pack('<II',0x41000,4096))
tail=bytes((i*17+23)&255 for i in range(3072));flash[0x1cd800:0x1ce400]=tail
assert m.call('user_writeConfigAndDevice',budget=16000000)
assert flash[0x1cd800:0x1ce400]==tail
config=bytes(m.u.mem_read(m.addr('config'),m.map_size('config')))
device=bytes(m.u.mem_read(m.addr('device'),m.map_size('device')))
assert flash[0x1cd400:0x1cd400+len(config)]==config
assert flash[0x1cd400+len(config):0x1cd400+len(config)+len(device)]==device
used=m.top-m.minimum
cell=bytearray(32);cell[:2]=b'\x07\0';cell[12]=1;flash[0x41000:0x41020]=cell;cell[22]^=1;m.u.mem_write(0x21000000,bytes(cell))
assert m.call('user_updateID',0x21000000,budget=16000000)
used=max(used,m.top-m.minimum)
assert not requests,requests
irq=int(re.search(r"^\s+interrupt\s+[0-9']+\s+([0-9']+)",m.maptext,re.M)[1].replace("'",''))
assert services and used+irq<=m.top-m.bottom,('stack with IRQ estimate',used,irq)
print('Page rewrite: real config/ID, heap requests',requests,'stack max tested',used,'IRQ map estimate',irq,'PASS')
