"""Регрессии периферии в настоящем ELF: LED, ID Flash, NVIC и короткие секции RX."""
import argparse
import itertools

from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.arm_const import UC_ARM_REG_PRIMASK
from iar_irq_support import Machine
from test_iar_peripheral_config import Registers


def leds():
    for led, pin in ((0, 8), (1, 7)):
        for phase in range(16):
            m = Machine(); m.init()
            level = [0]

            def gpio(u, access, address, size, value, data):
                if address == 0x40010C10: level[0] = (level[0] | (value & 65535)) & ~(value >> 16)
                if address == 0x40010C14: level[0] &= ~value

            # MMIO не портит ITSTATE Unicorn при условной записи GPIO.
            m.watch_writes(0x40010C10, 0x40010C17, gpio)
            m.call('led_load', led, 0xFFFF)
            m.call('led_fun100ms')
            assert level[0] & (1 << pin), 'LED_ON missed first tick'
            for _ in range(phase):
                m.call('led_fun100ms')
            for pattern in (0x0002, 0x8001, 0, 0xFFFF):
                m.call('led_load', led, pattern)
                for tick in range(32):
                    m.call('led_load', led, pattern)
                    m.call('led_fun100ms')
                    assert bool(level[0] & (1 << pin)) == bool(pattern & (1 << (tick % 16)))
    print('LED: first tick, every change phase, repeated loads preserve phase PASS')


def flash_id():
    faults = (None, (6, 'TXE'), (6, 'RXNE'), (7, 'TXE'), (7, 'RXNE'), (7, 'BSY'))
    for device in (0x17EF, 0x18EF):
        for fault in faults:
            m = Machine(); m.init()
            m.u.mem_write(m.bottom, b'\xef\x17' * ((m.top - m.bottom) // 2))
            m.write(0x40013000, 0x40)
            sent = []

            def write(u, access, address, size, value, data):
                if address == 0x4001300C:
                    sent.append(value & 255)

            def read(u, access, address, size, value, data):
                if address == 0x40013008:
                    sr = 3
                    if fault:
                        index, flag = fault
                        if flag == 'TXE' and len(sent) == index:
                            sr &= ~2
                        if len(sent) == index + 1:
                            if flag == 'RXNE': sr &= ~1
                            if flag == 'BSY': sr |= 0x80
                    m.write(address, sr)
                elif address == 0x4001300C:
                    response = device & 255 if len(sent) == 7 else device >> 8 if len(sent) == 8 else 0
                    m.write(address, response)

            m.u.hook_add(UC_HOOK_MEM_WRITE, write)
            m.u.hook_add(UC_HOOK_MEM_READ, read)
            assert m.call('w25_read_devID') == (0xFFFF if fault else device), fault
            assert sent[:6] == [0x05, 0xFF, 0x90, 0, 0, 0]
            assert m.call('spi_ready') == (fault is None)
            assert m.read(0x40010C10) == 0x1000, 'CS must be released'
    print('Flash ID: valid 17EF/18EF; TXE/RXNE on either byte and final BSY fail closed PASS')


def priorities():
    for order in itertools.permutations(('usb_init', 'uart_init', 'comm_uart_init')):
        m = Machine(); m.init(); Registers(m)
        m.call('board_init'); m.call('WG_init'); m.call('timers_init')
        for _ in range(2):
            for name in order:
                args = (115200, 0, 0x21000000, 512) if name == 'uart_init' else (115200,) if name == 'comm_uart_init' else ()
                m.call(name, *args)
                assert m.read(0xE000ED0C) & 0x700 == 0x300, ('group changed', name)
        levels = {1: 0x00, 8: 0x10, 9: 0x10, 20: 0x80, 42: 0x80, 37: 0xF0, 39: 0xF0, 13: 0xF0}
        for irq, priority in levels.items():
            assert m.read(0xE000E400 + irq, 1) == priority
        assert m.read(0xE000ED23, 1) == 0xF0
        # F103 реализует четыре старших бита; PRIGROUP=3 отдаёт все вытеснению.
        assert levels[1] >> 4 < levels[8] >> 4 < levels[20] >> 4 < levels[37] >> 4
    print('NVIC: six init/reinit orders, PVD > Wiegand > USB > UART/DMA/SysTick PASS')


def outs_empty():
    for out, initial, activ, repeats in itertools.product((0, 1), (0, 1, 2), (0, 1), (0, 1)):
        if activ and repeats: continue
        m = Machine(); m.init()

        def load(mode, duration, pause, period):
            m.write(m.top, period)  # Пятый аргумент AAPCS передаётся в стеке.
            m.call('out_load', out, mode, duration, pause)

        if initial == 1: load(1, 0, 0, 0)
        if initial == 2: load(3, 3, 1, 3)
        level = m.read(m.addr('out_st'), 1)
        load(3, activ, 1, repeats)
        assert not m.call('out_getmodestate', out)
        for _ in range(100):
            m.call('out_process')
            assert m.read(m.addr('out_st'), 1) == level
    print('Relay: zero duration/repeats stop automaton, preserve off/on/active output PASS')


def wiegand_idle():
    m = Machine(); m.init()
    m.write(m.addr('milliSeconds'), 1000)
    reads = []

    def code(u, address, size, data):
        assert u.reg_read(UC_ARM_REG_PRIMASK) == 0, 'idle Wiegand masked IRQ'

    m.u.hook_add(UC_HOOK_CODE, code)
    m.u.hook_add(UC_HOOK_MEM_READ, lambda *args: reads.append(1),
                 begin=m.addr('milliSeconds'), end=m.addr('milliSeconds') + 3)
    m.call('WG_process')
    assert not reads and m.read(m.addr('wgTime')) == 0
    print(f'Wiegand idle: {m.steps} instructions, no IRQ masking or time read PASS')


def uart_clear():
    lengths = (0, 1, 64, 512)
    steps = []
    for mask in (0, 1):
        for length in lengths:
            m = Machine(); m.init()
            buffer = 0x21000000
            m.call('uart_init', 115200, 0, buffer, 512)
            payload = b'\xa5' * 512
            m.u.mem_write(buffer, payload)
            for i in range(length):
                m.write(0x40013800, 0x20); m.write(0x40013804, 0xA5)
                m.call('USART1_IRQHandler')
            m.u.reg_write(UC_ARM_REG_PRIMASK, mask)
            m.call('uart_clear'); steps.append(m.steps)
            assert m.u.reg_read(UC_ARM_REG_PRIMASK) == mask
            assert not m.call('uart_received') and not m.call('uart_getstate') and not m.call('uart_isReceiving')
            assert bytes(m.u.mem_read(buffer, 512)) == payload
            m.write(0x40013800, 0x20); m.write(0x40013804, 0x37)
            m.call('USART1_IRQHandler')
            assert m.call('uart_received') == 1 and m.read(buffer, 1) == 0x37
    assert len(set(steps)) == 1, steps
    print(f'UART clear: 0..512 bytes, constant {steps[0]} instructions, PRIMASK and next byte PASS')


def usb_consume():
    for tail in (0, 1, 64, 511):
        m = Machine(); m.init()
        buffer = m.addr('usb_buffer')
        payload = bytes(i & 255 for i in range(17 + tail))
        m.u.mem_write(buffer, payload)
        m.write(m.addr('usb_buffer_count'), len(payload), 2)
        m.write(m.addr('usb_rx_expired'), 17 + min(tail, 3), 2)
        m.write(m.addr('usb_rx_silence'), 123, 2)
        moves = []
        move = m.addr('__aeabi_memmove')

        def code(u, address, size, data):
            if address == move: moves.append(1)

        m.u.hook_add(UC_HOOK_CODE, code)
        m.u.reg_write(UC_ARM_REG_PRIMASK, 1)
        m.call('usb_rx_consume', 17)
        assert m.u.reg_read(UC_ARM_REG_PRIMASK) == 1
        assert len(moves) == int(tail != 0)
        assert m.read(m.addr('usb_buffer_count'), 2) == tail
        assert bytes(m.u.mem_read(buffer, tail)) == payload[17:]
        assert m.read(m.addr('usb_rx_expired'), 2) == min(tail, 3)
        assert m.read(m.addr('usb_rx_silence'), 2) == (123 if tail else 0)
    print('USB consume: empty tail skips memmove; nonempty tail, expiry and PRIMASK preserved PASS')


if __name__ == '__main__':
    tests = (leds, flash_id, priorities, outs_empty, wiegand_idle, uart_clear, usb_consume)
    parser = argparse.ArgumentParser()
    parser.add_argument('--case', choices=['all'] + [test.__name__ for test in tests], default='all')
    case = parser.parse_args().case
    for test in tests:
        if case in ('all', test.__name__): test()
