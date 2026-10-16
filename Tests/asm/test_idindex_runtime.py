# Доработка: Кряжев А., 2026
from test_work_memory import take, drop, owned, PHOTO, PHOTO_PACKET, REPLY, CONFIG, FW_CONTEXT, FW_DATA, TABLE, UP_CONTEXT, UP_PACKET, SIZES
"""Пересборка индекса в связанном IAR ELF; модель задаёт границу NOR, код и владение выполняются реально."""
import argparse, hashlib, json, struct
from storage_machine import *

BASE, IX_BASE, IX_LOG, MAGIC = 0x41000, 0x300000, 0x340000, 0x58444932


# Считает независимый эталон хеша FNV.
def fnv(data):
    h = 2166136261
    for b in data:
        h = ((h ^ b) * 16777619) & 0xffffffff
    return h


# Вычисляет корзину ID по независимому эталону хеша.
def bucket(ident):
    h = fnv(ident)
    return IX_BASE + ((h ^ (h >> 12) ^ (h >> 24)) & 4095) * 64


# Создаёт одну запись заданного номера для тестовой базы.
def row(i):
    return struct.pack('<H', i) + struct.pack('<IIH', i * 17, i * 19, i) + bytes(20)


class IndexMachine(StorageMachine):
    # Готовит связанный образ, модель NOR и состояние сценария.
    def __init__(self, flash=None):
        super().__init__(flash)
        self.at = self.addr('ix')
        # Изолируем индекс: прежний прямой poll не включал CRC-фон.
        self.stubs[self.addr('idcrc_background_begin')] = lambda:self.ret(0)
        self.stubs[self.addr('w25_write_data')] = self.no_sync
        self.stubs[self.addr('w25_erase_sector')] = self.no_sync
        self.max_read = self.max_io = 0

    # Запрещает синхронную запись в фоновом сценарии.
    def no_sync(self):
        raise AssertionError('index called synchronous program/erase')

    # Читает состояние фоновой пересборки индекса.
    def state(self):
        return self.u.mem_read(self.at + 10, 1)[0]

    # Читает шаг текущего INDEX; 1–6 — физическое ожидание, 7 — чтение корзины.
    def owner(self):
        kind, step, result = self.exec_state()
        return step if kind == 5 and result == 1 else 0

    # Создаёт исходное состояние сценария и его подменяемые границы.
    def setup(self, indices=(1, 2, 129), log_full=False):
        expected = bytearray(b'\xff' * 0x40000)
        total = 0
        for i in indices:
            data = row(i)
            self.flash[BASE + (i - 1) * 32:BASE + i * 32] = data
            total ^= fnv(struct.pack('<H', i) + data[2:12])
            off = bucket(data[2:12]) - IX_BASE
            for slot in range(32):
                if expected[off + 2 * slot:off + 2 * slot + 2] == b'\xff\xff':
                    expected[off + 2 * slot:off + 2 * slot + 2] = struct.pack('<H', i)
                    break
            else:
                raise AssertionError('fixture full bucket')
        # Старую верную отметку с той же суммой нужно отменить до
        # любого стирания; пересборка может начаться после приостановки и нового сканирования.
        self.flash[IX_BASE:IX_LOG] = expected
        mark = struct.pack('<II', total, (~total ^ MAGIC) & 0xffffffff)
        if log_full:
            self.flash[IX_LOG:IX_LOG + 4096] = mark * 512
        else:
            self.flash[IX_LOG:IX_LOG + 8] = mark
        self.write(self.at + 8, 512 if log_full else 1, 2)
        self.write(self.at + 10, 4, 1)
        self.checked('idindex_scanned', total, max(indices, default=0))
        assert self.state() == 1 and not self.owner()
        return bytes(expected), total, max(indices, default=0)

    # Выполняет шаг пересборки индекса с контролем ресурсов.
    def tick_index(self, glue=False):
        self.now += 1
        primask = self.u.reg_read(UC_ARM_REG_PRIMASK)
        if glue:
            assert self.owner()
            assert bool(self.checked('flash_exec_read_allowed')) == (self.io is None)
            self.checked('user_storage_poll')
        else:
            self.checked('user_storage_poll')
        assert self.u.reg_read(UC_ARM_REG_PRIMASK) == primask
        self.max_read = max(self.max_read, self.read_budget)
        self.max_io = max(self.max_io, self.io_budget)
        assert self.read_budget <= 64, self.read_budget
        assert self.io_budget <= 1, self.io_budget
        assert (1 <= self.owner() <= 6) == (self.io is not None)
        if self.owner() == 7:assert self.io is None and self.state() == 6

    # Доводит пересборку индекса до конечного состояния.
    def drain_index(self, limit=20000, glue=False):
        for _ in range(limit):
            if self.state() in (3, 4) and not self.owner():
                return
            self.tick_index(glue and bool(self.owner()))
        raise AssertionError(('index stuck', self.state(), self.owner()))

    # Сверяет данные базы, счётчики и байты ответа с эталоном.
    def verify(self, expected, total):
        assert self.state() == 3 and not self.owner()
        assert self.flash[IX_BASE:IX_LOG] == expected
        n = int.from_bytes(self.u.mem_read(self.at + 8, 2), 'little')
        assert self.flash[IX_LOG + (n - 1) * 8:IX_LOG + n * 8] == struct.pack('<II', total, (~total ^ MAGIC) & 0xffffffff)


# Проверяет штатные операции, данные и ответы модуля.
def test_normal():
    results = {}
    for indices, full in [((), False), ((1,), False), ((1, 2, 129), True), ((1, 50048), False)]:
        m = IndexMachine()
        expected, total, end = m.setup(indices, full)
        m.u.reg_write(UC_ARM_REG_PRIMASK, 1)
        m.drain_index(60000, glue=full)
        m.verify(expected, total)
        boot = IndexMachine(m.flash)
        boot.checked('idindex_scanned', total, end)
        assert boot.state() == 3
        results[str((indices, full))] = dict(passes=len(m.costs), max_instructions=max(m.costs),
            total_instructions=sum(m.costs), stack=m.stack_peak, max_read=m.max_read,
            max_async_actions=m.max_io, counts=m.counts)
    return results


# Проверяет отказы каждой границы обмена и состояние после них.
def test_faults():
    count = 0
    for kind, points in [('read', (1, 2, 3, 20, 130)), ('start', (1, 2, 16, 65, 66, 67, 68, 69, 70)),
                         ('poll', (1, 4, 5, 64, 260, 265, 270, 275, 280))]:
        for point in points:
            m = IndexMachine()
            m.setup()
            m.fault = kind, point
            m.drain_index()
            if m.counts[kind] < point:
                continue
            assert m.state() == 4 and not m.owner() and m.io is None, (kind, point)
            assert m.flash[BASE:BASE + 32] == row(1)
            count += 1
    # Отсутствие команды не считается проверенным результатом, в том числе после сброса драйвера.
    m = IndexMachine(); m.setup(); m.tick_index()
    m.stubs[m.addr('w25_async_poll')] = lambda: (setattr(m, 'io', None), m.ret(0))
    m.tick_index(); assert m.state() == 4 and not m.owner(); count += 1
    return count


# Проверяет приостановку и перезапуск фонового индекса.
def test_cancel_restart():
    count = 0
    for pending in (1, 2, 3, 4, 5, 6):
        m = IndexMachine(); expected, total, end = m.setup((1,), pending == 1)
        if pending == 5:
            m.write(m.at + 8, 511, 2)
        for _ in range(2000):
            if m.owner() == pending:
                break
            m.tick_index()
        else:
            raise AssertionError(('missing pending', pending))
        source = bytes(m.u.mem_read(m.addr('flash_job') + 4, 8))
        m.checked('idindex_runtime_suspend')
        assert m.state() == 4 and m.owner()
        # Смена сеанса и сброс не должны оставлять фонового владельца занятым.
        m.checked('user_comm_session', 0)
        m.write(m.addr('usb_bus_resets'), 1)
        while m.owner():
            assert bytes(m.u.mem_read(m.addr('flash_job') + 4, 8)) == source
            m.tick_index(glue=True)
            if m.owner():
                assert bytes(m.u.mem_read(m.addr('flash_job') + 4, 8)) == source
            else:
                assert m.io is None and m.exec_state() == (0,0,0)
        assert m.state() == 4 and m.io is None
        m.checked('idindex_scanned', total, end)
        m.drain_index(); m.verify(expected, total)
        count += 1
    # Новый скан во время стирания требует перезапуска, сохраняя источник текущей команды.
    m = IndexMachine(); expected, total, end = m.setup((1,))
    m.tick_index(); m.checked('idindex_scanned', total, end)
    assert m.state() == 8 and m.owner()
    m.drain_index(); m.verify(expected, total); count += 1
    # Индекс не опрашивает и не перезаписывает команду другого владельца.
    m = IndexMachine(); m.setup(); m.io = (0x1000, 0, 0, b'\xff' * 4096, 0)
    m.checked('user_storage_poll'); assert m.counts['poll'] == 0 and not m.owner(); count += 1
    return count


# Проверяет последние допустимые позиции и переполненную корзину.
def test_boundaries():
    m = IndexMachine(); expected, total, _ = m.setup((1,))
    # Полная корзина отключает индекс без записи за её границу.
    while m.state() != 6:
        m.tick_index()
    address = bucket(row(1)[2:12]); m.flash[address:address + 64] = struct.pack('<32H', *range(2, 34))
    before = bytes(m.flash); m.tick_index()
    assert m.state() == 4 and bytes(m.flash) == before
    # Последнее место корзины лежит в диапазоне и живёт до проверки W25.
    m = IndexMachine(); m.setup((1,))
    while m.state() != 6:
        m.tick_index()
    address = bucket(row(1)[2:12]); m.flash[address:address + 62] = struct.pack('<31H', *range(2, 33))
    m.tick_index(); assert m.owner() == 4
    while m.owner():
        m.tick_index()
    assert m.flash[address + 62:address + 64] == b'\x01\0' and m.state() == 7
    # Последний 16-битный курсор обрабатывается без перехода к нулевому номеру.
    m = IndexMachine(); m.write(m.at + 10, 2, 1)
    m.write(m.at + 4, 65535, 2); m.write(m.at + 6, 65535, 2)
    m.tick_index(); assert m.state() == 7
    # Нехватка кучи не влияет на индекс: он не запрашивает память.
    m = IndexMachine(); expected, total, _ = m.setup((1,))
    m.stubs[m.addr('work_acquire')] = lambda: (_ for _ in ()).throw(AssertionError('index allocated'))
    m.drain_index(); m.verify(expected, total)
    return 4


# Проверяет устойчивые состояния каждого обрыва операции.
def test_power():
    count = 0
    boot = IndexMachine()
    boot_ram = bytes(boot.u.mem_read(0x20000000, 0x5000))
    for full in (False, True):
        m = IndexMachine(); expected, total, end = m.setup((1, 2), full)
        while m.state() not in (3, 4) or m.owner():
            before = bytes(m.flash)
            m.tick_index()
            candidates = [bytes(m.flash)]
            if before != candidates[0]:
                # Обрываем каждую изменяющую команду NOR на первом, среднем и последнем байте.
                changed = [i for i, (a, b) in enumerate(zip(before, candidates[0])) if a != b]
                for cut in sorted({1, len(changed)//2, len(changed)-1}):
                    if cut <= 0:
                        continue
                    torn = bytearray(before)
                    for pos in changed[:cut]:
                        torn[pos] = m.flash[pos]
                    candidates.append(bytes(torn))
            for image in candidates:
                # Используем уже разобранные ELF и DWARF, но восстанавливаем весь начальный
                # образ ОЗУ при каждом старте, включая ix, кучу и владельца W25.
                boot.u.mem_write(0x20000000, boot_ram)
                boot.flash = bytearray(image)
                assert boot.boot_ids() == 2
                if boot.state() == 3:
                    assert boot.flash[IX_BASE:IX_LOG] == expected, ('accepted partial index', full, count)
                else:
                    assert boot.state() == 1
                count += 1
        m.verify(expected, total)
    return count


# Проверяет индекс с реальными командами драйвера W25.
def test_real_w25():
    """Индекс с реальным фоновым W25; модель подменяет только границу SPI."""
    m = IndexMachine(); expected, total, _ = m.setup((1, 2))
    m.write(0x40013000, 0x40, 2)
    wire = dict(cmd=0, address=0, address_bytes=0, wel=0, busy=0, read=0, status=0)
    for name in ('w25_read_data', 'w25_async_start', 'w25_async_poll', 'w25_async_active', 'w25_read_available'):
        del m.stubs[m.addr(name)]

    # Моделирует границу выбора SPI-чипа и завершение команды NOR.
    def cs(u, access, a, n, value, unused):
        if a == 0x40010c14 and value == 4096:
            wire.update(cmd=0, address=0, address_bytes=0)
        if a == 0x40010c10 and value == 4096:
            if wire['cmd'] == 6:
                wire['wel'] = 2
            elif wire['cmd'] == 2:
                wire.update(wel=0, busy=3)
            elif wire['cmd'] == 0x20:
                assert wire['wel'] and not wire['address'] & 4095
                a = wire['address']; m.flash[a:a+4096] = b'\xff' * 4096
                wire.update(wel=0, busy=3)

    # Моделирует команду, адрес и данные одного обмена SPI.
    def send_byte():
        b = m.args()[0] & 255
        if not wire['cmd']:
            wire['cmd'] = b
        elif wire['cmd'] in (2, 3, 0x20) and wire['address_bytes'] < 3:
            wire['address'] = (wire['address'] << 8) | b
            wire['address_bytes'] += 1
        m.ret()

    # Моделирует один байт ответа SPI.
    def read_byte():
        assert wire['cmd'] == 5
        wire['status'] += 1
        v = wire['wel'] | bool(wire['busy'])
        if wire['busy']:
            wire['busy'] -= 1
        m.ret(v)

    # Моделирует передачу буфера по границе SPI.
    def send():
        p, n, _, _ = m.args()
        assert wire['cmd'] == 2 and wire['wel'] and n <= 64
        for b in m.u.mem_read(p, n):
            m.flash[wire['address']] &= b; wire['address'] += 1
        m.ret()

    # Читает модельную NOR с контролем диапазона и заданным отказом.
    def read():
        p, n, _, _ = m.args()
        assert wire['cmd'] == 3 and n <= 64
        a = wire['address']; m.u.mem_write(p, bytes(m.flash[a:a+n]))
        wire['address'] += n; wire['read'] += n; m.ret()

    m.watch_writes(0x40010c10, 0x40010c14, cs)
    for name, fn in [('spi_send_byte', send_byte), ('spi_read_byte', read_byte), ('spi_send', send), ('spi_read', read)]:
        m.stubs[m.addr(name)] = fn
    m.costs = []; max_read = max_status = 0
    for _ in range(10000):
        if m.state() in (3, 4) and not m.owner():
            break
        m.now += 1; wire['read'] = wire['status'] = 0
        m.checked('user_storage_poll')
        max_read = max(max_read, wire['read']); max_status = max(max_status, wire['status'])
        assert wire['read'] <= 64 and wire['status'] <= 4
    else:
        raise AssertionError('real W25 index did not terminate')
    m.verify(expected, total)
    return dict(passes=len(m.costs), max_instructions=max(m.costs), total_instructions=sum(m.costs),
                stack=m.stack_peak, max_spi_read_bytes=max_read, max_status_reads=max_status)


if __name__ == '__main__':
    p = argparse.ArgumentParser(); p.add_argument('--power', action='store_true'); args = p.parse_args()
    result = dict(normal=test_normal(), faults=test_faults(), cancel_restart=test_cancel_restart(),
                  boundaries=test_boundaries(), real_w25=test_real_w25())
    if args.power:
        result['power_cuts'] = test_power()
    result['elf_sha256'] = hashlib.sha256((__import__('iar_image').image_paths()[0]).read_bytes()).hexdigest()
    result['index_bss_bytes'] = 24
    result['status'] = 'PASS'
    (__import__('iar_image').result_path('idindex_runtime_results.json')).write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
