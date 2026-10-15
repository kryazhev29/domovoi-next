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
ids_boot = 'user_ids_boot' if 'user_ids_boot' in m.symbols else 'user_getNumbersCount'
steps = (ids_boot, 'log_getRecordsCount', 'user_getTempIDRecordsCount',
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
result['startup_steps_ms'] = {key: round(sum(r[key + '_ms_72MHz'] for r in rows), 3)
                              for key in ('instructions', 'fetch_estimate')}
# Варианты а/б используют тот же поиск: готовая корзина или полный резервный проход.
from test_idindex_runtime import bucket
last = 1601536 // 32
record = bytearray(32); struct.pack_into('<H', record, 0, last)
record[2:12] = b'9999999999'; record[12] = 1
m.flash[266240+1601536-32:266240+1601536] = record
m.u.mem_write(0x21000000, bytes(record[2:12]))
first_cards = []
def measured(name, action):
    reads.clear(); value = profile.run(action)
    data = {'step': name, 'result': value, **profile.last,
            'reads': len(reads), 'bytes': sum(n for _, n in reads)}
    sizes = Counter(n for _, n in reads)
    for n in sizes:
        if n not in costs: costs[n] = spi_cost(n)
    for key in ('instructions', 'fetch_estimate'):
        data[key + '_ms_72MHz'] = round((profile.last[key] + sum(sizes[n]*costs[n][key] for n in sizes))/72000, 3)
    return data
for ready in (False, True):
    m.write(m.addr('id_cells_end'), last, 2)
    if ready and ids_boot == 'user_ids_boot': m.write(m.addr('id_scan_cell'), 0, 2)
    m.write(m.addr('ix')+10, 3 if ready else 4, 1)
    address = bucket(record[2:12]); m.flash[address:address+2] = struct.pack('<H', last)
    row = measured('ready-index (a)' if ready else 'full-search (b)',
                   lambda: m.call('user_findID', 0x21000000, 0x21000200, budget=30000000))
    assert row['result'] == 1; first_cards.append(row)
result['first_card_at_real_end'] = first_cards
if ids_boot == 'user_ids_boot':
    m.call('user_ids_boot'); m.write(m.addr('ix')+10, 0, 1)
    m.call('idindex_runtime_begin')
    result['scan_step_live'] = measured('scan eight live IDs', lambda: m.call('flash_exec_poll', 0))
    m.call('flash_exec_take_result')
    m.write(m.addr('id_scan_cell'), last-7, 2)
    # Полный журнал: последний квант включает ограниченное чтение 4 КиБ отметок.
    m.flash[0x340000:0x341000] = struct.pack('<II', 0, 0x58444932 ^ 0xffffffff) * 512
    m.call('idindex_runtime_begin')
    result['scan_step_last_full_marks'] = measured('last scan + full marks', lambda: m.call('flash_exec_poll', 0))
    # Полная первая карта: готовность Wiegand -> решение доступа -> выход и событие.
    from test_access_reads import AccessMachine
    m = AccessMachine(); m.u.mem_write(m.addr('mem')+16, struct.pack('<II', 266240, 1601536))
    m.put_group(1); m.put_id(last, b'0000050048'); m.call('user_ids_boot')
    m.u.mem_write(0x21000000, b'0000050048')
    original_read = m.read; m.stubs[m.addr('w25_read_data')] = nor_read
    profile = Profile(m, {})
    def first_access():
        assert m.call('user_access_ready', 0x21000000, budget=30000000)
        m.call('WG_readedID', 0x50048, budget=30000000)
        assert m.loads and m.events[-1][0] == 71
    result['first_Wiegand_access_at_real_end'] = measured('first Wiegand access (b)', first_access)
    m.write(m.addr('id_scan_cell'), 0, 2); m.write(m.addr('ix')+10, 3, 1)
    address = bucket(b'0000050048'); m.flash[address:address+2] = struct.pack('<H', last)
    m.loads.clear(); m.events.clear()
    result['first_Wiegand_access_ready_index'] = measured('first Wiegand access, READY (a)', first_access)
result_path('user-init-measure.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
for row in rows:
    print(row['step'], 'reads', row['reads'], 'bytes', row['bytes'], 'ms estimates', row['instructions_ms_72MHz'], row['fetch_estimate_ms_72MHz'])
print('startup steps total ms estimates', result['startup_steps_ms'])
for row in first_cards:
    print(row['step'], 'reads', row['reads'], 'bytes', row['bytes'], 'ms estimates', row['instructions_ms_72MHz'], row['fetch_estimate_ms_72MHz'])
if 'first_Wiegand_access_at_real_end' in result:
    print('first Wiegand access ms estimates', {k:result['first_Wiegand_access_at_real_end'][k] for k in ('reads','bytes','instructions_ms_72MHz','fetch_estimate_ms_72MHz')})
    print('READY Wiegand access ms estimates', {k:result['first_Wiegand_access_ready_index'][k] for k in ('reads','bytes','instructions_ms_72MHz','fetch_estimate_ms_72MHz')})
