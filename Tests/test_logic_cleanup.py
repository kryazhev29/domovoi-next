# Доработка: Кряжев А., 2026
"""Проверки логики на связанном ELF: кадры, доступ и замер инструкций до/после.
Периферийные границы моделируются; разбор команд и скан таблиц исполняются из прошивки.
Результаты и замеры сохраняются вне дерева исходников.
"""
import argparse
import hashlib
import json
import random
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'Tests'), str(ROOT / 'Tests/asm')]
from firmware_suite import select_image
from protocol_contract import Profile
from test_access_reads import AccessMachine
from test_service_runtime import ServiceMachine
from storage_machine import StorageMachine, crc16
from iar_layout import fields
from iar_image import image_paths


# Сформировать независимый эталон входного SLDN.
def frame(command, payload=b'', uniq=73):
    body = struct.pack('<HBBB', len(payload) + 3, 0, uniq, command) + payload
    return b'SLDN' + body + struct.pack('<H', crc16(body))


# Читать короткое состояние сборщика из ELF, а не из ожидаемого размещения.
def used(machine):
    return int.from_bytes(machine.u.mem_read(machine.addr('server_rx_count'), 2), 'little')


# Дождаться обработки буфера; неполный хвост может законно остаться.
def drain_rx(machine):
    for _ in range(100):
        before = used(machine), len(machine.sent)
        machine.checked('server_rx_begin_pass')
        machine.checked('server_rx_process')
        after = used(machine), len(machine.sent)
        if not after[0] or before == after:
            return
    raise AssertionError('сборщик не заканчивает работу')


# Отдать поток отдельными закончёнными QIRD, сохраняя обратное давление.
def receive(machine, data, chunk):
    at = 0
    while at < len(data):
        machine.checked('server_rx_begin_pass')
        capacity = machine.checked('user_comm_rx_capacity')
        if not capacity:
            drain_rx(machine)
            capacity = machine.checked('user_comm_rx_capacity')
            assert capacity, 'QIRD не получает свободное место'
        part = data[at:at + min(chunk, capacity)]
        machine.u.mem_write(0x21000000, part)
        assert machine.checked('user_comm_rx', 0x21000000, len(part)) == len(part)
        machine.checked('user_comm_rx_done', len(part), 0)
        drain_rx(machine)
        at += len(part)


# Выполнить реальные ответы протокола вместо заглушки ответа владельца хранения.
def rx_machine():
    machine = ServiceMachine()
    machine.stubs.pop(machine.addr('user_send_answer_comand'), None)
    return machine


# Проверить поток, повреждения, срок ожидания и границу незавершённого QIRD.
def rx_contracts():
    checks = 0
    good = frame(27)
    bad = bytearray(frame(27, uniq=72)); bad[-1] ^= 1
    streams = [good, good * 3, b'noiseSLD' + good, bytes(bad) + good,
               b'SLDN\xff\xffgarbage' + good, b'SLDN\x00\x00' + good,
               b'SLDN\x02\x00' + good, b'x' * 2048 + good]
    for stream in streams:
        for chunk in (1, 2, 3, 7, 64, 2048):
            machine = rx_machine(); receive(machine, stream, chunk)
            expected = 3 if stream == good * 3 else 1
            assert len(machine.sent) == expected, (stream[:12], chunk, machine.sent)
            assert not used(machine)
            assert all(p[:4] == b'SLDN' and p[8:11] == bytes([11, 27, 0])
                       and crc16(p[4:-2]) == int.from_bytes(p[-2:], 'little') for p in machine.sent)
            checks += 1
    machine = rx_machine()
    machine.u.mem_write(0x21000000, good)
    machine.checked('user_comm_rx', 0x21000000, len(good))
    machine.checked('server_rx_process'); assert not machine.sent
    machine.checked('user_comm_rx_done', len(good), 1)
    drain_rx(machine); assert not used(machine) and not machine.sent
    checks += 1
    for expiry in (29999, 30000):
        machine = rx_machine(); receive(machine, good[:8], 8)
        machine.now = expiry; machine.checked('server_rx_begin_pass')
        assert bool(used(machine)) == (expiry < 30000)
        checks += 1
    machine = rx_machine(); receive(machine, good[:8], 8)
    machine.now = 30000
    machine.u.mem_write(0x21000000, good)
    machine.checked('user_comm_rx', 0x21000000, len(good))
    machine.checked('server_rx_process'); assert not machine.sent
    machine.checked('user_comm_rx_done', len(good), 0)
    drain_rx(machine); assert len(machine.sent) == 1
    checks += 1
    machine = rx_machine(); receive(machine, good[:8], 8)
    for name in ('photo_connection_lost', 'fw_service_quiet', 'upload_runtime_quiet',
                 'table_runtime_quiet', 'service_crc_quiet'):
        machine.stubs[machine.addr(name)] = machine.ret
    machine.checked('user_comm_session', 1); assert not used(machine)
    receive(machine, good, 3); assert len(machine.sent) == 1
    return checks + 1


# Проверить исключённые физические источники, сохранив Wiegand и RS485.
def reader_contracts():
    for source, address, code in ((0, 0, 1), (1, 0, 0), (2, 0, 1),
                                  (3, 0, 1), (3, 1, 0), (4, 1, 1), (255, 1, 1)):
        machine = rx_machine()
        receive(machine, frame(6, bytes([5, source, address])), 2)
        assert len(machine.sent) == 1 and machine.sent[0][10] == code, (source, machine.sent)
    return 7


# Замерить реальные инструкции на одних и тех же внешних данных.
def measurements():
    result = {}
    def measure(name, machine, call, value):
        profile = Profile(machine, {})
        actual = profile.run(call)
        result[name] = {'instructions': profile.last['instructions'], 'result': value(machine, actual), 'functions': profile.totals}
    for name, found in (('find-last', True), ('find-absent', False)):
        machine = AccessMachine()
        for cell in range(1, 70): machine.put_id(cell, b'%010d' % cell)
        machine.u.mem_write(0x21000400, b'0000000060' if found else b'9999999999')
        measure(name, machine, lambda: machine.checked('user_findID', 0x21000400, 0x21000300),
                lambda m, r: [r, bytes(m.u.mem_read(0x21000300, 32)).hex() if r else ''])
    machine = AccessMachine(); machine.put_group(1)
    for cell in range(1, 70): machine.put_id(cell, b'%010d' % cell)
    machine.u.mem_write(0x21000400, b'0000000060')
    measure('access', machine, lambda: machine.checked('user_checkIdAndControl', 0x21000400, 2),
            lambda m, r: [m.events, m.loads])
    machine = ServiceMachine(); machine.stubs.pop(machine.addr('user_send_timeshtamp'), None)
    measure('reply-12', machine, lambda: machine.checked('user_send_timeshtamp', 1, 0x12345678, 0x9abc, 1, 73),
            lambda m, r: [p.hex() for p in m.sent])
    machine = ServiceMachine()
    def crc_reply():
        machine.packet(17, channel=1); machine.drain(60000)
    measure('reply-17', machine, crc_reply, lambda m, r: [p.hex() for p in m.sent])
    for chunk in (1, 2048):
        machine = rx_machine()
        measure('rx-' + str(chunk), machine, lambda: receive(machine, b'noise' + frame(27) * 2, chunk),
                lambda m, r: [p.hex() for p in m.sent])
    return result


# Проверить скан ID на границе страниц, оборванную ячейку и отказ чтения NOR.
def startup_counts():
    checks = 0
    for fail_at in (0, 1, 2):
        m = StorageMachine()
        base = 0x41000
        m.u.mem_write(m.addr('mem') + 2 * 8, struct.pack('<II', base, 512))
        m.flash[base:base + 512] = b'\xff' * 512
        for cell in (0, 7, 8):
            record = bytearray(32)
            struct.pack_into('<H', record, 0, cell + 1)
            record[fields('cellID_s')['ID']:fields('cellID_s')['ID'] + 10] = b'1234567890'
            m.flash[base + cell * 32:base + (cell + 1) * 32] = record
        m.flash[base + 15 * 32 + 2] = 0  # Оборванная запись тоже занимает место.
        if fail_at:
            m.fault = ('read', fail_at)
        m.checked('user_ids_boot')
        for _ in range(2):
            if m.boot_scan_tick() == 3:
                break
        if fail_at:
            # Отказ чтения: поиск идёт по всей базе, конец данных неизвестен.
            assert int.from_bytes(m.u.mem_read(m.addr('id_cells_end'), 2), 'little') == 512 // 32
            assert int.from_bytes(m.u.mem_read(m.addr('id_used_bytes'), 4), 'little') == 0xFFFFFFFF
        else:
            assert int.from_bytes(m.u.mem_read(m.device+m.d['numbersCount'], 2), 'little') == 3
            assert int.from_bytes(m.u.mem_read(m.addr('id_cells_end'), 2), 'little') == 9
            assert int.from_bytes(m.u.mem_read(m.addr('id_used_bytes'), 4), 'little') == 512
        checks += 1
    for count in (0, 7, 8, 9, 16):
        m = StorageMachine()
        base = 0x1e8000
        m.u.mem_write(m.addr('mem') + 8 * 8, struct.pack('<II', base, 512))
        m.flash[base:base + 512] = b'\xff' * 512
        from test_upload_runtime import temp
        for cell in range(count):
            m.flash[base + cell * 32:base + (cell + 1) * 32] = temp(cell + 1)
        assert m.checked('user_getTempIDRecordsCount') == count
        assert int.from_bytes(m.u.mem_read(m.device + m.d['newAddressTempID'], 4), 'little') == base + count * 32
        checks += 1
    return checks


# Проверить отсутствие мёртвого расчёта ПИН и размер тематических модулей.
def source_contracts():
    new = (ROOT / 'User/c/user_config.c').read_text(encoding='utf-8')
    assert 'user_unipassword' not in new and 'user_imeivalid' not in new
    assert not (ROOT / 'User/hybrid').exists()
    assert not (ROOT / 'User/c/user_fun_runtime.c').exists()
    for path in (ROOT / 'User/c').glob('user_*.c'):
        data = path.read_bytes(); text = data.decode('utf-8')
        assert b'\n' not in data.replace(b'\r\n', b'') and len(text.splitlines()) <= 600, path
    return 1


# Сохранить измерения вне дерева проекта для сравнения старого и нового ELF.
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-root', type=Path, required=True)
    parser.add_argument('--config', default='Release')
    parser.add_argument('--baseline', action='store_true')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    import os
    os.environ['DOMOVOI_COMPARE_CONFIG'] = args.config
    select_image(args.build_root)
    checks = 0 if args.baseline else source_contracts() + rx_contracts() + reader_contracts() + startup_counts()
    data = {'checks': checks, 'measurements': measurements(),
            'elf_sha256': hashlib.sha256(image_paths()[0].read_bytes()).hexdigest()}
    args.output.write_text(json.dumps(data, ensure_ascii=False, indent=2, default=lambda v: v.hex()) + '\n', encoding='utf-8')
    print('PASS', checks, 'проверок; инструкции сохранены в', args.output)


if __name__ == '__main__':
    main()
