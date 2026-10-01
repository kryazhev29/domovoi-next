"""B31: реальная инициализация PVD/EXTI16 и ISR, без физического питания."""
import re
from iar_irq_support import Machine

m=Machine();m.init();line=1<<16
m.u.mem_map(0x42000000,0x600000)  # алиасы bit-band GPIO в board_init
m.call('board_init')
assert m.read(0x40010400)&line, 'B31 EXTI16 interrupt disabled'
assert m.read(0x40010408)&line and not m.read(0x4001040C)&line, 'B31 must trigger on PVDO assertion only'
assert m.read(0xE000E100)&2, 'B31 NVIC PVD disabled'
resets=[];clears=[]
def reset():resets.append(1);m.ret()
for value in re.findall(r"^NVIC_SystemReset\s+0x([0-9a-f']+)",m.maptext,re.M):
    m.stubs[int(value.replace("'",''),16)&~1]=reset
def clear():
    clears.append(1);m.write(0x40010414,0);m.ret()
m.stubs[m.addr('EXTI_ClearITPendingBit')]=clear
for pending,low,expected in ((0,1,0),(1,0,0),(1,1,1)):
    resets.clear();clears.clear();m.write(0x40010414,line if pending else 0);m.write(0x40007004,4 if low else 0)
    m.call('PVD_IRQHandler')
    assert len(resets)==expected and len(clears)==pending
print('B31 IAR: EXTI16 rising PVDO, low-voltage reset, pending acknowledged, no spurious reset PASS')
