"""URC остаётся в UART, AT ищет только новый ответ; настоящий parser и heap IAR."""
import struct
from iar_irq_support import Machine,UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R3
from iar_layout import fields
from unicorn import UC_HOOK_CODE
m=Machine();m.init();buf=m.addr('gsm_buffer');uart=m.addr('uart')+2*m.map_size('uart')//3;f=m.members
def count():return struct.unpack('<H',m.u.mem_read(uart+f['received'],2))[0]
def append(raw):
    n=count();assert n+len(raw)<=m.map_size('gsm_buffer')
    m.u.mem_write(buf+n,raw);m.u.mem_write(uart+f['received'],struct.pack('<H',n+len(raw)))
    m.u.mem_write(uart+f['state'],b'\1')
m.u.mem_write(uart+f['rxbuffer'],struct.pack('<I',buf));m.u.mem_write(uart+f['bfsize'],struct.pack('<H',m.map_size('gsm_buffer')))
requests=[];got=[];closed=[];clock=[0];answers=[b'+IPD,3:C\0D\r\nOK\r\n',b'\r\n+CREG: 0,1\r\nOK\r\n',b'']
def observe(u,a,n,_):
    if a==m.addr('__basic_calloc'):requests.append(u.reg_read(UC_ARM_REG_R0)*u.reg_read(UC_ARM_REG_R1))
m.u.hook_add(UC_HOOK_CODE,observe)
def transmit():append(answers.pop(0));m.ret()
def millis():clock[0]+=1;m.ret(clock[0])
def callback():
    if m.u.reg_read(UC_ARM_REG_R0)==16:got.append(bytes(m.u.mem_read(m.u.reg_read(UC_ARM_REG_R1),m.u.reg_read(UC_ARM_REG_R3))))
    if m.u.reg_read(UC_ARM_REG_R0)==15:closed.append(1)
    m.ret()
for name in ('gsm_user_debug','wdt_reload','timers_pause','server_rx_process'):m.stubs[m.addr(name)]=lambda:m.ret()
m.stubs[m.addr('uart_sendstr')]=transmit;m.stubs[m.addr('timers_getMillis')]=millis;m.stubs[m.addr('gsm_dataread')]=callback
append(b'+IPD,2:AB');m.u.mem_write(0x21000000,b'AT\r\0OK\0')
assert m.call('gsm_runcmd',0x21000000,0x21000004)
assert m.call('gsm_runcmd',0x21000000,0x21000004)
assert not requests,('URC stash still allocated',requests)
start=clock[0]
assert not m.call('gsm_runcmd',0x21000000,0x21000004),'previous OK accepted as new reply'
assert clock[0]-start>=500,'old RX caused immediate refusal before timeout'
m.call('gsm_check_data',budget=3000000)
assert b''.join(got)==b'ABC\0D',(got,count())
assert not count()
append(b'+IPD,65535:'+b'x'*(m.map_size('gsm_buffer')-11))
m.u.mem_write(0x40004800,struct.pack('<I',0x20));m.u.mem_write(0x4000480c,struct.pack('<I',0x20));m.u.mem_write(0x40004804,b'Z\0\0\0')
m.call('USART3_IRQHandler');m.u.mem_write(uart+f['state'],b'\1')
m.call('gsm_check_data',budget=3000000)
assert closed and not count(),'overflowed UART stream was silently continued'
print('No stash: two URCs across ATs, embedded NUL, stale OK rejected, real parser PASS')
