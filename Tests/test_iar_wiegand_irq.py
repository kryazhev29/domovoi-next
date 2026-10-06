"""B38: EXTI D1 между всеми инструкциями передачи Wiegand, свежий ELF IAR."""
from iar_irq_support import Machine,UC_ARM_REG_PRIMASK
m=Machine();seed=0x0123457
def setup():
    m.init();m.write(m.addr('wgData'),seed);m.write(m.addr('wgTime'),100);m.write(m.addr('wgBitCount'),26,1);m.write(m.addr('milliSeconds'),400)
setup();m.call('WG_process');steps=m.steps;max_stack=0
for at in range(steps):
    setup();m.call('WG_process',inject_at=at,irq=('EXTI3_IRQHandler',()))
    count=m.read(m.addr('wgBitCount'),1);available=m.read(m.addr('wgDataAvalible'),1)
    assert count in (1,27),('lost EXTI edge during snapshot/reset',at,count)
    if available:
        assert count==1 and m.read(m.addr('wgData'))==1,('mixed snapshot/new edge',at,count)
        assert m.read(m.addr('wgID'))==((seed>>1)&0x03ffffff),('mixed Wiegand snapshot',at)
    else:assert count==27 and m.read(m.addr('wgData'))==((seed<<1)|1)
    assert m.u.reg_read(UC_ARM_REG_PRIMASK)==0
    max_stack=max(max_stack,m.top-m.minimum)
setup();m.u.reg_write(UC_ARM_REG_PRIMASK,1);m.call('WG_process');assert m.u.reg_read(UC_ARM_REG_PRIMASK)==1
m.init()
for i in range(282):m.call('EXTI3_IRQHandler')
assert m.read(m.addr('wgBitCount'),1)!=26
print(f'IAR Wiegand IRQ PASS: EXTI at {steps} instruction boundaries, no lost/mixed edge, saturation and PRIMASK; executed stack with simulated IRQ {max_stack} bytes')
