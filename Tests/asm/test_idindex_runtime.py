"""Handwritten index rebuild in the linked IAR ELF; no physical hardware.

NOR operations use StorageMachine's delayed verified/error boundary. All index
logic, ownership glue, hashes, caller ABI and cancellation execute as Thumb-2.
"""
import argparse, hashlib, json, struct
from storage_machine import *

BASE, IX_BASE, IX_LOG, MAGIC = 0x41000, 0x300000, 0x340000, 0x58444931


def fnv(data):
    h = 2166136261
    for b in data:
        h = ((h ^ b) * 16777619) & 0xffffffff
    return h


def bucket(ident):
    h = fnv(ident)
    return IX_BASE + ((h ^ (h >> 12) ^ (h >> 24)) & 4095) * 64


def row(i):
    return struct.pack('<H', i) + struct.pack('<IIH', i * 17, i * 19, i) + bytes(20)


class IndexMachine(StorageMachine):
    def __init__(self, flash=None):
        super().__init__(flash)
        self.at = self.addr('ix')
        self.stubs[self.addr('w25_write_data')] = self.no_sync
        self.stubs[self.addr('w25_erase_sector')] = self.no_sync
        self.max_read = self.max_io = 0

    def no_sync(self):
        raise AssertionError('index called synchronous program/erase')

    def state(self):
        return self.u.mem_read(self.at + 10, 1)[0]

    def owner(self):
        return self.u.mem_read(self.at + 11, 1)[0]

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
        # A previous valid cache with the SAME sum must be invalidated before
        # any erase. Rebuild may be requested after a runtime suspend/scan.
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

    def tick_index(self, glue=False):
        self.now += 1
        primask = self.u.reg_read(UC_ARM_REG_PRIMASK)
        if glue:
            assert self.owner()
            assert not self.checked('user_storage_flash_ready')
            assert not self.checked('mem_tables_ready')
            self.checked('user_storage_poll')
        else:
            self.checked('idindex_poll', BASE)
        assert self.u.reg_read(UC_ARM_REG_PRIMASK) == primask
        self.max_read = max(self.max_read, self.read_budget)
        self.max_io = max(self.max_io, self.io_budget)
        assert self.read_budget <= 64, self.read_budget
        assert self.io_budget <= 1, self.io_budget
        assert bool(self.owner()) == (self.io is not None)

    def drain_index(self, limit=20000, glue=False):
        for _ in range(limit):
            if self.state() in (3, 4) and not self.owner():
                return
            self.tick_index(glue and bool(self.owner()))
        raise AssertionError(('index stuck', self.state(), self.owner()))

    def verify(self, expected, total):
        assert self.state() == 3 and not self.owner()
        assert self.flash[IX_BASE:IX_LOG] == expected
        n = int.from_bytes(self.u.mem_read(self.at + 8, 2), 'little')
        assert self.flash[IX_LOG + (n - 1) * 8:IX_LOG + n * 8] == struct.pack('<II', total, (~total ^ MAGIC) & 0xffffffff)


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
    # Idle is not a verified result, including an unexpected driver reset.
    m = IndexMachine(); m.setup(); m.tick_index()
    m.stubs[m.addr('w25_async_poll')] = lambda: (setattr(m, 'io', None), m.ret(0))
    m.tick_index(); assert m.state() == 4 and not m.owner(); count += 1
    return count


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
        source = bytes(m.u.mem_read(m.at + 12, 8))
        m.checked('idindex_runtime_suspend')
        assert m.state() == 4 and m.owner()
        # Reconnect/reset cannot strand this background owner.
        m.checked('user_comm_session', 0)
        m.write(m.addr('usb_bus_resets'), 1)
        while m.owner():
            m.tick_index(glue=True)
            assert bytes(m.u.mem_read(m.at + 12, 8)) == source
        assert m.state() == 4 and m.io is None
        m.checked('idindex_scanned', total, end)
        m.drain_index(); m.verify(expected, total)
        count += 1
    # Re-scan in an outstanding erase requests restart, retaining its source.
    m = IndexMachine(); expected, total, end = m.setup((1,))
    m.tick_index(); m.checked('idindex_scanned', total, end)
    assert m.state() == 8 and m.owner()
    m.drain_index(); m.verify(expected, total); count += 1
    # Another owner is never polled or overwritten by the index.
    m = IndexMachine(); m.setup(); m.io = (0x1000, 0, 0, b'\xff' * 4096, 0)
    m.checked('idindex_poll', BASE); assert m.counts['poll'] == 0 and not m.owner(); count += 1
    return count


def test_boundaries():
    m = IndexMachine(); expected, total, _ = m.setup((1,))
    # A full collision bucket must turn off without writing an out-of-range slot.
    while m.state() != 6:
        m.tick_index()
    address = bucket(row(1)[2:12]); m.flash[address:address + 64] = struct.pack('<32H', *range(2, 34))
    before = bytes(m.flash); m.tick_index()
    assert m.state() == 4 and bytes(m.flash) == before
    # The last available bucket slot is in range and remains live until verified.
    m = IndexMachine(); m.setup((1,))
    while m.state() != 6:
        m.tick_index()
    address = bucket(row(1)[2:12]); m.flash[address:address + 62] = struct.pack('<31H', *range(2, 33))
    m.tick_index(); assert m.owner() == 4
    while m.owner():
        m.tick_index()
    assert m.flash[address + 62:address + 64] == b'\x01\0' and m.state() == 7
    # Last 16-bit cursor is consumed without wrapping into index zero.
    m = IndexMachine(); m.write(m.at + 10, 2, 1)
    m.write(m.at + 4, 65535, 2); m.write(m.at + 6, 65535, 2)
    m.tick_index(); assert m.state() == 7
    # OOM cannot affect the index: no allocation is requested at any point.
    m = IndexMachine(); expected, total, _ = m.setup((1,))
    m.stubs[m.addr('__basic_malloc')] = lambda: (_ for _ in ()).throw(AssertionError('index allocated'))
    m.drain_index(); m.verify(expected, total)
    return 4


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
                # Tear first/half/last byte of every changed NOR operation.
                changed = [i for i, (a, b) in enumerate(zip(before, candidates[0])) if a != b]
                for cut in sorted({1, len(changed)//2, len(changed)-1}):
                    if cut <= 0:
                        continue
                    torn = bytearray(before)
                    for pos in changed[:cut]:
                        torn[pos] = m.flash[pos]
                    candidates.append(bytes(torn))
            for image in candidates:
                # Reuse decoded ELF/debug metadata; restore the complete initial
                # RAM image for every reboot, including ix, heap and W25 owner.
                boot.u.mem_write(0x20000000, boot_ram)
                boot.flash = bytearray(image)
                boot.checked('idindex_scanned', total, end)
                if boot.state() == 3:
                    assert boot.flash[IX_BASE:IX_LOG] == expected, ('accepted partial index', full, count)
                else:
                    assert boot.state() == 1
                count += 1
        m.verify(expected, total)
    return count


def test_real_w25():
    """Index + the real async W25 driver, modeled only at the SPI boundary."""
    from unicorn import UC_HOOK_MEM_WRITE
    m = IndexMachine(); expected, total, _ = m.setup((1, 2))
    m.write(0x40013000, 0x40, 2)
    wire = dict(cmd=0, address=0, address_bytes=0, wel=0, busy=0, read=0, status=0)
    for name in ('w25_read_data', 'w25_async_start', 'w25_async_poll', 'w25_async_active', 'w25_read_available'):
        del m.stubs[m.addr(name)]

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

    def send_byte():
        b = m.args()[0] & 255
        if not wire['cmd']:
            wire['cmd'] = b
        elif wire['cmd'] in (2, 3, 0x20) and wire['address_bytes'] < 3:
            wire['address'] = (wire['address'] << 8) | b
            wire['address_bytes'] += 1
        m.ret()

    def read_byte():
        assert wire['cmd'] == 5
        wire['status'] += 1
        v = wire['wel'] | bool(wire['busy'])
        if wire['busy']:
            wire['busy'] -= 1
        m.ret(v)

    def send():
        p, n, _, _ = m.args()
        assert wire['cmd'] == 2 and wire['wel'] and n <= 64
        for b in m.u.mem_read(p, n):
            m.flash[wire['address']] &= b; wire['address'] += 1
        m.ret()

    def read():
        p, n, _, _ = m.args()
        assert wire['cmd'] == 3 and n <= 64
        a = wire['address']; m.u.mem_write(p, bytes(m.flash[a:a+n]))
        wire['address'] += n; wire['read'] += n; m.ret()

    m.u.hook_add(UC_HOOK_MEM_WRITE, cs)
    for name, fn in [('spi_send_byte', send_byte), ('spi_read_byte', read_byte), ('spi_send', send), ('spi_read', read)]:
        m.stubs[m.addr(name)] = fn
    m.costs = []; max_read = max_status = 0
    for _ in range(10000):
        if m.state() in (3, 4) and not m.owner():
            break
        m.now += 1; wire['read'] = wire['status'] = 0
        m.checked('user_storage_poll' if m.owner() else 'idindex_poll', BASE)
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
    result['elf_sha256'] = hashlib.sha256((ROOT/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest()
    result['index_bss_bytes'] = 24
    result['status'] = 'PASS'
    (ROOT/'Tests/asm/idindex_runtime_results.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))
