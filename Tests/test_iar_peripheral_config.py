"""Доработка: Кряжев А., 2026.
Снимок связанной периферии IAR: VCC и 433 удалены, остальные регистры сохранены.
Модель учитывает GPIO, bit-band, аппаратный сброс и завершение калибровки.
Сравнение с базой допускает только два поля GPIOA и два регистра ADC1.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path

from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from iar_irq_support import Machine

REGIONS = (
    (0x40021000, 0x28), (0x40010000, 0x20),
    (0x40010800, 0x1C), (0x40010C00, 0x1C), (0x40011000, 0x1C),
    (0x40007000, 8), (0x40003000, 0x14),
    (0x40013800, 0x1C), (0x40013000, 0x24), (0x40012400, 0x50),
    (0x40005C00, 0x54), (0xE000E010, 0xC),
    (0xE000E100, 0x320), (0xE000ED00, 0x40),
    (0x40000000, 0x50), (0x40000400, 0x50),
    (0x40000800, 0x50), (0x40012C00, 0x50),
)

# Выбрать ровно один образ, чтобы адреса и типы брались из его карты и ELF.
class Registers:
    def __init__(self, machine):
        self.m = machine
        machine.u.mem_map(0x42000000, 0x600000)
        machine.u.mem_map(0x1FFFF000, 0x1000)
        machine.u.mem_write(0x1FFFF7E8, bytes.fromhex('1234567821436587abcd1234'))
        machine.u.hook_add(UC_HOOK_MEM_READ, self.read)
        machine.u.hook_add(UC_HOOK_MEM_WRITE, self.write)

    # Перевести bit-band адрес в обычное слово и номер бита.
    def bit(self, address):
        offset = address - 0x42000000
        byte = 0x40000000 + offset // 32
        return byte & ~3, (byte & 3) * 8 + (offset & 31) // 4

    # Подать готовность кварца/PLL и снять самосбрасывающиеся биты калибровки.
    def read(self, u, access, address, size, value, data):
        m = self.m
        if 0x42000000 <= address < 0x42600000:
            word, bit = self.bit(address)
            m.write(address, (m.read(word) >> bit) & 1)
        elif address == 0x40021000:
            value = m.read(address)
            value = (value | 0x20000) if value & 0x10000 else value & ~0x20000
            value = (value | 0x2000000) if value & 0x1000000 else value & ~0x2000000
            m.write(address, value)
        elif address == 0x40021004:
            value = m.read(address)
            if value & 3 == 2:
                m.write(address, (value & ~12) | 8)
        elif address == 0x40012408:
            m.write(address, m.read(address) & ~12)

    # Выполнить W1C/W1S, GPIO BSRR/BRR, bit-band и сброс отдельных периферийных блоков.
    def write(self, u, access, address, size, value, data):
        m = self.m
        if 0x42000000 <= address < 0x42600000:
            word, bit = self.bit(address)
            current = m.read(word)
            m.write(word, (current | (1 << bit)) if value else current & ~(1 << bit))
            return
        for gpio in (0x40010800, 0x40010C00, 0x40011000):
            if address == gpio + 0x10:
                m.write(gpio + 0xC, (m.read(gpio + 0xC) | (value & 65535)) & ~(value >> 16))
            elif address == gpio + 0x14:
                m.write(gpio + 0xC, m.read(gpio + 0xC) & ~value)
        if address == 0x4002100C:
            for mask, peripheral, length in ((0x200, 0x40012400, 0x50),
                                            (0x1000, 0x40013000, 0x24),
                                            (0x4000, 0x40013800, 0x1C)):
                if value & mask:
                    u.mem_write(peripheral, bytes(length))
                    if peripheral == 0x40013000:
                        m.write(peripheral + 8, 2)
                    elif peripheral == 0x40013800:
                        m.write(peripheral, 0xC0)

# Исполнить тот же порядок периферийного старта без хранилища, модема и задержки 500 мс.
def snapshot(baud, stop, cable, seed):
    m = Machine()
    m.init()
    Registers(m)
    m.write(0x40021000, 3)
    for gpio in (0x40010800, 0x40010C00, 0x40011000):
        m.write(gpio, seed)
        m.write(gpio + 4, seed)
    m.call('SystemInit')
    m.call('board_init')
    m.call('timers_init')
    m.call('spi_init')
    m.call('adc_init')
    m.call('wdt_init', 3750)  # Старый ABI принимает значение, новый использует ту же константу.
    m.call('uart_init', baud, stop, 0x21000000, 64)
    m.call('WG_init')
    if cable:
        m.call('usb_init')
    assert m.call('adc_ready')
    assert m.read(0x40012438) == 0x14A000
    result = {}
    for base, length in REGIONS:
        for address in range(base, base + length, 4):
            result[f'{address:08x}'] = m.read(address)
    # BSRR/BRR — регистры записи, их прочитанные значения не описывают состояние GPIO.
    for gpio in (0x40010800, 0x40010C00, 0x40011000):
        result.pop(f'{gpio + 0x10:08x}')
        result.pop(f'{gpio + 0x14:08x}')
    return result

# Проверить ровно разрешённые отличия удаления PA1/PA4 и третьего канала.
def values():
    m = Machine()
    m.init()
    for channel, register in ((8, 0x4001243C), (9, 0x40012440)):
        for raw in (0, 1, 127, 2048, 4095):
            m.write(register, raw)
            m.write(0x40012444, 4095)
            assert m.call('adc_getinjectedvalue_mV', channel) == raw * 33 * 1016 // 4096
    assert m.call('adc_getinjectedvalue_mV', 4) == 0

# Проверить инициализацию в выбранном профиле.
def current():
    """Проверить живую инициализацию с разными исходными регистрами и UART."""
    count = 0
    for seed in (0, 0x44444444):
        for baud in (9600, 115200):
            for stop in (1, 2):
                for cable in (False, True):
                    registers = snapshot(baud, stop, cable, seed)
                    assert registers['40010800'] & 0x000F00F0 == seed & 0x000F00F0
                    assert registers['40012438'] == 0x14A000
                    count += 1
    return count


if __name__ == '__main__':
    print(current())
