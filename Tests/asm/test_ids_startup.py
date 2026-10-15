"""Фоновый старт ID: полный поиск, задержка записей/статуса и обрывы NOR."""
import struct
from storage_machine import StorageMachine
from test_ids_boot_read import cell
from test_idindex_runtime import IndexMachine, bucket, fnv, MAGIC, IX_BASE, IX_LOG

BASE = 0x41000

def ram(m, name, n=2):
    return int.from_bytes(m.u.mem_read(m.addr(name), n), 'little')

def find(m, ident):
    m.u.mem_write(0x21000400, ident)
    return m.checked('user_findID', 0x21000400, 0x21000500, budget=30000000)

def oracle(m):
    size = int.from_bytes(m.u.mem_read(m.addr('mem') + 20, 4), 'little')
    count = end = used = total = 0
    for off in range(0, size, 32):
        row = m.flash[BASE+off:BASE+off+32]; i = off // 32 + 1
        if row != b'\xff'*32: used = i * 32
        if row[:2] == b'\xff\xff' or row[31] == 255: continue
        count += 1; end = i
        total ^= fnv(struct.pack('<H', i) + row[2:12])
    return count, end, used, total

def verify_boot(m):
    count, end, used, total = oracle(m)
    assert m.boot_ids() == count
    assert ram(m, 'id_cells_end') == end and ram(m, 'id_used_bytes', 4) == used
    assert ram(m, 'ix', 4) == total
    if m.state() == 3:
        for off in range(end):
            row = m.flash[BASE+32*off:BASE+32*(off+1)]
            if row[:2] == b'\xff\xff' or row[31] == 255: continue
            slots = struct.unpack('<32H', m.flash[bucket(row[2:12]):bucket(row[2:12])+64])
            assert off + 1 in slots, ('READY пропустил ID', off + 1)
    return count, end, used

def seed_index(m):
    # Перед изменением базы существует верная долговечная отметка и настоящие корзины.
    count, end, used, total = oracle(m)
    for off in range(end):
        row = m.flash[BASE+32*off:BASE+32*(off+1)]
        if row[:2] == b'\xff\xff' or row[31] == 255: continue
        address = bucket(row[2:12])
        for slot in range(32):
            if m.flash[address+2*slot:address+2*slot+2] == b'\xff\xff':
                m.flash[address+2*slot:address+2*slot+2] = struct.pack('<H', off+1); break
        else: raise AssertionError('полная корзина фикстуры')
    m.flash[IX_LOG:IX_LOG+8] = struct.pack('<II', total, (~total ^ MAGIC)&0xffffffff)
    return total, end

def main():
    m = StorageMachine(); m.flash[BASE:BASE+32] = cell(1, b'1111111111')
    m.flash[BASE+5*32:BASE+6*32] = cell(6, b'2222222222')
    m.flash[BASE+255*32+12] = 0
    assert m.boot_ids() == 2 and ram(m, 'id_cells_end') == 6
    assert ram(m, 'id_used_bytes', 4) == 8192
    m = StorageMachine(); m.flash[BASE:BASE+32] = cell(1, b'1111111111')
    m.flash[BASE+8192-32:BASE+8192] = cell(256, b'2222222222')
    m.checked('user_ids_boot'); m.fault = ('read', 2)
    assert m.boot_scan_tick() == 2 and m.boot_scan_tick() == 3
    assert ram(m, 'id_cells_end') == 256 and ram(m, 'id_used_bytes', 4) == 0xffffffff
    assert find(m, b'2222222222') == 1
    while ram(m, 'id_scan_cell'): m.boot_scan_tick()
    assert int.from_bytes(m.u.mem_read(m.device+m.d['numbersCount'], 2), 'little') == 2
    m = StorageMachine(); size = 1601536; last = size // 32
    m.u.mem_write(m.addr('mem')+16, struct.pack('<II', BASE, size))
    m.flash[BASE+size-32:BASE+size] = cell(last, b'9999999999')
    m.checked('user_ids_boot'); m.limit_reads = size + 256
    assert find(m, b'9999999999') == 1
    m.boot_scan_tick(); assert find(m, b'9999999999') == 1
    for cmd in (3, 13, 17, 18, 101, 103):
        m.u.mem_write(0x21000600, bytes((3, 0, 0, 0, cmd)))
        assert not m.checked('user_command_ready', 0x21000600, 1), cmd
    for name, args in [('store_id_begin', (BASE, 0x21000500)),
                       ('idclear_runtime_begin', (0, 7)), ('expiry_runtime_begin', ()),
                       ('store_factory_begin', ()), ('service_crc_begin_pack', (0, 7)),
                       ('idcrc_background_begin', ())]:
        assert not m.checked(name, *args), name
    assert not m.checked('idcrc_io_start', BASE, 0x21000500, 32, 0)
    sent = []
    def packet():
        p, n, _, _ = m.args(); sent.append(bytes(m.u.mem_read(p, n+6))); m.ret(1)
    m.stubs[m.addr('packet_send')] = packet
    m.checked('user_send_statusPackToConfigurator', 0); assert not sent
    while ram(m, 'id_scan_cell'): m.boot_scan_tick()
    m.checked('user_send_statusPackToConfigurator', 0)
    assert sent[0][37:39] == b'\x01\0' and ram(m, 'id_cells_end') == last
    m = StorageMachine(); m.write(m.device+m.d['memID'], 0x14c2, 2)
    assert m.boot_ids() == 0 and ram(m, 'id_used_bytes', 4) == 0
    migration(); mutations(); cached_access()
    print('PASS IDs startup: 256B, last real ID, retry, writers/CRC/status wait, 2MB, old marks, 2 mutations')

def migration():
    m = IndexMachine(); m.flash[BASE:BASE+32] = cell(1, b'1234567890'); total = oracle(m)[3]
    m.flash[IX_LOG:IX_LOG+8] = struct.pack('<II', total, (~total ^ 0x58444931) & 0xffffffff)
    assert verify_boot(m) == (1, 1, 32) and m.state() == 1
    m.drain_index(); image = bytes(m.flash)
    boot = IndexMachine(image); assert verify_boot(boot) == (1, 1, 32)
    assert boot.state() == 3 and bytes(boot.flash) == image

def cached_access():
    from test_access_reads import AccessMachine
    m = AccessMachine(); m.put_group(1)
    size = 1601536; last = size//32
    m.u.mem_write(m.addr('mem')+16, struct.pack('<II', BASE, size))
    m.put_id(last, b'0000050048'); m.checked('user_ids_boot')
    m.u.mem_write(0x21000400, b'0000050048')
    assert m.checked('user_access_ready', 0x21000400, budget=30000000)
    assert m.read_budget == size
    m.checked('user_checkIdAndControl', 0x21000400, 2)
    assert m.read_budget <= 128 and m.loads and m.events[-1][0] == 71
    # Квант хранения не мешает повтору этой карты; другой ID проверяет всю базу.
    assert m.checked('user_access_ready', 0x21000400, budget=30000000)
    assert m.read_budget == 32
    m.boot_scan_tick()
    assert m.checked('user_access_ready', 0x21000400) and m.read_budget == 32
    # Карта со счётчиком остаётся ожидающей, повтор не мешает проходу базы.
    from iar_layout import fields
    m.flash[BASE+size-32+fields('cellID_s')['cntPassOnDay']] = 3
    for _ in range(3):
        assert not m.checked('user_access_ready', 0x21000400) and m.read_budget == 32
        before = ram(m, 'id_scan_cell'); m.boot_scan_tick()
        assert ram(m, 'id_scan_cell') == before + 8
    m.u.mem_write(0x21000400, b'0000050047')
    m.checked('user_checkIdAndControl', 0x21000400, 2, budget=30000000)
    assert m.events[-1][0] == 65

def mutations():
    m = StorageMachine(); m.flash[BASE+8192-32:BASE+8192] = cell(256, b'1234567890')
    m.checked('user_ids_boot'); m.boot_scan_tick(); m.write(m.addr('id_cells_end'), 8, 2)
    assert find(m, b'1234567890') == 0, 'мутация границы не обнаружена'
    m = IndexMachine(); expected, total, end = m.setup((1,))
    m.write(m.at+10, 5, 1)  # IX_ERASE вместо погашения отметки IX_INVALIDATE
    while m.state() != 2 or m.owner(): m.tick_index()
    boot = IndexMachine(m.flash)
    try: verify_boot(boot)
    except AssertionError as e: assert e.args[0][0] == 'READY пропустил ID'
    else: raise AssertionError('мутация погашения отметки выжила')

def power():
    from test_expiry_runtime import ExpiryMachine, record
    from test_idclear_runtime import ClearMachine
    totals = {}
    for case in ('command13', 'sms_add', 'delete', 'expiry', 'clear'):
        if case in ('command13', 'sms_add', 'delete'):
            m = StorageMachine(); m.record_changes = True
            if case != 'sms_add': m.flash[BASE:BASE+32] = cell(1, b'1111111111')
            seed_index(m)
            m.boot_ids()
            m.checked('idindex_runtime_suspend'); initial = bytes(m.flash)
            if case == 'sms_add':
                from iar_layout import fields
                m.write(m.addr('config')+fields('config_s')['pinCode'], 4321)
                m.stubs[m.addr('user_sendsms')] = lambda: m.ret(1)
                m.u.mem_write(0x21000700, b'\0')
                m.u.mem_write(0x21000800, b'9990001234\0')
                m.u.mem_write(0x21000900, b'#add 4321,9990001234\0')
                assert m.checked('user_smsreceived', 0x21000800, 0x21000900, 0x21000700)
                new = b'\x01\0' + bytes(m.flash[0x1e8002:0x1e8020])
                assert new[2:12] == b'9990001234'
            else:
                new = cell(1, b'2222222222') if case == 'command13' else cell(1, b'1111111111')[:-1]+b'\xff'
            new = new[:12] + b'\x01' + new[13:]  # действующий тип записи команды 13
            m.command(13, new, 1); m.drain()
            assert m.timestamps and not m.answers, (case, m.answers)
        elif case == 'expiry':
            m = ExpiryMachine(); m.install([record()]); m.record_changes = True
            m.checked('idindex_scanned', *seed_index(m))
            initial = bytes(m.flash); assert m.start_expiry(); m.expiry_drain()
        else:
            m = ClearMachine(); m.record_changes = True
            initial = bytes(m.flash); assert m.start_clear(); m.clear_drain()
        state = bytearray(initial); count = 0
        boot = IndexMachine(); saved_ram = bytes(boot.u.mem_read(0x20000000, 0x5000))
        boot.stubs[boot.addr('w25_write_data')] = boot.sync_write
        boot.stubs[boot.addr('w25_erase_sector')] = boot.sync_erase
        final_sector = bytes(m.flash[BASE:BASE+4096])
        for kind, address, data in m.changes:
            for n in sorted({0, 1, len(data)//2, len(data)-1, len(data)}):
                cut = bytearray(state)
                if kind == 'erase': cut[address:address+n] = data[:n]
                else:
                    for i, v in enumerate(data[:n]): cut[address+i] &= v
                boot.u.mem_write(0x20000000, saved_ram); boot.flash = cut; boot.io = None
                boot.checked('sj_recover', budget=2000000)
                if case in ('command13', 'delete', 'expiry'):
                    assert bytes(boot.flash[BASE:BASE+4096]) in (initial[BASE:BASE+4096], final_sector)
                if case == 'sms_add':
                    # Не завершённая допись не видна; зафиксированная совпадает целиком.
                    row = boot.flash[BASE:BASE+32]
                    assert row[31] == 255 or bytes(row) == new
                verify_boot(boot); count += 1
            if kind == 'erase': state[address:address+len(data)] = data
            else:
                for i, v in enumerate(data): state[address+i] &= v
        totals[case] = count
        assert count, case
    return totals

if __name__ == '__main__': main()
