"""B31: реальная инициализация PVD/EXTI16 и ISR, без физического питания."""
import re, sys
from iar_irq_support import Machine
from unicorn.arm_const import UC_ARM_REG_R0
from unicorn import UC_HOOK_MEM_WRITE

m=Machine();m.init();line=1<<16
m.u.mem_map(0x42000000,0x600000)  # алиасы bit-band GPIO в board_init
m.call('board_init')
assert m.read(0x4002101C)&0x18000000==0x18000000, 'PVD marker requires PWR/BKP clocks'
# Unicorn хранит bit-band alias отдельно от основного регистра; проверяем адреса записи SPL.
assert m.read(0x40007000)&0x100, 'PVD marker requires backup write access'
assert m.read(0x40007000)&0xE0==0x60 and m.read(0x40007000)&0x10, 'PVD must enable 2.5 V threshold'
assert m.read(0x40010400)&line, 'B31 EXTI16 interrupt disabled'
assert m.read(0x40010408)&line and not m.read(0x4001040C)&line, 'B31 must trigger on PVDO assertion only'
assert m.read(0xE000E100)&2, 'B31 NVIC PVD disabled'
resets=[];clears=[]
marker=0x40006C04;magic=0x5056;event=39
def reset():
    assert m.read(marker,2)==magic, 'PVD reset must persist BKP_DR1 marker before NVIC_SystemReset'
    resets.append(1);m.ret()
for value in re.findall(r"^NVIC_SystemReset\s+0x([0-9a-f']+)",m.maptext,re.M):
    m.stubs[int(value.replace("'",''),16)&~1]=reset
watching = [False]
def clear(u, access, address, size, value, data):
    if watching[0] and address == 0x40010414:
        assert value == line
        clears.append(1)
m.u.hook_add(UC_HOOK_MEM_WRITE, clear)
if '--startup' not in sys.argv:
    for pending,low,expected in ((0,0,0),(0,1,0),(1,0,0),(1,1,1)):
        m.write(marker,0x1234,2);m.write(marker+4,0xABCD,2)
        resets.clear();clears.clear();m.write(0x40010414,line if pending else 0);m.write(0x40007004,4 if low else 0)
        watching[0] = True
        m.call('PVD_IRQHandler')
        watching[0] = False
        assert len(resets)==expected and len(clears)==pending
        assert m.read(marker,2)==(magic if expected else 0x1234)
        assert m.read(marker+4,2)==0xABCD, 'adjacent BKP register modified'
    print('PVD IAR: EXTI16 rising, marker precedes reset, no spurious reset PASS')

# Исполняем настоящий user_init и журнал; заменены только периферия и конфигурация.
# Событие при старте встаёт в очередь журнала; во флеш его пишет фоновый владелец
# журнала, поэтому синхронная запись log_addEvent здесь — ошибка.
records=[]
def sync_write():
    raise AssertionError('event written synchronously at startup instead of the journal queue')
def fresh(value):
    global m
    m=Machine();m.init();m.u.mem_map(0x42000000,0x600000)
    for name in ('w25_read_devID','mem_recover','user_readconfig','user_initvariables',
                 'app_boot_metadata','sched16_boot','uart_init','uart_setRecTimeout','rs485_init'):
        m.stubs[m.addr(name)]=lambda:m.ret()
    m.stubs[m.addr('log_addEvent')]=sync_write
    m.write(marker,value,2)
    return m
def queued():
    count=m.read(m.addr('log_pendingCount'),1)
    return [bytes(m.u.mem_read(m.addr('log_pending')+32*i,32)) for i in range(count)]

def startup(value):
    fresh(value)
    m.call('board_init')
    m.call('user_init')
    m.call('log_setTime',123456)
    records.extend(queued())

startup(magic)
assert len(records)==1 and records[0][8]==event, 'PVD startup must emit one power-reset journal event'
assert records[0][9:]==bytes(23), 'power-reset event arguments must be zero'
assert m.read(marker,2)==0, 'PVD startup must clear accepted marker'
startup(0)
assert len(records)==1, 'second startup without marker must not duplicate event'
for value in (0,0x1234,0xFFFF):
    startup(value)
    assert len(records)==1 and m.read(marker,2)==value, 'unrelated BKP value must not log a reset'

# При отказе журнала метка остаётся до следующего успешного старта.
fresh(magic);m.write(m.addr('log_pendingCount'),16,1)
m.call('user_init')
assert m.read(marker,2)==magic, 'failed log insertion must retain marker'
startup(magic)
assert len(records)==2 and records[-1][8]==event and m.read(marker,2)==0
print('PVD IAR: startup event 39 once, second startup silent, invalid marker ignored, failed insertion retained PASS')

# Настоящий user_init читает CSR до RMVF; старый POR не скрывает WDT/SW.
por=1<<27; pin=1<<26
for flags,cold in ((0,False),(por,True),(por|pin,True),(pin,False),
                   *((bit,False) for bit in (1<<28,1<<29,1<<30,1<<31)),
                   *((por|bit,False) for bit in (1<<28,1<<29,1<<30,1<<31))):
    fresh(0);m.write(0x40021024,flags);m.call('user_init')
    assert m.read(m.addr('power_state'),1)==(7 if cold else 0), ('cold modem start',hex(flags),cold)
print('modem IAR: POR/PDR skips discharge once; pin/SW/IWDG/WWDG/LPWR and retained POR keep discharge PASS')
