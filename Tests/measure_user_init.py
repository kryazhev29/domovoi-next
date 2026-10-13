"""Доработка: Кряжев А., 2026. Чтения NOR и оценки инструкций шагов после user_comm_boot."""
import json
import re
import struct
import sys
from collections import Counter
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent / 'asm'))
from storage_machine import StorageMachine, crc16
from protocol_contract import Profile
from iar_irq_support import Machine
from iar_image import result_path


def spi_cost(length):
    m = Machine(); m.init()
    m.u.mem_map(0x42000000, 0x600000)
    m.write(0x40013000, 0x40, 2); m.write(0x40013008, 3, 2)
    profile = Profile(m, {})
    profile.run(lambda: m.call('spi_read', 0x21000000, length))
    buffer = dict(profile.last)
    profile.run(lambda: m.call('spi_send_byte', 0xff))
    byte = dict(profile.last)
    # Настоящий W25/SPI; только ответ статуса NOR принудительно свободен.
    m.stubs[m.addr('spi_send_byte')] = m.ret
    m.stubs[m.addr('spi_read_byte')] = m.ret
    m.stubs[m.addr('wdt_reload')] = m.ret
    profile.run(lambda: m.call('w25_read_data', 0x41000, 0x21000000, length))
    return {k: profile.last[k] + 6 * byte[k] for k in buffer}


m = StorageMachine()
m.u.mem_map(0x42000000, 0x600000)
# Размеры областей совпадают с настоящим user_init, а не с укороченной фикстурой.
for mb, base, size in ((2, 266240, 1601536), (8, 1999872, 49152)):
    m.u.mem_write(m.addr('mem') + mb * 8, struct.pack('<II', base, size))
for i in range(3151):
    cell = bytearray(32); struct.pack_into('<H', cell, 0, i + 1)
    cell[2:12] = ('%010d' % (i + 1)).encode()
    m.flash[266240 + 32 * i:266240 + 32 * (i + 1)] = cell
for i in range(3):
    cell = bytearray(32); struct.pack_into('<H', cell, 0, i + 1)
    m.flash[1999872 + 32 * i:1999872 + 32 * (i + 1)] = cell
# Один полностью занятый сектор современного журнала, без форматирования старого.
header = struct.pack('<8I', 0xd74c4752, 1, 0xfffffffe, 127, 0xffffff80,
                     0xff80007f, 0xffffffff, 0xffffffff)
m.flash[1900544:1900576] = header
for i in range(127):
    cell = bytearray(32); struct.pack_into('<I', cell, 0, i + 1)
    crc = crc16(cell[:24]); struct.pack_into('<HHI', cell, 24, crc, crc ^ 0xffff, 0x32474f4c)
    m.flash[1900576 + 32 * i:1900576 + 32 * (i + 1)] = cell
# Установившийся формат расписаний: миграция и NOR BUSY считаются отдельно.
source = (Path(__file__).resolve().parents[1] / 'User/table_runtime.c').read_text(encoding='utf-8')
offset = int(re.search(r'#define SCHED16_MARK_OFFSET\s+(\d+)', source)[1])
m.u.mem_write(m.device + m.d['reserv'] + offset, bytes(m.u.mem_read(m.addr('sched16_mark'), 8)))
m.write(0x08007ffc, 0x32, 1); m.write(0x08007ffd, 139, 1)
reads, rows = [], []
original_read = m.read
def nor_read():
    address, pointer, length, _ = m.args()
    reads.append((address, length))
    original_read()
m.stubs[m.addr('w25_read_data')] = nor_read
profile = Profile(m, {})
steps = ('user_getNumbersCount', 'log_getRecordsCount', 'user_getTempIDRecordsCount',
         'sched16_boot', 'user_unblock_recheck', 'log_addSystemEvent', 'log_addSystemValue',
         'app_boot_metadata', 'WG_init', 'rs485_init', 'uart_setRecTimeout')
for name in steps:
    reads.clear()
    args = {'user_unblock_recheck': (1,), 'uart_setRecTimeout': (10,),
            'log_addSystemEvent': (39,), 'log_addSystemValue': (40, 0x10)}.get(name, ())
    value = profile.run(lambda: m.call(name, *args, budget=30000000))
    rows.append({'step': name, 'result': value, **profile.last,
                 'reads': len(reads), 'bytes': sum(n for _, n in reads), 'read_sizes': dict(Counter(n for _, n in reads))})
    if name == 'user_getNumbersCount':
        assert value == 3151
        m.write(m.device + m.d['numbersCount'], value, 2)
    elif name == 'log_getRecordsCount': assert value == 127
    elif name == 'user_getTempIDRecordsCount': assert value == 3
costs = {n: spi_cost(n) for n in {n for r in rows for n in r['read_sizes']}}
for row in rows:
    # Отдельно учитывается код W25/SPI для каждого размера чтения.
    for key in ('instructions', 'fetch_estimate'):
        row['with_spi_' + key] = row[key] + sum(count * costs[n][key] for n, count in row['read_sizes'].items())
        row[key + '_ms_72MHz'] = round(row['with_spi_' + key] / 72000, 3)
    row['wire_ms_18MHz'] = round((row['bytes'] + 6 * row['reads']) * 8 / 18000, 3)
result = {'fixture': '3151 живой ID, остальная область FF; 127 событий, 3 временных ID; готовые расписания; NOR не BUSY',
          'method': 'Связанный ELF; инструкции алгоритмов + реальный W25/SPI для размеров чтения. Одна инструкция за такт и выборка без prefetch с +2 ожиданиями на слово Flash. SPI SR готов сразу; ветвления, конвейер, IRQ и физический BUSY не моделируются. Это оценки, не такты/время платы.',
          'steps': rows, 'spi_read_cost': costs}
result_path('user-init-measure.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
for row in rows:
    print(row['step'], 'reads', row['reads'], 'bytes', row['bytes'], 'ms estimates', row['instructions_ms_72MHz'], row['fetch_estimate_ms_72MHz'])
