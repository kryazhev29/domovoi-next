"""Доработка: Кряжев А., 2026.
USART1 RS-485 запускается без предварительного board_init.
Проверяются GPIOA/APB2, делитель 72 МГц, стоп-биты и отсутствие USART2/3.
"""
from iar_irq_support import Machine

# Проверить все используемые скорости и оба варианта стоп-битов в свежем ELF.
def run():
    for baud in (1200, 9600, 19200, 57600, 115200):
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
    print('IAR USART1 clock: 10 cases, GPIO/APB2/BRR/stop bits PASS')

if __name__ == '__main__':
    run()
