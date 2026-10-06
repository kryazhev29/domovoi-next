# Доработка: Кряжев А., 2026
from test_work_memory import take, drop, owned, PHOTO, PHOTO_PACKET, REPLY, CONFIG, FW_CONTEXT, FW_DATA, TABLE, UP_CONTEXT, UP_PACKET, SIZES
"""Независимый эталон команд и проводных ответов фонового сервиса CRC."""
import argparse
import hashlib
import json
import random
import struct
from storage_machine import *


class ServiceMachine(StorageMachine):
    # Готовит связанный образ, модель NOR и состояние сценария.
    def __init__(self, used=129, total=8192):
        super().__init__()
        self.sent = []; self.poll_costs = []; self.busy = False
        self.read_ranges = []; self.acquisitions = 0; self.commit_fail = False
        self.transport = 0x21000800
        self.regions = {2: (0x41000, total), 3: (0x1c9000, 2048),
                        4: (0x1ca800, 2048), 5: (0x1cc000, 1024),
                        6: (0x1cd400, 1024)}
        for mb, (base, size) in self.regions.items():
            self.u.mem_write(self.addr('mem') + mb * 8, struct.pack('<II', base, size))
            n = min(used, size) if mb == 2 else size
            self.flash[base:base+n] = bytes((i * 17 + mb * 11) & 255 for i in range(n))
        self.write(self.addr('id_used_bytes'), used)
        self.write(self.addr('cfg_boot_done' if 'cfg_boot_done' in self.symbols else 'cfg_legacy_checked'), 1, 1)
        self.write(self.addr('cfg_cached_active'), 0x1cd400)
        self.write(self.device+self.d['datetime'], 0x78563412)
        self.write(self.device+self.d['freeBufferSize'], 0xa9b8, 2)
        for name, fn in [('usb_send', self.send_usb), ('comm_tx_acquire', self.acquire),
                         ('comm_tx_release', self.ret), ('comm_tx_commit', self.commit),
                         ('comm_tx_busy', lambda: self.ret(self.busy)),
                         ('comm_channel_ready', lambda: self.ret(1))]:
            self.stubs[self.addr(name)] = fn

        if 'usb_try_send' in self.symbols:
            self.stubs[self.addr('usb_try_send')] = self.send_usb

    # Читает модельную NOR с контролем диапазона и заданным отказом.
    def read(self):
        a, p, n, _ = self.args()
        self.read_ranges.append((a, n))
        super().read()

    # Моделирует проверенную синхронную запись NOR.
    def sync_write(self):
        raise AssertionError('service reached synchronous program')

    # Моделирует проверенное синхронное стирание NOR.
    def sync_erase(self):
        raise AssertionError('service reached synchronous erase')

    # Фиксирует USB-кадр с учётом занятости канала.
    def send_usb(self):
        p, n, _, _ = self.args()
        assert 0 < n <= 211
        if self.busy:
            self.ret(0); return
        self.sent.append(bytes(self.u.mem_read(p, n))); self.ret(1)

    # Моделирует выдачу передающего буфера с учётом занятости транспорта.
    def acquire(self):
        self.acquisitions += 1
        self.write(self.args()[0], 1040, 2)
        self.ret(0 if self.busy else self.transport)

    # Фиксирует принятый транспортом кадр либо моделирует отказ.
    def commit(self):
        n = self.args()[0]
        assert 0 < n <= 211
        if self.commit_fail:
            self.ret(0); return
        self.sent.append(bytes(self.u.mem_read(self.transport, n))); self.ret(1)

    # Передаёт кадр внешней команды в связанную прошивку.
    def packet(self, cmd, idx=0, channel=0, uniq=73):
        body = bytes([idx]) if cmd == 18 else b''
        p = struct.pack('<HBBB', len(body)+3, 0, uniq, cmd) + body
        p += struct.pack('<H', crc16(p))
        self.u.mem_write(0x21000000, p)
        self.checked('user_parse_comand', 0x21000000, channel)
        assert self.read_budget <= 64 and not self.io_budget
        self.u.mem_write(0x21000000, b'\xa5' * len(p))

    # Читает признак незавершённой операции связанного кода.
    def active(self):
        return self.call('service_crc_active') != 0

    # Выполняет один проход хранения и проверяет его бюджет.
    def tick(self):
        self.now += 1
        before = self.u.reg_read(UC_ARM_REG_PRIMASK)
        self.checked('user_storage_poll')
        assert self.u.reg_read(UC_ARM_REG_PRIMASK) == before
        self.poll_costs.append(self.steps)
        assert self.read_budget <= 256, ('bounded CRC batch', self.read_budget)
        assert self.io_budget == 0 and self.steps < 100000

    # Проверяет возврат всей рабочей памяти после окончания.
    def restored_heap(self):
        assert owned(self)==0, 'service workspace leaked'
        p=take(self,REPLY);assert p
        drop(self,REPLY,p)

    # Создаёт независимый эталон результата сценария.
    def expected(self, cmd, idx=0, uniq=73):
        # Считает эталон CRC заданной области.
        def crc_region(mb, off=0, n=None):
            base, size = self.regions[mb]
            n = (1040 if mb==4 else size) if n is None else n
            return crc16(self.flash[base+off:base+off+n])
        if cmd == 17:
            payload = b''.join(struct.pack('<H', crc_region(mb)) for mb in (2, 3, 4, 6, 5))
            code = 5
        elif idx:
            payload = bytes([idx]) + struct.pack('<H', crc_region(2, (idx-1)*16000, 16000))
            code = 15
        else:
            payload = b''.join(struct.pack('<H', crc_region(2, off, 16000)) for off in range(0, 1600000, 16000))
            code = 6
        body = struct.pack('<HBBB', len(payload)+3, 255, uniq, code) + payload
        return b'SLDN' + body + struct.pack('<H', crc16(body))

    # Собирает ожидаемые байты ответа об ошибке.
    def expected_error(self, cmd, uniq=73):
        timestamp = int.from_bytes(self.u.mem_read(self.device+self.d['datetime'], 4), 'little')
        free = int.from_bytes(self.u.mem_read(self.device+self.d['freeBufferSize'], 2), 'little')
        body = struct.pack('<HBBBBBIH', 11, 255, uniq, 11, cmd, 2, timestamp, free)
        return b'SLDN' + body + struct.pack('<H', crc16(body))


# Проверяет точные ответы CRC и ограничения одного прохода.
def wire_and_budgets():
    result = {}
    for cmd, idx, used, total in ((17, 0, 0, 8192), (17, 0, 63, 8192),
        (17, 0, 64, 8192), (17, 0, 65, 8192), (17, 0, 4097, 8192),
        (17, 0, 8192, 1601536), (18, 0, 16000, 1601536),
        (18, 0, 16001, 1601536), (18, 1, 16000, 1601536),
        (18, 2, 16001, 1601536), (18, 100, 1600000, 1601536)):
        for channel in (0, 1):
            m = ServiceMachine(used, total)
            m.packet(cmd, idx, channel)
            assert m.active() and not m.sent
            assert not m.checked('user_storage_command_ready')
            assert not m.checked('user_storage_flash_ready')
            assert not m.acquisitions, 'transport held while reading flash'
            m.drain(60000)
            assert m.sent == [m.expected(cmd, idx)], (cmd, idx, used, channel)
            assert not m.answers and not m.counts['start']
            m.restored_heap()
            result[f'{cmd}:{idx}:{used}:{channel}'] = {
                'polls': len(m.poll_costs), 'max_poll_instructions': max(m.poll_costs),
                'total_poll_instructions': sum(m.poll_costs), 'stack': m.stack_peak,
                'read_bytes': sum(n for _, n in m.read_ranges)}
    return result


# Проверяет отказы обмена и время жизни ответа CRC.
def faults_and_lifetime():
    checks = 0
    for cmd, idx in ((17, 0), (18, 1)):
        for fail_at in (1, 2, 3):
            m = ServiceMachine(129, 1601536)
            m.fault = ('read', fail_at)
            m.packet(cmd, idx); m.drain()
            assert m.sent == [m.expected_error(cmd)] and not m.answers
            assert not m.counts['start']; m.restored_heap(); checks += 1
        m = ServiceMachine()
        p = take(m,REPLY); assert p
        before = bytes(m.flash)
        m.packet(cmd, idx)
        assert not m.active() and not m.sent and not m.read_ranges
        assert bytes(m.flash) == before and m.answers[-1] == (cmd, 2, 0, 73)
        drop(m,REPLY,p); m.restored_heap(); checks += 1

    for action, channel in (('cancel', 0), ('quiet', 0), ('reconnect', 1),
                            ('usb_reset', 0), ('reset', 0)):
        m = ServiceMachine(); m.packet(17, channel=channel); m.tick()
        if action == 'cancel': m.checked('service_crc_cancel')
        elif action == 'quiet': m.checked('service_crc_quiet', channel)
        elif action == 'reconnect': m.checked('user_comm_session', 0)
        elif action == 'usb_reset': m.write(m.addr('usb_bus_resets'), 1)
        else: m.checked('user_savereset')
        m.drain(); assert not m.sent and not m.timestamps
        assert not m.counts['start']; m.restored_heap(); checks += 1

    for channel in (0, 1):
        m = ServiceMachine(); m.packet(17, channel=channel); m.busy = True
        for _ in range(200): m.tick()
        assert m.active() and not m.sent
        m.busy = False; m.drain(); assert m.sent == [m.expected(17)]
        m.restored_heap(); checks += 1

    m = ServiceMachine(); m.packet(17, channel=1); m.commit_fail = True
    for _ in range(200): m.tick()
    assert m.active() and not m.sent
    m.commit_fail = False; m.drain(); assert m.sent == [m.expected(17)]
    m.restored_heap(); checks += 1

    # Владелец таблицы отклоняет CRC до обмена; после возврата памяти запрос проходит.
    m = ServiceMachine()
    held=take(m,TABLE);assert held
    m.packet(17);assert not m.active() and not m.sent
    drop(m,TABLE,held);m.restored_heap();checks += 1

    m = ServiceMachine(); m.packet(17)
    m.checked('service_crc_quiet', 1)
    m.u.reg_write(UC_ARM_REG_PRIMASK, 1)
    m.drain(); assert m.sent == [m.expected(17)]
    assert m.u.reg_read(UC_ARM_REG_PRIMASK) == 1
    m.restored_heap(); checks += 1

    m = ServiceMachine(); m.packet(18, 101)
    assert not m.active() and not m.sent and not m.read_ranges
    assert m.answers[-1] == (18, 1, 0, 73); checks += 1
    return checks


# Сверяет CRC и байты ответа на сохранение настроек.
def config_reply():
    m = ServiceMachine(); s = fields('store_state_s')
    base = m.addr('store')
    for name, value in (('job', 8), ('cmd', 12), ('channel', 0), ('uniq', 37)):
        m.write(base + s[name], value, 1)
    m.write(m.addr('config') + fields('config_s')['datetime'], 123456789)
    held = take(m,REPLY); assert held
    m.tick(); assert not m.active() and not m.sent
    assert int.from_bytes(m.u.mem_read(base+s['job'], 1), 'little') == 8
    drop(m,REPLY,held)
    m.tick(); assert m.active() and not m.timestamps
    m.busy = True
    for _ in range(50): m.tick()
    assert m.active() and not m.sent
    m.busy = False
    m.drain()
    body = struct.pack('<HBBBBIH', 10, 255, 37, 12, 4, 123456789, crc16(m.flash[0x1cd400:0x1cd800]))
    assert m.sent == [b'SLDN' + body + struct.pack('<H', crc16(body))]
    assert not m.timestamps
    m.restored_heap()
    return {'polls': len(m.poll_costs), 'max_poll': max(m.poll_costs), 'stack': m.stack_peak}


# Проверяет, что CRC читает активный слот вместо старого места.
def config_cached_source():
    m = ServiceMachine()
    # Старое место может отличаться: действующим остаётся проверенный слот A/B.
    m.flash[0x1f5000:0x1f5400] = bytes((i*31+7)&255 for i in range(1024))
    m.write(m.addr('cfg_cached_active'), 0x1f5000)
    m.regions[6] = (0x1f5000, 1024)
    m.packet(17); m.drain()
    assert m.sent == [m.expected(17)]
    assert any(0x1f5000 <= a < 0x1f5400 for a, _ in m.read_ranges)
    assert not any(0x1f6000 <= a < 0x1f9000 for a, _ in m.read_ranges), 'hidden config slot selection'
    m.restored_heap()
    return 'PASS'


# Сверяет преобразование CRC пустого хвоста для всех битов длины.
def ff_affine():
    # Независимый побитовый эталон поднимается в однородное 17-битное
    # преобразование: проверяем все 32 бита длины без массива 4 ГиБ.
    def apply(cols, value):
        result = 0
        for bit, column in enumerate(cols):
            if value & (1 << bit): result ^= column
        return result
    constant = crc16(b'\xff', 0)
    powers = [[crc16(b'\xff', 1 << b) ^ constant for b in range(16)] + [constant | 65536]]
    for _ in range(31): powers.append([apply(powers[-1], c) for c in powers[-1]])
    # Создаёт независимый эталон результата сценария.
    def expected(seed, length):
        value = seed | 65536
        for bit in range(32):
            if length & (1 << bit): value = apply(powers[bit], value)
        return value & 65535
    rng = random.Random(204805)
    lengths = [0, 1, 63, 64, 65, 16000, 1601536, 0xffffffff]
    lengths += [1 << b for b in range(32)]
    lengths += [rng.randrange(0x100000000) for _ in range(32)]
    maximum = 0
    for length in lengths:
        m = ServiceMachine(); assert m.checked('service_crc_begin_config', 0, 73)
        job = int.from_bytes(m.u.mem_read(m.addr('service_crc_job'), 4), 'little')
        seed = rng.randrange(65536); offsets=fields('service_crc_s')
        m.write(job + offsets['real'], 0); m.write(job + offsets['erased'], length); m.write(job + offsets['crc'], seed, 2)
        m.write(job + offsets['phase'], 2, 1); m.write(job + offsets['waiting'], 0, 1)
        passes = 0
        while int.from_bytes(m.u.mem_read(job + offsets['phase'], 1), 'little') == 2:
            m.tick(); passes += 1; assert passes <= 33
        assert int.from_bytes(m.u.mem_read(job + offsets['crc'], 2), 'little') == expected(seed, length), (seed, length)
        maximum = max(maximum, max(m.poll_costs)); assert not m.read_ranges
        m.checked('service_crc_cancel'); m.restored_heap()
    return {'cases': len(lengths), 'maximum_poll_instructions': maximum, 'maximum_passes': 33}


# Проверяет ограничение ожидания недоступной W25.
def pending_timeout():
    for start in (100, 0xfffffff0):
        m = ServiceMachine(); m.packet(17)
        m.now = start
        m.stubs[m.addr('w25_read_available')] = lambda: m.ret(0)
        m.tick(); m.tick()  # Подготовка; первая недоступность начинает отсчёт ожидания.
        m.now += 5998; m.tick()
        assert m.active() and not m.answers and not m.read_ranges
        m.now += 1; m.tick(); m.drain()
        assert m.sent == [m.expected_error(17)] and not m.answers
        m.restored_heap()
    m = ServiceMachine(); m.packet(17)
    m.stubs[m.addr('w25_read_available')] = lambda: m.ret(0)
    for _ in range(5): m.tick()
    assert m.active() and not m.answers and not m.read_ranges
    m.stubs[m.addr('w25_read_available')] = lambda: m.ret(1)
    m.drain(); assert m.sent == [m.expected(17)]; m.restored_heap()
    return 3


# Проверяет CRC через настоящие чтения драйвера W25.
def real_w25():
    """Выполняет связанный драйвер W25; модель заменяет только SPI, NOR и транспорт."""
    m = ServiceMachine(8192, 1601536)
    # Unicorn ошибается на цикле IAR memset с включённым MEM_WRITE
    # в этой среде; это воспроизводится даже с пустым обработчиком.
    # Выдаём контекст до установки перехвата выбора SPI; последующие
    # шаги выполняют реальный W25, а вход команды измерен отдельно выше.
    m.packet(17)
    m.write(0x40013000, 0x40, 2)
    wire = {'cmd': 0, 'address': 0, 'address_bytes': 0, 'read': 0, 'status': 0}
    for name in ('w25_read_data', 'w25_async_start', 'w25_async_poll',
                 'w25_async_active', 'w25_read_available'):
        del m.stubs[m.addr(name)]
    # Моделирует границу выбора SPI-чипа и завершение команды NOR.
    def cs(u, access, address, size, value, unused):
        if address == 0x40010c14 and value == 4096:
            wire.update(cmd=0, address=0, address_bytes=0)
    # Моделирует команду, адрес и данные одного обмена SPI.
    def send_byte():
        value = m.args()[0] & 255
        if not wire['cmd']: wire['cmd'] = value
        elif wire['cmd'] == 3 and wire['address_bytes'] < 3:
            wire['address'] = (wire['address'] << 8) | value
            wire['address_bytes'] += 1
        else: assert False, ('unexpected SPI command', wire)
        m.ret()
    # Моделирует один байт ответа SPI.
    def read_byte():
        assert wire['cmd'] == 5
        wire['status'] += 1; m.ret(0)
    # Читает модельную NOR с контролем диапазона и заданным отказом.
    def read():
        p, n, _, _ = m.args()
        assert wire['cmd'] == 3 and n <= 64
        a = wire['address']; m.u.mem_write(p, bytes(m.flash[a:a+n]))
        wire['address'] += n; wire['read'] += n; m.ret()
    m.u.hook_add(UC_HOOK_MEM_WRITE, cs, begin=0x40010c10, end=0x40010c14)
    for name, fn in (('spi_send_byte', send_byte), ('spi_read_byte', read_byte), ('spi_read', read)):
        m.stubs[m.addr(name)] = fn
    max_read = max_status = total_read = 0
    for _ in range(10000):
        if not m.active(): break
        wire['read'] = wire['status'] = 0
        m.tick()
        max_read = max(max_read, wire['read']); max_status = max(max_status, wire['status'])
        total_read += wire['read']
        assert wire['read'] <= 256 and wire['status'] <= 16
    else: assert False, 'real driver service did not terminate'
    assert m.sent == [m.expected(17)]
    m.restored_heap()
    return {'polls': len(m.poll_costs),
            'maximum_poll_instructions': max(m.poll_costs),
            'total_poll_instructions': sum(m.poll_costs), 'stack': m.stack_peak,
            'maximum_spi_read_bytes': max_read, 'maximum_status_reads': max_status,
            'total_spi_read_bytes': total_read}


# Проверяет запрет CRC и записи при невосстановленном журнале.
def unrecovered_storage():
    # Ошибка старта оставляет таблицы закрытыми: внутренний запрос не повторяет восстановление.
    m=ServiceMachine();m.write(m.addr('mem_storage_ready'),0,1);m.write(m.addr('store_runtime'),0,1)
    reads=m.counts['read'];assert not m.checked('mem_tables_ready') and m.counts['read']==reads
    for mode in ('not-recovered', 'journal-fault'):
        m = ServiceMachine()
        if mode == 'not-recovered': m.write(m.addr('mem_storage_ready'), 0, 1)
        else: m.write(m.addr('sjr')+59, 1, 1)
        m.packet(17)
        assert not m.active() and not m.sent and not m.read_ranges
        assert m.answers[-1] == (17, 2, 0, 73) and not m.counts['start']
        m.restored_heap()
    return 2


if __name__ == '__main__':
    p = argparse.ArgumentParser(); p.add_argument('--output', type=Path)
    args = p.parse_args()
    out = {'units': 'Unicorn instructions; modeled NOR; NOT cycles or milliseconds',
           'elf_sha256': hashlib.sha256((ROOT/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest()}
    out['wire'] = wire_and_budgets(); print('service wire/ownership/budgets PASS', flush=True)
    out['faults_lifetime'] = faults_and_lifetime(); print('service faults/OOM/cancel/session/reset PASS', flush=True)
    out['config_reply'] = config_reply(); print('service config timestamp PASS', flush=True)
    out['config_cached_source'] = config_cached_source(); print('service cached config source PASS', flush=True)
    out['ff_affine'] = ff_affine(); print('service FF affine all 32 length bits PASS', flush=True)
    out['pending_timeout'] = pending_timeout(); print('service pending/recovery/wrapped timeout PASS', flush=True)
    out['real_w25'] = real_w25(); print('service linked W25/SPI boundary PASS', flush=True)
    out['unrecovered_storage'] = unrecovered_storage(); print('service unrecovered storage rejected PASS', flush=True)
    if args.output: args.output.write_text(json.dumps(out, indent=2)+'\n')
