"""Настоящий gsm_check_data/gsm_read: бинарный RX, heap IAR, хвост ISR."""
import struct
from iar_irq_support import Machine,UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R3
from iar_layout import fields
from unicorn import UC_HOOK_CODE

m=Machine();m.init();buf=m.map_addr('gsm_buffer');capacity=m.map_size('gsm_buffer')
uart=m.addr('uart')+2*(m.map_size('uart')//3);f=m.members;g=m.addr('GSM');gf=fields('GsmStr')
m.u.mem_write(uart+f['rxbuffer'],struct.pack('<I',buf))
m.u.mem_write(uart+f['bfsize'],struct.pack('<H',capacity))
m.u.mem_write(uart+f['state'],b'\1');m.u.mem_write(g+gf['operation'],b'\0')
payload=bytes((i*17)&255 for i in range(1900));raw=b'+IPD,1900:'+payload
m.u.mem_write(buf,raw);m.u.mem_write(uart+f['received'],struct.pack('<H',len(raw)))
requests=[];got=[];tail=b'+IPD,2:XY'
def observe(u,a,n,_):
    if a==m.addr('__basic_malloc'):requests.append(u.reg_read(UC_ARM_REG_R0))
    if a==m.addr('__basic_calloc'):requests.append(u.reg_read(UC_ARM_REG_R0)*u.reg_read(UC_ARM_REG_R1))
m.u.hook_add(UC_HOOK_CODE,observe)
def callback():
    p=m.u.reg_read(UC_ARM_REG_R1);n=m.u.reg_read(UC_ARM_REG_R3)
    if n:
        got.append(bytes(m.u.mem_read(p,n)))
        # Модель ISR: дописывает только после опубликованного received.
        count=struct.unpack('<H',m.u.mem_read(uart+f['received'],2))[0]
        m.u.mem_write(buf+count,tail);m.u.mem_write(uart+f['received'],struct.pack('<H',count+len(tail)))
    m.ret()
m.stubs[m.addr('gsm_dataread')]=callback
m.stubs[m.addr('server_rx_process')]=lambda:m.ret()
m.call('gsm_check_data',budget=3000000)
assert got==[payload],(len(got),[len(x) for x in got])
assert max(requests,default=0)<=128,('UART copy allocated',requests)
assert bytes(m.u.mem_read(buf,len(tail)))==tail
assert struct.unpack('<H',m.u.mem_read(uart+f['received'],2))[0]==len(tail)
print('UART inplace: 1900 binary bytes, IAR allocations <=128, ISR tail retained PASS')
