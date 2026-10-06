"""Проверяет ограниченное ожидание RESET в USB IRQ загрузчика.
Восстановление должно начаться через 20 мс в главном цикле.
Доработка: Кряжев А., 2026"""
import sys
from iar_irq_support import Machine
from unicorn import UC_HOOK_MEM_READ

mode=sys.argv[1] if len(sys.argv)>1 else 'both'
for boot,name in ((True,'USB_Istr'),):
    assert mode in ('both','esof'), mode
    m=Machine(boot=boot);m.init();m.u.mem_map(0x42000000,0x600000)
    polls=[0]
    # Удерживает ESOF и запрещает появление RESET в модели USB.
    def read(u,access,a,n,value,_):
        if a==0x40005c44:
            polls[0]+=1;m.write(a,0x100,2)
        if a==0x40005c48:m.write(a,0x8000,2)
    m.u.hook_add(UC_HOOK_MEM_READ,read)
    m.write(m.addr('wInterrupt_Mask'),0x100,2);m.write(m.addr('esof_counter'),4,1)
    m.call(name,budget=2000000)
    assert polls[0]<=32002, (name,'unbounded RESET wait',polls[0])
    if boot:
        starts=[];m.stubs[m.addr('USB_Init')]=lambda:(starts.append(1),m.ret())
        m.call('boot_usb_poll');assert not starts
        m.write(m.addr('ticks'),19);m.call('boot_usb_poll');assert not starts
        m.write(m.addr('ticks'),20);m.call('boot_usb_poll');assert len(starts)==1
        m.call('boot_usb_poll');assert len(starts)==1
    print(f"B36 IAR {'boot' if boot else 'app'} {name}: bounded RESET wait, main recovery PASS")
