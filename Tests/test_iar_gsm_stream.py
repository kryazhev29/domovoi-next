"""Реальный gsm_read и аллокатор IAR: heap остаётся ограниченным при разрывах URC."""
from iar_irq_support import Machine,UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R3
m=Machine();m.init()
for n in ('gsm_user_debug','gsm_disconnect','gsm_readsms','gsm_runcmd'):
    if n in m.symbols:m.stubs[m.addr(n)]=lambda:m.ret()
seen=bytearray();events=[];input_start=0x21000000;input_end=input_start
def received():
    op=m.u.reg_read(UC_ARM_REG_R0);events.append(op)
    if op==16:
        p=m.u.reg_read(UC_ARM_REG_R1);n=m.u.reg_read(UC_ARM_REG_R3)
        assert input_start<=p<=input_end and n<=input_end-p,'modem payload outside input'
        seen.extend(m.u.mem_read(p,n))
    m.ret()
m.stubs[m.addr('gsm_dataread')]=received
m.call('gsm_initvar',0,1)
uart=m.addr('uart')+2*m.map_size('uart')//3
m.u.mem_write(uart+m.members['rxbuffer'],input_start.to_bytes(4,'little'))
m.u.mem_write(uart+m.members['bfsize'],m.map_size('gsm_buffer').to_bytes(2,'little'))
max_stack=0
def feed(packet):
    global input_end,max_stack
    input_end=input_start+len(packet);m.u.mem_write(input_start,packet)
    if 'gsm_read' in m.symbols:m.call('gsm_read',input_start,len(packet))
    else:
        m.u.mem_write(uart+m.members['received'],len(packet).to_bytes(2,'little'))
        m.call('gsm_read_uart')
    max_stack=max(max_stack,m.top-m.minimum)
def largest():
    lo,hi=0,m.heap_bytes
    while lo<hi:
        mid=(lo+hi+1)//2;p=m.call('__basic_malloc',mid)
        if p:m.call('__basic_free',p);lo=mid
        else:hi=mid-1
    return lo
cold=largest()
# Давление на heap двумя блоками прежнего размера UART/stash; в прошивке их уже нет.
copies=[m.call('__basic_malloc',2056),m.call('__basic_malloc',2056)]
assert all(copies)
body=b'A\0ERROR RING CLOSED OK';packet=b'+QIURC: "recv",0,'+str(len(body)).encode()+b'\r\n'+body+b'+IPD,3:xyz\r\nOK\r\n'
for ch in packet:feed(bytes([ch]))
assert seen==body+b'xyz' and 37 not in events and 3 not in events and 15 not in events
for p in copies:m.call('__basic_free',p)
assert largest()==cold,'parser leaks heap after completed lines/data'
# Отказ роста незавершённой строки явно восстанавливает RX, а не оставляет утечку.
held=m.call('__basic_malloc',cold-96);assert held
feed(b'x'*128)
assert 15 in events
m.call('__basic_free',held)
assert largest()==cold
print(f'IAR gsm_read PASS: byte splits, binary tokens/NUL, UART+stash allocated, OOM recovery, no leak; executed parser stack {max_stack} bytes (peripheral callbacks stubbed)')
