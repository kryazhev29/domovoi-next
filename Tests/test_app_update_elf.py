# Доработка: Кряжев А., 2026
"""Публикация и подтверждение прошивки на ELF с отказами границы NOR.
Приложение выполняет реальные функции; модель W25 подтверждает записи и стирания.
Проверки не запускают замену загрузчика, перезагрузку, плату или сервер.
"""
import argparse
import hashlib
import json
import os
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'Tests'), str(ROOT / 'Tests/asm')]
from c_vs_asm_suite import select_image
from iar_image import image_paths
from storage_machine import StorageMachine

JOURNAL = 0x20000
MAGIC = 0x4e465744


# Посчитать CRC STM32 независимо от исполняемой функции приложения.
def image_crc(image):
    crc = 0xffffffff
    for (word,) in struct.iter_unpack('<I', image):
        crc ^= word
        for _ in range(32):
            crc = ((crc << 1) ^ (0x04c11db7 if crc & 0x80000000 else 0)) & 0xffffffff
    return crc


# Подготовить реальный заголовок, векторы и аппаратные метки для платы 128 КиБ.
def machine():
    m = StorageMachine()
    m.u.mem_map(0x1ffff000, 0x1000)
    m.write(0x1ffff7e0, 128, 2)
    m.write(0x08007ffc, 0x32, 1)
    m.write(0x08007ffd, 139, 1)
    image = struct.pack('<II', 0x20004ff8, 0x08008209) + bytes(range(128))
    header = struct.pack('<IBHBIIII8s', 0xffffffff, 139, 71, 255,
                         len(image), image_crc(image), 0xffffffff, 1791331200, bytes([255]) * 8)
    m.flash[:32] = header
    m.flash[512:512 + len(image)] = image
    return m


# Отвергнуть каждое неудачное чтение образа до стирания журнала и публикации magic.
def request_faults():
    m = machine()
    assert m.checked('app_boot_request') == 1
    assert m.flash[:4] == struct.pack('<I', MAGIC)
    reads = m.counts['read']
    assert reads == 3, 'лишняя перепроверка W25 в приложении'
    for failure in range(1, reads + 1):
        m = machine()
        before = bytes(m.flash)
        m.fault = ('read', failure)
        assert m.checked('app_boot_request') == 0
        assert bytes(m.flash) == before
    for name in ('w25_erase_sector', 'w25_write_data'):
        m = machine()
        m.stubs[m.addr(name)] = lambda m=m: m.ret(0)
        assert m.checked('app_boot_request') == 0
        assert m.flash[:4] == bytes([255]) * 4
    return reads + 2


# Отвергнуть несовместимую плату, размеры и векторы до первой записи.
def request_boundaries():
    cases = [('hardware', 4, 140, 1), ('empty', 8, 0, 4),
             ('alignment', 8, 137, 4), ('large', 8, 0x40000, 4),
             ('crc', 12, 123, 4)]
    for name, offset, value, width in cases:
        m = machine()
        m.flash[offset:offset + width] = value.to_bytes(width, 'little')
        before = bytes(m.flash)
        assert m.checked('app_boot_request') == 0, name
        assert bytes(m.flash) == before
        assert m.counts['read'] == (3 if name == 'crc' else 1)
    vectors = [(0x20000000, 0x08008209), (0x20004ff9, 0x08008209),
               (0x20004ff8, 0x08008001), (0x20004ff8, 0x08008208),
               (0x20004ff8, 0x08008301)]
    for sp, reset in vectors:
        m = machine()
        m.flash[512:520] = struct.pack('<II', sp, reset)
        m.flash[12:16] = struct.pack('<I', image_crc(m.flash[512:648]))
        before = bytes(m.flash)
        assert m.checked('app_boot_request') == 0, (sp, reset)
        assert bytes(m.flash) == before and m.counts['read'] == 2
    return len(cases) + len(vectors)


# Восстановить только точно взведённую пробу и подтвердить её после успеха W25.
def trial_faults():
    m = machine()
    m.flash[JOURNAL + 4:JOURNAL + 8] = bytes(4)
    m.flash[JOURNAL + 24:JOURNAL + 28] = bytes(4)
    m.checked('app_boot_metadata')
    assert m.checked('app_trial_active') == 1
    reads = m.counts['read']
    m.stubs[m.addr('w25_write_data')] = lambda: m.ret(0)
    m.checked('app_trial_confirm')
    assert m.checked('app_trial_active') == 1
    assert m.flash[JOURNAL + 28:JOURNAL + 32] == bytes([255]) * 4
    m.stubs[m.addr('w25_write_data')] = m.sync_write
    m.checked('app_trial_confirm')
    assert m.checked('app_trial_active') == 0
    assert m.counts['read'] == reads, 'подтверждение повторно читает запись W25'
    assert m.flash[JOURNAL + 28:JOURNAL + 32] == bytes(4)
    m = machine()
    m.flash[JOURNAL + 4:JOURNAL + 8] = bytes(4)
    m.flash[JOURNAL + 24:JOURNAL + 28] = bytes(4)
    m.fault = ('read', 1)
    m.checked('app_boot_metadata')
    assert m.checked('app_trial_active') == 1
    return 3


# Не создавать ложное событие ошибки при недоступном журнале загрузчика.
def error_faults():
    for failure in (1, 2):
        m = machine()
        m.flash[JOURNAL + 12:JOURNAL + 16] = struct.pack('<I', 1)
        m.fault = ('read', failure)
        assert not m.checked('app_boot_error_unreported')
    m = machine()
    m.flash[JOURNAL + 12:JOURNAL + 16] = struct.pack('<I', 1)
    assert m.checked('app_boot_error_unreported') == 1
    assert m.checked('app_boot_error_code') == 21
    m.checked('app_boot_error_reported')
    assert not m.checked('app_boot_error_unreported')
    m = machine()
    m.fault = ('read', 1)
    assert m.checked('app_boot_error_code') == 30
    return 4


# Запустить прикладные проверки выбранного ELF и сохранить контрольную сумму артефакта.
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-root', type=Path, required=True)
    parser.add_argument('--variant', choices=('C', 'HYBRID'), default='HYBRID')
    parser.add_argument('--config', choices=('Release', 'ReleaseCustomer'), default='Release')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    os.environ['DOMOVOI_COMPARE_CONFIG'] = args.config
    select_image(args.variant, args.build_root)
    result = {'checks': request_faults() + request_boundaries() + trial_faults() + error_faults(),
              'elf_sha256': hashlib.sha256(image_paths()[0].read_bytes()).hexdigest()}
    if args.output:
        args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('PASS прикладное обновление ELF:', result['checks'], 'проверок отказов и публикации')


if __name__ == '__main__':
    main()
