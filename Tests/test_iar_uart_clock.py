"""B22: настоящий uart_init и RCC, GPIO без предварительного board_init."""
from iar_irq_support import Machine

for port, gpio, usart in ((1, 4, 1 << 17), (2, 8, 1 << 18)):
    m = Machine()
    m.init()
    m.call('uart_init', port, 9600, 0, 0x21000000)
    apb2 = int.from_bytes(m.u.mem_read(0x40021018, 4), 'little')
    apb1 = int.from_bytes(m.u.mem_read(0x4002101C, 4), 'little')
    assert apb2 & gpio, ('B22 GPIO clock absent', port, hex(apb2))
    assert apb1 & usart and not apb1 & gpio, ('B22 wrong APB1 bits', port, hex(apb1))
print('B22 IAR: USART2/3 enable GPIO on APB2, USART on APB1 PASS')
