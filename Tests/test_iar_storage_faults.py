"""Доработка: Кряжев А., 2026. Отказы Flash на машинном коде поиска ID и фото."""
import struct
from iar_irq_support import Machine, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2
from iar_layout import fields

m = Machine()
m.init()
offsets = fields('device_s')
device = m.addr('device')
# Storage map normally populated by user_init; peripherals remain stubbed.
for slot, start, size in ((2, 266240, 1601536), (8, 1999872, 49152), (9, 2097152, 1048576)):
    m.u.mem_write(m.addr('mem') + slot * 8, struct.pack('<II', start, size))
number = b'1234567890'
record = struct.pack('<H', 7) + number + bytes(20)
read_ok = False
read_data = record
write_ok = False
writes = []


def memory_read():
    p, n = (m.u.reg_read(r) for r in (UC_ARM_REG_R1, UC_ARM_REG_R2))
    # Even a full transfer is invalid when the peripheral reports failure.
    m.u.mem_write(p, (read_data + b'\xff' * n)[:n])
    m.ret(int(read_ok))


def memory_write():
    a, p, n = (m.u.reg_read(r) for r in (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2))
    writes.append((a, bytes(m.u.mem_read(p, n))))
    m.ret(int(write_ok))


def value(name):
    return int.from_bytes(m.u.mem_read(device + offsets[name], 4), 'little')


m.stubs[m.addr('mem_read')] = memory_read
m.stubs[m.addr('mem_write')] = memory_write
m.stubs[m.addr('wdt_reload')] = lambda: m.ret()
m.stubs[m.addr('idindex_find')] = lambda: m.ret(0xffffffff)
m.u.mem_write(m.addr('id_cells_end'), struct.pack('<H', 1))
m.u.mem_write(0x21000000, number)
m.u.mem_write(0x21000040, b'\x5a' * 32)
assert m.call('user_findID', 0x21000000, 0x21000040) == 0
assert bytes(m.u.mem_read(0x21000040, 32)) == b'\x5a' * 32
read_ok = True
assert m.call('user_findID', 0x21000000, 0x21000040) == 1
assert bytes(m.u.mem_read(0x21000040, 32)) == record

read_data = b'\xff' * 32
temp_pointer = device + offsets['newAddressTempID']
for read_ok, write_ok, expected_writes in ((False, True, 0), (True, False, 1), (True, True, 1)):
    m.u.mem_write(temp_pointer, bytes(4))
    writes.clear()
    m.call('user_addIdToTempBuffer', 0x21000000, 0)
    assert len(writes) == expected_writes, (read_ok, write_ok, writes)
    assert bool(value('newAddressTempID')) == (read_ok and write_ok)

read_ok = False
read_data = bytes(32)
assert m.call('photo_confirmed_offset', 0, 100) == 0
assert m.call('user_getPhotoCount') == 0
assert value('newAddressPhoto') == 0xffffffff
print('IAR storage faults: ID denied, destination preserved, temp queue unchanged on failure, photo confirmation and unreadable storage rejected PASS')
