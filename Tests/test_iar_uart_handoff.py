"""B19: каждая граница инструкции снимка + RX, настоящий код свежего IAR ELF."""
from iar_irq_support import Machine,UC_ARM_REG_PRIMASK
m=Machine();out=0x21000000
base,rx=m.uart(b'abcdef')
assert m.call('uart_takerx',0,out,3)==3
steps=m.steps;max_stack=0
for at in range(steps):
    base,rx=m.uart(b'abcdef')
    assert m.call('uart_takerx',0,out,3,inject_at=at,irq=('uart_addbyte',(0,ord('X'))))==3
    assert bytes(m.u.mem_read(out,3))==b'abc'
    assert m.read(base+m.members['received'],2)==4
    assert bytes(m.u.mem_read(rx,4))==b'defX',at
    assert m.u.reg_read(UC_ARM_REG_PRIMASK)==0
    max_stack=max(max_stack,m.top-m.minimum)
base,rx=m.uart(b'a'*2048)
assert m.call('uart_takerx',0,out,2048)==2048
assert m.read(base+m.members['received'],2)==0
base,rx=m.uart(b'abc');m.u.reg_write(UC_ARM_REG_PRIMASK,1)
assert m.call('uart_takerx',0,out,2)==2 and m.u.reg_read(UC_ARM_REG_PRIMASK)==1
print(f'IAR UART handoff PASS: IRQ at {steps} instruction boundaries, full 2048-byte snapshot, PRIMASK restored; executed stack with simulated IRQ {max_stack} bytes')
