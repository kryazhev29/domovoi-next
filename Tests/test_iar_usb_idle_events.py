"""Доработка: Кряжев А., 2026.
Неиспользуемые события SUSP/WKUP не меняют тактирование, VTOR и точки CDC.
Проверка исполняет актуальный обработчик USB_Istr из выбранного ELF IAR.
"""
from iar_irq_support import Machine

# Проверить выключенные события по отдельности и вместе при обоих состояниях CDC.
def run():
    cases = 0
    for flags in (0x0800, 0x1000, 0x1800):
        for state in (1, 5):
            m = Machine()
            m.init()
            m.write(m.addr('wInterrupt_Mask'), 0xA700, 2)
            m.write(m.addr('bDeviceState'), state)
            m.write(0x40005C44, flags, 2)
            m.write(0xE000ED08, 0x08008200)
            before = bytes(m.u.mem_read(0x40005C00, 0x400))
            clock = m.read(m.addr('SystemCoreClock'))
            m.call('USB_Istr')
            assert bytes(m.u.mem_read(0x40005C00, 0x400)) == before
            assert m.read(0xE000ED08) == 0x08008200
            assert m.read(m.addr('SystemCoreClock')) == clock
            assert m.read(m.addr('bDeviceState')) == state
            assert m.steps < 100
            cases += 1
    print(f'USB idle events: {cases} cases, registers/VTOR/clock preserved PASS')

if __name__ == '__main__':
    run()
