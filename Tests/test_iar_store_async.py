"""Final IAR W25 async machine, modeled SPI/NOR; counts are NOT board timings."""
from iar_irq_support import Machine, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2
from unicorn import UC_HOOK_MEM_WRITE
m=Machine();m.init();m.write(0x40013000,0x40,2)
flash=bytearray(b"\xff"*8192)
cmd=address=wel=busy=reads=status_reads=0
address_bytes=0
maximum=0
def cs(u,access,a,n,value,_):
 global cmd,address,address_bytes,wel,busy
 if a==0x40010c14 and value==4096:cmd=address=address_bytes=0
 if a==0x40010c10 and value==4096:
  if cmd==6:wel=2
  elif cmd==2:wel=0;busy=3
  elif cmd==0x20:
   assert wel and not address&4095
   flash[address:address+4096]=b"\xff"*4096;wel=0;busy=3
def send_byte():
 global cmd,address,address_bytes
 x=m.u.reg_read(UC_ARM_REG_R0)&255
 if not cmd:cmd=x
 elif cmd in (2,3,0x20) and address_bytes<3:address=(address<<8)|x;address_bytes+=1
 m.ret(0)
def read_byte():
 global busy,status_reads
 assert cmd==5
 status_reads+=1
 v=wel|(1 if busy else 0)
 if busy:busy-=1
 m.ret(v)
def send():
 global address
 p,n=m.u.reg_read(UC_ARM_REG_R0),m.u.reg_read(UC_ARM_REG_R1)
 assert cmd==2 and wel and n<=64
 for b in m.u.mem_read(p,n):flash[address]&=b;address+=1
 m.ret()
def read():
 global reads,address
 p,n=m.u.reg_read(UC_ARM_REG_R0),m.u.reg_read(UC_ARM_REG_R1)
 assert cmd==3 and n<=64
 m.u.mem_write(p,bytes(flash[address:address+n]));address+=n;reads+=n;m.ret()
m.u.hook_add(UC_HOOK_MEM_WRITE,cs)
for name,fn in (("spi_send_byte",send_byte),("spi_read_byte",read_byte),("spi_send",send),("spi_read",read)):
 m.stubs[m.addr(name)]=fn
m.write(m.addr('store_runtime'),1,1)
p=0x21000000;m.u.mem_write(p,b"Z"*64)
assert m.call("w25_async_start",16,p,64,100)
assert not m.call("w25_async_start",4096,0,0,100)
for i in range(4):
 reads=status_reads=0;r=m.call("w25_async_poll",101+i);maximum=max(maximum,m.steps)
 assert r==(2 if i==3 else 1) and reads<=64 and status_reads<=2
assert flash[16:80]==b"Z"*64
flash[:4096]=b"\0"*4096
assert m.call("w25_async_start",0,0,0,200)
for i in range(67):
 reads=status_reads=0;r=m.call("w25_async_poll",201+i);maximum=max(maximum,m.steps)
 assert r==(2 if i==66 else 1) and reads<=64 and status_reads<=2
assert flash[:4096]==b"\xff"*4096
assert m.call("w25_async_start",0,p,64,0xfffffff0)
busy=999
assert m.call("w25_async_poll",3)==1
assert m.call("w25_async_poll",4)==3
reads=status_reads=0
assert not m.call('w25_read_data',0,p,64,budget=5000)
assert status_reads==1 and reads==0 and not m.call('w25_async_active')
busy=0;reads=status_reads=0
assert m.call('w25_read_data',0,p,64) and reads==64 and status_reads==1
assert maximum<2000,maximum
print(f"IAR W25 async PASS: BUSY, ownership, write/erase, readback <=64 bytes, status <=2, timeout/wrap; maximum {maximum} modeled instructions per poll, NOT board time")
