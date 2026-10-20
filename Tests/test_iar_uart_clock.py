"""Доработка: Кряжев А., 2026.
USART1 RS-485 запускается без предварительного board_init.
Проверяются GPIOA/APB2, делитель 72 МГц, стоп-биты и отсутствие USART2/3.
"""
from iar_irq_support import Machine

# Проверить все используемые скорости и оба варианта стоп-битов в свежем ELF.
def run():
    for baud in (1200, 9600, 19200, 38400, 57600, 115200, 460800):
        for stop in (0, 1):
            m = Machine()
            m.init()
            m.call('uart_init', baud, stop, 0x21000000, 64)
            apb2 = m.read(0x40021018)
            assert apb2 & ((1 << 14) | 4 | 1) == ((1 << 14) | 4 | 1)
            assert m.read(0x4002101C) == 0
            divider = 25 * 72000000 // (4 * baud)
            integer = divider // 100
            brr = (integer << 4) | (((divider - 100 * integer) * 16 + 50) // 100 & 15)
            assert m.read(0x40013808) == brr
            assert m.read(0x40013810) == (0 if stop == 0 else 0x2000)
            assert m.read(0x40010804) & 0xFF0 == 0x8B0
    # Повторная настройка того же USART: мост -> камера -> мост.
    # Проверяем машинный код и регистры; электрическую линию модель не воспроизводит.
    m = Machine()
    m.init()
    m.write(0x40010804, 0x88888888)
    for baud, brr in ((115200, 625), (460800, 156), (115200, 625),
                      (460800, 156), (115200, 625)):
        m.call('uart_init', baud, 0, 0x21000000, 64)
        assert m.read(0x4002100C) & (1 << 14) == 0
        assert m.read(0x40013808) == brr
        assert m.read(0x4001380C) == 0x202C  # UE, RXNEIE, TE, RE.
        assert m.read(0x40010804) == 0x888888B8
        assert m.read(0x40010810) == (1 << 10)
        assert m.read(0xE000E104) & (1 << 5)  # USART1_IRQn = 37.
        assert m.call('uart_received') == 0
        m.call('uart_setRecTimeout', 10)
        for i, byte in enumerate((0xAA, 0x0E, 0)):
            m.write(0x40013800, 0xA0 | (2 if byte == 0 else 0))
            m.write(0x40013804, byte)
            m.call('USART1_IRQHandler')
            assert m.call('uart_received') == i + 1
        assert bytes(m.u.mem_read(0x21000000, 3)) == b'\xaa\x0e\0'
        for _ in range(9):
            m.call('uart_timer1ms')
            assert m.call('uart_getstate') == 0
        m.call('uart_timer1ms')
        assert m.call('uart_getstate') == 1 and not m.call('uart_isReceiving')
        m.write(0x40013800, 0x80)
        m.u.mem_write(0x21000100, b'AT\r')
        m.call('uart_send', 0x21000100, 3)
        assert m.read(0x40013804) == 13
    print('IAR USART1 clock: 14 init + 5 baud switches, RX/FE/TX/IRQ/timeout PASS')

if __name__ == '__main__':
    run()
