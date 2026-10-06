"""Проверяет атомарность таймеров и обработчик SysTick в свежем ELF IAR.
Вставляет прерывания на каждом шаге записи и чтения таймера.
Доработка: Кряжев А., 2026"""
from iar_irq_support import Machine,UC_ARM_REG_PRIMASK,UC_HOOK_MEM_WRITE
m=Machine()
m.stubs[m.addr('timers_user_handle')]=lambda:m.ret()
base=m.addr('timer');millis=m.addr('milliSeconds')
timer_bytes=m.map_size('timer')
setting=False
# Выявляет прерывание во время записи таймера.
def write_hook(u,access,address,size,value,_):
    if setting and base<=address<base+timer_bytes and not m.in_irq:
        assert u.reg_read(UC_ARM_REG_PRIMASK)==1,'timer set store executes with IRQ unmasked'
m.u.hook_add(UC_HOOK_MEM_WRITE,write_hook)
# Записывает новое значение таймера через рабочую функцию.
def set_timer(*args,**kwargs):
    global setting
    setting=True
    try:return m.call('timers_set',*args,**kwargs)
    finally:setting=False
m.init();set_timer(3,100);steps=m.steps
assert m.volatile.get('milliSeconds') and m.volatile.get('timer'),'shared timer declarations must be volatile in IAR DWARF'
for at in range(steps):
    m.init();m.write(base+12,7)
    set_timer(3,100,inject_at=at,irq=('SysTick_Handler',()))
    assert m.read(base+12) in (100,101)
    assert m.read(millis)==1 and m.read(base)==1
    assert m.u.reg_read(UC_ARM_REG_PRIMASK)==0
for name,address,args in (('timers_get',base+12,(3,)),('timers_getMillis',millis,())):
    m.init();m.write(address,7);m.call(name,*args);count=m.steps
    for at in range(count):
        m.init();m.write(address,7)
        value=m.call(name,*args,inject_at=at,irq=('SysTick_Handler',()))
        assert value in (7,8) and m.read(address)==8,(name,at,value)
        assert m.u.reg_read(UC_ARM_REG_PRIMASK)==0
    m.init();m.u.reg_write(UC_ARM_REG_PRIMASK,1);m.call(name,*args);assert m.u.reg_read(UC_ARM_REG_PRIMASK)==1
m.init();m.u.reg_write(UC_ARM_REG_PRIMASK,1);set_timer(0,5);assert m.u.reg_read(UC_ARM_REG_PRIMASK)==1
m.init();m.write(millis,0xfffffffe)
# Приостанавливает проверку в точке вызова SysTick.
def pause_tick():
    m.ret();m.pending=True;m.irq=('SysTick_Handler',())
m.stubs[m.addr('wdt_reload')]=pause_tick
m.call('timers_pause',5)
assert m.read(millis)==3
print(f'IAR timers IRQ PASS: set at {steps} boundaries, coherent snapshots, saved PRIMASK, real SysTick pause across uint32 wrap')
