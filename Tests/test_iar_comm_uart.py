"""Доработка: Кряжев А., 2026.
Проверяет настоящий код USART3 из выбранного ELF IAR.
Наблюдает регистры и обращения к DR; физическая плата не используется.
"""
from iar_irq_support import Machine
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.arm_const import UC_ARM_REG_PRIMASK

USART = 0x40004800
RCC = 0x40021000
GPIO = 0x40010C00


# Проверить делитель, соседние GPIO и обе группы NVIC, встречающиеся при старте v5.
def initialization():
    for clock, prescaler, brr in ((72000000, 0, 625), (72000000, 4, 312),
                                  (72000000, 5, 156), (24000000, 0, 208)):
        for group, priority in ((7, 0x70), (6, 0xF0)):
            m = Machine()
            m.init()
            m.write(m.addr('SystemCoreClock'), clock)
            m.write(RCC + 4, prescaler << 8)
            m.write(GPIO + 4, 0xA5A5A5A5)
            m.write(0xE000ED0C, group << 8)
            m.u.reg_write(UC_ARM_REG_PRIMASK, 1)
            m.call('comm_uart_init', 115200)
            assert m.u.reg_read(UC_ARM_REG_PRIMASK) == 1
            assert m.read(USART + 8) == brr
            assert m.read(USART + 12) == 0x200C
            assert m.read(USART + 16) == 0
            assert m.read(USART + 20) == 0x241
            assert m.read(GPIO + 4) == 0xA3858BA5
            assert m.read(RCC + 24) & 9 == 9
            assert m.read(RCC + 28) & (1 << 18)
            assert m.read(RCC + 20) & 1
            assert m.read(0x40020034) == 512
            for irq in (39, 13):
                assert m.read(0xE000E400 + irq, 1) == priority
                assert m.read(0xE000E100 + 4 * (irq // 32)) & (1 << (irq % 32))


# Проверить отсутствие чтения DR по IDLE, точные байты TX и завершение по отдельному TC.
def transfer():
    m = Machine()
    m.init()
    m.write(m.addr('SystemCoreClock'), 72000000)
    m.call('comm_uart_init', 115200)
    reads, writes = [], []
    m.u.hook_add(UC_HOOK_MEM_READ, lambda u, a, p, n, v, x: reads.append(p),
                 begin=USART + 4, end=USART + 4)
    m.u.hook_add(UC_HOOK_MEM_WRITE, lambda u, a, p, n, v, x: writes.append(v),
                 begin=USART + 4, end=USART + 4)
    m.write(USART, 16)
    m.call('USART3_IRQHandler')
    assert not reads and not writes
    m.u.mem_write(0x21000000, b'AT\r')
    assert m.call('comm_uart_tx', 0x21000000, 3)
    for byte in b'AT\r':
        m.write(USART, 16 | 128 | 64)
        m.call('USART3_IRQHandler')
        assert writes[-1] == byte and not reads
        assert not m.call('comm_uart_tx_done')
    m.write(USART, 64)
    m.call('USART3_IRQHandler')
    assert writes == list(b'AT\r') and m.call('comm_uart_tx_done')
    m.write(USART, 8 | 2 | 4)
    m.call('USART3_IRQHandler')
    assert len(reads) == 1 and m.call('comm_uart_errors') == 14
    assert not m.call('comm_uart_errors')


if __name__ == '__main__':
    initialization()
    transfer()
    print('IAR USART3: BRR/GPIO/DMA/NVIC/PRIMASK, IDLE/DR, TX/TC, ошибки PASS')
