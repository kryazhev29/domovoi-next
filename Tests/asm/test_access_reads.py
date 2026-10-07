"""Доработка: Кряжев А., 2026.
Доступ на ELF: отказы чтения таблиц не используют старые данные стека.
Проверяются события, выходы и счётчики на модели NOR без платы.
"""
import hashlib, json, struct
from storage_machine import *

GROUP, SHEDULE, HOLIDAY, IDS = 0x1c9000, 0x1ca800, 0x1cc000, 0x41000
TODAY = 1790000000


class AccessMachine(StorageMachine):
    def __init__(self):
        super().__init__()
        self.events, self.loads = [], []
        for name, fn in [('log_addControlEvent', self.event), ('out_getmodestate', lambda: self.ret(0)),
                         ('out_load', self.load), ('unreg_sms_note', self.ret)]:
            self.stubs[self.addr(name)] = fn
        self.write(self.device + self.d['datetime'], TODAY, 4)
        rele = fields('releMode_s')
        self.write(self.addr('config') + fields('config_s')['rele'] + rele['onWiegand'], 1, 1)

    def read(self):
        # Живой поиск читает не более страницы за один вызов.
        a, p, n, _ = self.args(); self.read_budget += n
        assert 0 < n <= 256 and a + n <= len(self.flash), (hex(a), n)
        if self.failed('read'): self.ret(0); return
        self.u.mem_write(p, bytes(self.flash[a:a + n])); self.ret(1)

    def event(self):
        n, p, cnt, _ = self.args()
        self.events.append((n, bytes(self.u.mem_read(p, 10)) if p else b'', cnt)); self.ret(1)

    def load(self):
        self.loads.append(tuple(self.args())); self.ret()

    def fail_read(self, k=1):
        self.fault = ('read', self.counts['read'] + k)

    def put_id(self, cell, ident, group=1):
        c = fields('cellID_s'); rec = bytearray(b'\0' * 32)
        rec[0:2] = struct.pack('<H', cell); rec[c['ID']:c['ID'] + 10] = ident; rec[c['group']] = group
        rec[c['cntPassOnDay']] = 255; rec[c['cntPass']] = 255
        self.flash[IDS + (cell - 1) * 32:IDS + cell * 32] = rec
        self.write(self.addr('id_cells_end'), max(cell, self.read16('id_cells_end')), 2)

    def read16(self, name): return int.from_bytes(self.u.mem_read(self.addr(name), 2), 'little')

    def put_group(self, n, wiegand=1):
        g = fields('cellGroup_s'); rec = bytearray(b'\0' * 16)
        rec[0] = n; rec[g['sheduleNumber']] = 0; rec[g['onWiegand']] = wiegand
        self.flash[GROUP + (n - 1) * 16:GROUP + n * 16] = rec


def table_cells():
    m = AccessMachine(); buf = 0x21000300
    m.put_group(5)
    m.flash[SHEDULE + 31 * 16:SHEDULE + 32 * 16] = bytes([32]) + bytes(range(1, 16))
    m.flash[SHEDULE + 1024:SHEDULE + 1040] = bytes(range(8)) + b"\1" + bytes(7)
    assert m.checked('user_getGroup', 5, buf) == 1 and bytes(m.u.mem_read(buf, 1)) == b'\5'
    assert m.counts['read'] == 1, 'one read at the cell, no 2 KiB scan'
    assert m.checked('user_getGroup', 6, buf) == 0, 'erased cell'
    assert m.checked('user_getGroup', 0, buf) == 0, 'number 0 is reserved'
    assert m.checked('user_getGroup', 129, buf) == 0, 'outside the 2048-byte area'
    m.flash[GROUP + 6 * 16] = 9
    assert m.checked('user_getGroup', 7, buf) == 0, 'cell holds another number'
    # После удачного чтения отказ той же группы не должен использовать старые данные стека.
    assert m.checked('user_getGroup', 5, buf) == 1
    m.fail_read(); assert m.checked('user_getGroup', 5, buf) == 0, 'read error is not a group'
    assert m.checked('user_getShedule', 32, buf) == 1 and bytes(m.u.mem_read(buf, 32))[0] == 32
    assert m.checked('user_getShedule', 65, buf) == 0
    assert m.checked('user_getShedule', 32, buf) == 1
    m.fail_read(); assert m.checked('user_getShedule', 32, buf) == 0
    return {'group_stack': m.stack_peak}


def holidays():
    m = AccessMachine()
    assert m.checked('user_dayIsHoliday') == 0 and m.counts['read'] == 16, '16 reads of 64 bytes'
    for k in (1, 5, 16):
        m.fail_read(k); assert m.checked('user_dayIsHoliday') == 2, ('read error', k)
    m.flash[HOLIDAY + 900:HOLIDAY + 904] = struct.pack('<I', TODAY - 100)
    assert m.checked('user_dayIsHoliday') == 1
    return {'stack': m.stack_peak}


def find_id():
    m = AccessMachine(); buf = 0x21000300
    for cell in range(1, 70): m.put_id(cell, b'%010d' % cell)
    assert m.checked('user_findID', m.addr('config') + 0, buf) in (0,)  # пустой ID конфигурации
    ident = 0x21000400; m.u.mem_write(ident, b'0000000060')
    assert m.checked('user_findID', ident, buf) == 1 and bytes(m.u.mem_read(buf + 2, 10)) == b'0000000060'
    m.u.mem_write(ident, b'9999999999'); assert m.checked('user_findID', ident, buf) == 0
    m.u.mem_write(ident, b'0000000060'); m.fail_read(2)
    assert m.checked('user_findID', ident, buf) == 2, 'read error in the scan is not "absent"'
    return {'stack': m.stack_peak}


def access():
    out = {}
    m = AccessMachine(); m.put_group(1)
    for cell in range(1, 70): m.put_id(cell, b'%010d' % cell)
    ident = 0x21000400
    m.u.mem_write(ident, b'0000000060'); m.checked('user_checkIdAndControl', ident, 2)
    assert m.loads and m.events and m.events[-1][0] == 71, ('access by Wiegand', m.events, m.loads)
    out['granted_stack'] = m.stack_peak
    m.events.clear(); m.loads.clear()
    m.u.mem_write(ident, b'9999999999'); m.checked('user_checkIdAndControl', ident, 2)
    assert m.events == [(65, b'9999999999', 0)] and not m.loads, 'absent -> event 65'
    for k in (1, 2, 3):
        m.events.clear(); m.loads.clear()
        m.u.mem_write(ident, b'0000000060'); m.fail_read(k); m.checked('user_checkIdAndControl', ident, 2)
        assert not m.loads and all(e[0] != 65 for e in m.events), ('read error -> no relay, no 65', k, m.events)
    m.events.clear(); m.loads.clear()
    m.fail_read(2)
    assert m.checked('user_activateOut', 1, 0, 2, ident) == 0 and not m.loads, 'activate: read error -> refused'
    return out


if __name__ == '__main__':
    res = {'units': 'Unicorn instructions; NOT cycles/time', 'table_cells': table_cells(), 'holidays': holidays(),
           'find_id': find_id(), 'access': access(),
           'elf_sha256': hashlib.sha256((__import__('iar_image').image_paths()[0]).read_bytes()).hexdigest()}
    (__import__('iar_image').result_path('access_reads_results.json')).write_text(json.dumps(res, indent=2) + '\n')
    print('PASS access reads: group/schedule by cell, holiday 0/1/2, find ID read error != absent, no 65 or relay on error')
