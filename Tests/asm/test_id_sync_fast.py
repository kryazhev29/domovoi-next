# Доработка: Кряжев А., 2026
"""Контракты синхронизации ID и модель времени NOR для единственного ELF.

Артефакты создаются в TEMP; --output задаёт явный путь. Плата не используется.
Выполняются связанные журнал и W25, граница SPI побайтно моделируется.
Оценка CPU: два такта на инструкцию, SPI 18 МГц, запись 0,7 мс,
стирание 45 мс, прочая работа цикла 0,12 мс. Это оценка, не замер платы.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import tempfile

from firmware_suite import select_image
from storage_machine import StorageMachine, Machine, fields, crc16, UC_HOOK_MEM_WRITE
from iar_image import image_paths

BASE, SIZE, CAPACITY = 0x41000, 1601536, 50048


# Создаёт запись ID с заданным номером и содержимым.
def cell(index, version=0):
    b = bytearray(32)
    struct.pack_into('<H', b, 0, index)
    b[2:12] = f'{index + version * 100000:010d}'.encode()
    b[12] = 1
    return bytes(b)


# Считает независимый хеш номера и текста ID.
def hash_id(index, record):
    h = 2166136261
    for b in struct.pack('<H', index) + record[2:12]:
        h = ((h ^ b) * 16777619) & 0xffffffff
    return h


# Готовит базу и метаданные заданного сценария.
def fixture(m, count, unknown=False, total=SIZE):
    m.write(m.addr('mem') + 20, total)
    checksum = 0
    for index in range(1, count + 1):
        record = cell(index)
        m.flash[BASE + 32*(index-1):BASE + 32*index] = record
        checksum ^= hash_id(index, record)
    m.write(m.addr('id_used_bytes'), 0xffffffff if unknown else count * 32)
    m.write(m.addr('id_cells_end'), count, 2)
    m.write(m.device + m.d['numbersCount'], count, 2)
    m.write(m.addr('ix'), checksum)
    m.write(m.addr('ix') + 6, count, 2)
    return m


class SyncMachine(StorageMachine):
    # Готовит связанный образ, модель NOR и состояние сценария.
    def __init__(self, flash=None):
        super().__init__(flash)
        self.sent = []
        self.ranges = []
        self.stubs.pop(self.addr('user_send_timeshtamp'))
        for name in ('usb_send', 'usb_try_send'):
            self.stubs[self.addr(name)] = self.send

    # Моделирует передачу буфера по границе SPI.
    def send(self):
        p, n, _, _ = self.args()
        self.sent.append(bytes(self.u.mem_read(p, n)))
        self.ret(1)

    # Читает модельную NOR с контролем диапазона и заданным отказом.
    def read(self):
        a, _, n, _ = self.args()
        self.ranges.append((a, n))
        super().read()

    # Сверяет данные базы, счётчики и байты ответа с эталоном.
    def verify(self, count, total=SIZE):
        data = self.flash[BASE:BASE+total]
        expected_count = expected_last = used = checksum = 0
        for i in range(0, total, 32):
            row = data[i:i+32]
            if row != b'\xff'*32:
                used = i+32
            if row[:2] != b'\xff\xff':
                expected_count += 1
                expected_last = i//32+1
                checksum ^= hash_id(expected_last, row)
        assert expected_count == count
        assert self.u.mem_read(self.device+self.d['numbersCount'], 2) == struct.pack('<H', count)
        assert self.u.mem_read(self.addr('id_cells_end'), 2) == struct.pack('<H', expected_last)
        assert self.u.mem_read(self.addr('id_used_bytes'), 4) == struct.pack('<I', used)
        if 'storage_has_index' not in self.symbols or self.checked('storage_has_index'):
            assert self.u.mem_read(self.addr('ix'), 4) == struct.pack('<I', checksum)
        assert self.sent and len(self.sent[-1]) == 18
        body = struct.pack('<HBBBBIH', 10, 255, 7, 12, 1, 123456, crc16(data))
        assert self.sent[-1] == b'SLDN'+body+struct.pack('<H', crc16(body)), 'reply differs from full CRC'
        assert self.u.mem_read(self.addr('flash_owner'), 1) == b'\0'
        return self.sent[-1]


# Выполняет запись таблицы и сверяет её данные и ответ.
def run_packet(m, indices=range(1, 31)):
    indices = list(indices)
    m.command(13, b''.join(cell(i, 1) for i in indices), len(indices))
    m.drain(60000)
    return m


# Возвращает ответы и полные образы NOR для парного сравнения.
def protocol_scenarios():
    """Возвращает для парного сравнения байты ответа, метаданные и полный образ NOR."""
    out = {}
    for count, unknown in ((0, False), (3000, False), (CAPACITY, False), (3000, True)):
        m = fixture(SyncMachine(), count, unknown)
        run_packet(m)
        m.verify(max(count, 30))
        out[f'id-fast-{count}-{unknown}'] = {
            'result': {'sent': m.sent, 'nor': bytes(m.flash),
                       'index': bytes(m.u.mem_read(m.addr('ix'), 12))}}
    m = fixture(SyncMachine(), 0, total=8192)
    m.flash[BASE+31] = 0  # Оборванная ячейка выбирает восстановление через журнал секторов.
    m.record_changes = True
    initial = bytes(m.flash)
    run_packet(m)
    final = bytes(m.flash)
    # Обрываем запись назначения после долговечной фиксации журнала.
    image = bytearray(initial)
    for kind, address, data in m.changes:
        if kind == 'write' and address == BASE:
            for i, b in enumerate(data[:17]):
                image[address+i] &= b
            break
        if kind == 'erase':
            image[address:address+4096] = b'\xff'*4096
        else:
            for i, b in enumerate(data):
                image[address+i] &= b
    else:
        raise AssertionError('power cut point not reached')
    reboot = SyncMachine(image)
    assert reboot.checked('sj_recover', budget=2000000)
    assert reboot.flash[BASE:BASE+4096] == final[BASE:BASE+4096]
    assert reboot.u.mem_read(reboot.addr('id_used_bytes'), 4) == b'\xff'*4
    reboot.settle_crc()
    run_packet(reboot)
    reboot.verify(30, 8192)
    out['id-fast-power'] = {'result': {'sent': reboot.sent, 'nor': bytes(reboot.flash)}}
    return out


# Проверяет байты ID, ответы и ресурсные ограничения синхронизации.
def contracts():
    # Повторные номера, пропуски, грязная ячейка со стёртым номером и повтор команды.
    m = fixture(SyncMachine(), 1, total=8192)
    m.flash[BASE+127*32+12] = 0
    m.write(m.addr('id_used_bytes'), 4096)
    run_packet(m, (2, 2, 128, 129, 1, 129))
    m.verify(4, 8192)
    m.record_changes = True
    run_packet(m, (1, 2, 128, 129))
    m.verify(4, 8192)
    assert not m.changes, 'identical packet changed flash'
    assert not any(a >= BASE+4128 and a < BASE+8192 for a, n in m.ranges[-65:])
    # Неизвестная граница определяется один раз, включая записанные осиротевшие байты.
    m = fixture(SyncMachine(), 1, True, total=8192)
    m.flash[BASE+4012] = 0
    run_packet(m, (1,))
    m.verify(1, 8192)
    m.ranges.clear()
    run_packet(m, (1,))
    m.verify(1, 8192)
    assert not any(BASE+4096 <= a < BASE+8192 for a, n in m.ranges)
    return {'metadata_duplicates_retry_unknown': 'PASS'}


# Подменяет связанные инструкции и проверяет обнаружение неверного поведения.
def mutations():
    # Меняем инструкции и данные связанного образа, сохраняя исходники продукта.
    from protocol_contract import Cs, CS_ARCH_ARM, CS_MODE_THUMB, CS_MODE_MCLASS
    md = Cs(CS_ARCH_ARM, CS_MODE_THUMB | CS_MODE_MCLASS)
    detected = {}
    for mode in ('used-end', 'skip-tail', 'chunk-cap', 'millisecond-budget'):
        count = 256 if 'budget' in mode or mode == 'chunk-cap' else 30
        m = fixture(SyncMachine(), count, total=8192)
        m.command(13, b''.join(cell(i, 1) for i in range(1, 31)), 30)
        job = m.addr('table_job')
        for _ in range(4000):
            if m.u.mem_read(job+m.table['phase'], 1) == b'\x03':
                break
            m.tick()
        else:
            raise AssertionError('CRC phase not reached')
        if mode == 'used-end':
            assert m.u.mem_read(job+m.table['crc_limit'], 4) == struct.pack('<I', 960)
            m.write(job+m.table['crc_limit'], 32)
        else:
            if mode == 'skip-tail':
                address = m.addr('crc16_ffInit')
                replacement = b'\x08\x46\x70\x47'  # Возвращаем входную CRC без расчёта.
            else:
                address = m.addr('table_runtime_poll')
                code = list(md.disasm(bytes(m.u.mem_read(address, 900)), address))
                timer = m.addr('timers_getMillis')
                calls = [i for i, ins in enumerate(code)
                         if ins.mnemonic == 'bl' and int(ins.op_str.lstrip('#'), 0) == timer]
                assert len(calls) == 2, calls
                if mode == 'chunk-cap':
                    candidates = [ins for ins in code[max(0, calls[0]-3):calls[0]+4]
                                  if ins.mnemonic == 'movs' and ins.op_str.endswith(', #4')]
                    assert len(candidates) == 1
                    ins = candidates[0]
                    assert ins.size == 2
                    address = ins.address
                    replacement = bytes([32, ins.bytes[1]])
                else:
                    ins = code[calls[-1]+1]
                    assert ins.mnemonic in ('subs', 'subs.w', 'cmp'), (ins.mnemonic, ins.op_str)
                    address = ins.address
                    replacement = b'\x00\x20' + b'\x00\xbf'*(ins.size//2-1)
            original = bytes(m.u.mem_read(address, len(replacement)))
            assert original != replacement
            m.u.mem_write(address, replacement)
            m.u.ctl_remove_cache(address, address+len(replacement))
        elapsed = [0.0]
        if mode == 'millisecond-budget':
            original_read = m.read
            # Добавляет задержку чтения для проверки миллисекундного бюджета.
            def slow_read():
                elapsed[0] += 0.6
                original_read()
            m.stubs[m.addr('w25_read_data')] = slow_read
            m.stubs[m.addr('timers_getMillis')] = lambda: m.ret(int(elapsed[0]))
        try:
            if mode in ('chunk-cap', 'millisecond-budget'):
                m.tick()
                assert elapsed[0] <= 2, 'millisecond slice exceeded 2 ms'
            else:
                m.drain()
                m.verify(count, 8192)
        except AssertionError as exc:
            detected[mode] = str(exc)
        else:
            raise AssertionError((mode, 'mutation survived'))
    return detected


class TimedMachine(SyncMachine):
    # Готовит связанный образ, модель NOR и состояние сценария.
    def __init__(self, gap_ms=0.12):
        self.cycles = 0
        super().__init__()
        self.gap = gap_ms * 72000
        self.polls = 0
        self.peak = 0
        self.crc_peak = 0
        self.id_read = 0
        self.crc_polls = 0
        self.spi_bytes = 0
        self.stubs[self.addr('timers_getMillis')] = lambda: self.ret(int(self.cycles//72000) & 0xffffffff)

    # Перехватывает границы вызовов и проверяет соглашение ARM о стеке.
    def hook(self, u, a, n, unused):
        if a not in self.stubs:
            self.cycles += 2
        super().hook(u, a, n, unused)

    # Подключает модель SPI к реальному драйверу W25.
    def install_nor(self):
        # Измеряем работу процессора в настоящих циклах SPI с готовыми флагами.
        probe = Machine(); probe.init()
        probe.write(0x40013000, 0x40, 2); probe.write(0x40013008, 3, 2)
        self.spi_steps = {}
        for name, args in (('spi_read', (0x21000000, 64)),
                           ('spi_send', (0x21000000, 64)),
                           ('spi_send_byte', (5,)), ('spi_read_byte', ())):
            probe.call(name, *args)
            self.spi_steps[name] = probe.steps
        self.write(0x40013000, 0x40, 2)
        wire = dict(cmd=0, address=0, count=0, wel=0, until=0, data=bytearray())
        self.wire = wire
        for name in ('w25_read_data', 'w25_async_start', 'w25_async_poll',
                     'w25_async_active', 'w25_read_available'):
            self.stubs.pop(self.addr(name))

        # Добавляет расчётную стоимость процессора или NOR.
        def charge(name, n=1):
            self.spi_bytes += n
            scale = n/64 if name in ('spi_read', 'spi_send') else 1
            self.cycles += 32*n + 2*self.spi_steps[name]*scale

        # Моделирует границу выбора SPI-чипа и завершение команды NOR.
        def cs(u, access, address, size, value, unused):
            if value != 4096:
                return
            if address == 0x40010c14:
                wire.update(cmd=0, address=0, count=0, data=bytearray())
            elif address == 0x40010c10:
                if wire['cmd'] == 6:
                    wire['wel'] = 2
                elif wire['cmd'] in (2, 0x20):
                    assert wire['wel'] and self.cycles >= wire['until']
                    a = wire['address']
                    if wire['cmd'] == 0x20:
                        assert not a & 4095
                        self.flash[a:a+4096] = b'\xff'*4096
                        duration = 45
                    else:
                        assert wire['data'] and (a & 255)+len(wire['data']) <= 256
                        for i, b in enumerate(wire['data']):
                            self.flash[a+i] &= b
                        duration = 0.7
                    wire.update(wel=0, until=self.cycles + duration*72000)

        # Моделирует команду, адрес и данные одного обмена SPI.
        def send_byte():
            b = self.args()[0] & 255
            charge('spi_send_byte')
            if not wire['cmd']:
                wire['cmd'] = b
            else:
                assert wire['cmd'] in (2, 3, 0x20) and wire['count'] < 3
                wire['address'] = wire['address']*256+b
                wire['count'] += 1
            self.ret()

        # Моделирует один байт ответа SPI.
        def read_byte():
            charge('spi_read_byte')
            assert wire['cmd'] == 5
            self.ret(wire['wel'] | (self.cycles < wire['until']))

        # Читает модельную NOR с контролем диапазона и заданным отказом.
        def read():
            p, n, _, _ = self.args()
            charge('spi_read', n)
            assert wire['cmd'] == 3 and 0 < n <= 64 and self.cycles >= wire['until']
            a = wire['address']
            self.u.mem_write(p, bytes(self.flash[a:a+n]))
            if BASE <= a < BASE+SIZE:
                self.id_read += n
            self.ranges.append((a, n))
            wire['address'] += n
            self.ret()

        # Моделирует передачу буфера по границе SPI.
        def send():
            p, n, _, _ = self.args()
            charge('spi_send', n)
            assert wire['cmd'] == 2 and 0 < n <= 64
            wire['data'] += self.u.mem_read(p, n)
            self.ret()

        self.u.hook_add(UC_HOOK_MEM_WRITE, cs, begin=0x40010c10, end=0x40010c14)
        for name, fn in (('spi_send_byte', send_byte), ('spi_read_byte', read_byte),
                         ('spi_read', read), ('spi_send', send)):
            self.stubs[self.addr(name)] = fn

    # Выполняет один проход хранения и проверяет его бюджет.
    def tick(self):
        self.cycles += self.gap
        before = self.cycles
        phase = self.u.mem_read(self.addr('table_job')+self.table['phase'], 1)[0]
        self.checked('user_storage_poll', budget=300000)
        cost = (self.cycles-before)/72000
        self.polls += 1
        self.peak = max(self.peak, cost)
        if phase == 3:
            self.crc_polls += 1
            self.crc_peak = max(self.crc_peak, cost)


# Оценивает стоимость связанных путей CPU, SPI и NOR на заданной модели.
def timing(count, unknown=False):
    m = fixture(TimedMachine(), count, unknown)
    m.command(13, b''.join(cell(i, 1) for i in range(1, 31)), 30)
    m.install_nor()
    m.cycles = 0
    m.drain(60000)
    m.verify(max(count, 30))
    return {'ms': round(m.cycles/72000, 3), 'polls': m.polls, 'crc_polls': m.crc_polls,
            'max_poll_ms': round(m.peak, 6), 'max_crc_poll_ms': round(m.crc_peak, 6),
            'id_read_bytes': m.id_read, 'spi_bytes': m.spi_bytes,
            'reply_hex': m.sent[-1].hex(), 'nor_sha256': hashlib.sha256(m.flash).hexdigest(),
            'spi_loop_instructions': m.spi_steps, 'stack': m.stack_peak}


# Запускает проверки модуля и сохраняет результаты в заданный файл.
def main():
    p = argparse.ArgumentParser()
    p.add_argument('--build-root', type=Path)
    p.add_argument('--output', type=Path)
    p.add_argument('--timing', action='store_true')
    p.add_argument('--oracle', action='store_true')
    p.add_argument('--baseline', action='store_true')
    args = p.parse_args()
    select_image(args.build_root)
    result = {'elf_sha256': hashlib.sha256(image_paths()[0].read_bytes()).hexdigest()}
    if args.oracle:
        for name, case in protocol_scenarios().items():
            state = case['result']
            result[name] = {'replies': [b.hex() for b in state['sent']],
                            'nor_sha256': hashlib.sha256(state['nor']).hexdigest()}
        print('PASS linked full-scan oracle', "DomovoiNext", flush=True)
    elif args.timing:
        for count, unknown in ((0, False), (3000, False), (CAPACITY, False), (3000, True)):
            name = f'{count}-'+('unknown' if unknown else 'known')
            result[name] = timing(count, unknown)
            if not args.baseline:
                assert result[name]['max_poll_ms'] <= 2, result[name]
            print("DomovoiNext", name, result[name], flush=True)
    else:
        result.update(contracts())
        print('PASS ID fast contracts', flush=True)
    out = args.output or Path(tempfile.gettempdir())/('id-sync-fast-'+"DomovoiNext"+'.json')
    out.write_text(json.dumps(result, indent=2)+'\n')


if __name__ == '__main__':
    main()

