"""USB 116: резервная выгрузка базы ID страницами из настоящего ELF (Алексей 26.10.2026)."""
import struct

from storage_machine import StorageMachine, crc16

ID_BASE = 0x41000


def cell(index, ident, deleted=False):
    raw = struct.pack('<H10sBIHBBB', index, ident.encode(), 3, 0x6a000000, 60, 1, 5, 4)
    return raw + bytes(9) + (b'\xff' if deleted else b'\x00')


def machine(cells, records):
    m = StorageMachine()
    m.u.mem_write(m.addr('mem') + 2 * 8, struct.pack('<II', ID_BASE, cells * 32))
    for number, raw in records.items():
        m.flash[ID_BASE + (number - 1) * 32:ID_BASE + number * 32] = raw
    m.sent = []
    def any_data():
        cmd, data, size, channel = m.args()
        m.sent.append((cmd, bytes(m.u.mem_read(data, size)), channel))
        m.ret()
    m.stubs[m.addr('user_send_anyData')] = any_data
    if 'user_command_ready' in m.symbols:
        m.stubs[m.addr('user_command_ready')] = lambda: m.ret(1)
    return m


def page(m, first):
    body = struct.pack('<H', first)
    p = struct.pack('<HBBB', 3 + len(body), 0, 73, 116) + body
    m.u.mem_write(0x21000000, p + struct.pack('<H', crc16(p)))
    m.sent.clear()
    m.receive_command(0x21000000, 0)
    assert len(m.sent) == 1 and m.sent[0][0] == 116 and m.sent[0][2] == 0, m.sent
    reply = m.sent[0][1]
    code, following, count = reply[0], struct.unpack('<H', reply[1:3])[0], reply[3]
    rows = [reply[4 + 32 * i:36 + 32 * i] for i in range(count)]
    assert len(reply) == 4 + 32 * count, len(reply)
    return code, following, rows


def run():
    stored = {n: cell(n, '%010d' % (9000000000 + n)) for n in (3, 10, 11, 12, 13, 14, 15, 16, 200)}
    m = machine(256, {**stored, 4: cell(4, '9999999999', deleted=True)})
    code, following, rows = page(m, 1)
    assert (code, following) == (0, 16) and rows == [stored[n] for n in (3, 10, 11, 12, 13, 14, 15)], (code, following)
    code, following, rows = page(m, following)
    assert (code, following) == (0, 0) and rows == [stored[16], stored[200]], (code, following)

    # За вызов не больше 256 ячеек: пустая база отвечает продолжением, а не долгим проходом.
    m = machine(600, {500: cell(500, '9135550500')})
    assert page(m, 1) == (0, 257, [])
    assert page(m, 257) == (0, 513, [cell(500, '9135550500')])
    assert page(m, 513) == (0, 0, [])

    # Флеш занята: код 1, та же ячейка для повтора, записей нет.
    m = machine(256, stored)
    m.write(m.addr('mem_storage_ready'), 0, 1)
    assert page(m, 10) == (1, 10, [])
    # Отказ чтения посреди страницы: уже прочитанное не отдаётся, повтор с той же ячейки.
    m = machine(256, stored)
    m.fault = ('read', m.counts['read'] + 3)
    assert page(m, 10) == (1, 10, [])
    assert page(m, 10)[:2] == (0, 17)
    return {'pages': 'PASS', 'scan_limit': 256, 'busy': 'retry same cell'}


def mutations():
    from test_flash_mutations import run as mutate
    return mutate({'usb-ids-deleted': ('run', [
        ('User/c/user_commands.c', 'c->index != 0xffff && c->reserve[9] != 255', 'c->index != 0xffff'),
    ]), 'usb-ids-busy': ('run', [
        ('User/c/user_commands.c', 'if (reply[0]) { n = 0; cell = from; }', 'if (reply[0]) { n = 0; }'),
    ])}, 'test_usb_ids', 'usb-ids-mutations')
