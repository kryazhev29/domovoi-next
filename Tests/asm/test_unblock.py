"""Разблокировка на IAR ELF: настоящий исполнитель, расписания, команды и outs.c."""
import struct

from storage_machine import fields, UC_ARM_REG_SP
from test_access_reads import AccessMachine, SHEDULE, HOLIDAY
from test_sched16 import record, tail, parse
from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_WRITE


class UnblockMachine(AccessMachine):
    def __init__(self, mask=2):
        super().__init__()
        self.system_events = []
        self.packets = []
        self.stubs[self.addr('log_addSystemEvent')] = self.system_event
        for name in ('usb_send', 'usb_try_send'):
            if name in self.symbols: self.stubs[self.addr(name)] = self.send_usb
        for name in ('out_load', 'out_getmodestate'):
            self.stubs.pop(self.addr(name))
        self.u.hook_add(UC_HOOK_CODE, self.output_call,
                        begin=self.addr('out_load'), end=self.addr('out_load'))
        self.u.hook_add(UC_HOOK_MEM_WRITE, self.gpio, begin=0x40010810, end=0x40010c14)
        self.cfg = fields('config_s'); self.rele = fields('releMode_s')
        self.trigger = fields('triggers_s')
        self.write(self.addr('mem')+6*8, 0x1cd400)
        self.write(self.addr('mem')+6*8+4, 4096)
        self.write(self.addr('cfg_boot_done'), 1, 1)
        self.write(self.addr('cfg_cached_active'), 0x1cd400)
        self.booting = False
        for i in range(2):
            for name, value in (('useInBlock', (mask >> i) & 1), ('onCmdServer', 1),
                                ('onWiegand', 1), ('tImpulse', 1), ('cnt', 1)):
                self.set_relay(i, name, value)
        self.clock(12, 0)

    def system_event(self):
        self.system_events.append(self.args()[0]); self.ret(1)

    def send_usb(self):
        p, n, _, _ = self.args()
        self.packets.append(bytes(self.u.mem_read(p, n))); self.ret(1)

    def status_outputs(self):
        self.checked('timers_set', 2, 100); self.checked('user_loop')
        self.checked('user_send_statusPack', 0)
        return int.from_bytes(self.packets[-1][13:15], 'little') & 3

    def read(self):
        a, p, n, _ = self.args()
        if n <= 256: return super().read()
        assert self.booting and n == 512 and a+n <= len(self.flash)
        self.read_budget += n
        self.u.mem_write(p, bytes(self.flash[a:a+n])); self.ret(1)

    def output_call(self, u, address, size, unused):
        period = int.from_bytes(u.mem_read(u.reg_read(UC_ARM_REG_SP), 4), 'little')
        self.loads.append(tuple(self.args()) + (period,))

    # Только электрическая граница модели: BSRR/BRR изменяют ODR реального outs.c.
    def gpio(self, u, access, address, size, value, unused):
        if address not in (0x40010810, 0x40010814, 0x40010c10, 0x40010c14): return
        odr = (address & ~0xff) + 12
        old = int.from_bytes(u.mem_read(odr, 4), 'little')
        new = old | value if address & 0xff == 0x10 else old & ~value
        u.mem_write(odr, struct.pack('<I', new))

    def set_relay(self, i, name, value):
        self.write(self.addr('config') + self.cfg['rele'] + i*11 + self.rele[name], value, 1)

    def set_trigger(self, name, value):
        self.write(self.device+self.d['triggers']+self.trigger[name], value, 1)

    def clock(self, hour, minute, day=0):
        self.write(self.device+self.d['datetime'], 1789948800+day*86400+hour*3600+minute*60)

    def effective(self): return self.u.mem_read(self.addr('unblock_active'), 1)[0]

    def outputs(self): return tuple(self.checked('out_getstate', i) for i in range(2))

    def triggers_command(self, hand=0, scene=0):
        data = bytearray(16)
        data[self.trigger['handUnBlock']] = hand
        data[self.trigger['sheduleUnBlock']] = scene
        parse(self, 5, data)
        self.finish_storage()
        assert self.answers[-1] == (5, 0, 0, 37), self.answers

    def finish_storage(self):
        for _ in range(20000):
            self.now += 1; self.checked('user_storage_poll')
            if not self.u.mem_read(self.addr('store')+fields('store_state_s')['job'], 1)[0] and not self.checked('service_crc_active'): return
        raise AssertionError('сохранение не завершилось')

    def schedule(self, rec=None, scenes=None, holiday=1):
        self.flash[SHEDULE:SHEDULE+16] = rec or record(1)
        self.flash[SHEDULE+1024:SHEDULE+1040] = tail(scenes if scenes is not None else bytes([1])+bytes(7), holiday)
        self.write(self.addr('log_timeKnown'), 1, 1)
        self.set_trigger('sheduleUnBlock', 1)


def manual():
    m = UnblockMachine(); m.checked('user_unblock_recheck', 1)
    assert m.outputs() == (0, 1)
    assert m.status_outputs() == 1
    m.loads.clear(); m.triggers_command(hand=1)
    assert m.effective() == 1 and m.outputs() == (1, 0)
    assert m.status_outputs() == 2
    opening = next(i for i, load in enumerate(m.loads) if load[:2] == (0, 3))
    breaking = next(i for i, load in enumerate(m.loads) if load[:2] == (1, 0))
    assert opening < breaking, 'линия разорвана до открытия'
    assert all(load[0] == 0 for load in m.loads[:breaking]), 'реле разблокировки тронуто до открытия'
    assert m.loads[opening] == (0, 3, 1, 0, 1), m.loads
    for _ in range(10): m.checked('out_process')
    assert m.outputs() == (0, 0), 'штатный импульс должен завершиться'
    assert m.status_outputs() == 0
    m.put_group(1); m.put_id(1, b'1234567890')
    m.u.mem_write(0x21000600, b'1234567890\0'); m.loads.clear()
    m.checked('user_checkIdAndControl', 0x21000600, 2)
    m.checked('user_incomingcall', 0x21000600, 1)
    assert m.checked('user_activateOut', 1, 1, 2, 0x21000600) == 0
    for outputs in (b'\x01\x00', b'\x00\x00', b'\x01\x01'):
        parse(m, 8, b'\x01'+outputs); assert m.answers[-1] == (8, 2, 0, 37)
    assert not m.loads and not m.events and m.outputs() == (0, 0)
    m.schedule(record(1, start=(13, 0), stop=(14, 0)))
    reads = m.counts['read']; m.checked('user_unblock_recheck', 0)
    assert m.effective() == 1 and m.counts['read'] == reads, 'ручная важнее сцены'
    m.clock(15, 0); m.checked('user_unblock_poll'); assert m.effective() == 1 and not m.loads
    m.triggers_command(); assert m.effective() == 0 and m.outputs() == (0, 1)
    assert m.system_events == [30, 31], m.system_events
    parse(m, 8, b'\x01\x00\x00'); assert m.outputs() == (0, 1)
    return {'manual_priority_and_guards': 'PASS', 'opening_before_break': 'PASS'}


def scenes():
    m = UnblockMachine(); m.schedule(); m.clock(8, 29)
    m.checked('user_unblock_recheck', 1); assert m.effective() == 0
    m.clock(8, 30); m.checked('user_unblock_poll'); assert m.effective() == 2
    m.loads.clear(); m.checked('user_unblock_poll'); assert not m.loads
    m.clock(18, 15); m.checked('user_unblock_poll'); assert m.effective() == 2
    m.clock(18, 16); m.checked('user_unblock_poll'); assert m.effective() == 0
    assert m.system_events == [28, 29]
    m.schedule(record(1, start=(22, 0), stop=(6, 0)))
    for hour, minute, active in ((21, 59, 0), (22, 0, 2), (23, 59, 2), (0, 0, 2), (6, 0, 2), (6, 1, 0)):
        m.clock(hour, minute); m.checked('user_unblock_poll'); assert m.effective() == active
    cases = 0
    for holiday in (1, 2, 3):
        for today_holiday in (False, True):
            m = UnblockMachine(); m.schedule(holiday=holiday)
            if today_holiday:
                stamp = int.from_bytes(m.u.mem_read(m.device+m.d['datetime'], 4), 'little')
                m.flash[HOLIDAY:HOLIDAY+4] = struct.pack('<I', stamp)
            m.checked('user_unblock_recheck', 1)
            expected = holiday == 1 or (holiday == 2 and today_holiday) or (holiday == 3 and not today_holiday)
            assert bool(m.effective()) == expected, (holiday, today_holiday)
            cases += 1
    for rec, numbers, enabled, known in (
        (record(1, blocked=1), bytes([1])+bytes(7), 1, 1),
        (record(1, days=bytes(7)), bytes([1])+bytes(7), 1, 1),
        (record(1), bytes(8), 1, 1), (record(1), bytes([2])*8, 1, 1),
        (record(1), bytes([65,255])+bytes(6), 1, 1),
        (record(1), bytes([1])*8, 0, 1), (record(1), bytes([1])*8, 1, 0)):
        m = UnblockMachine(); m.schedule(rec, numbers)
        m.set_trigger('sheduleUnBlock', enabled); m.write(m.addr('log_timeKnown'), known, 1)
        m.checked('user_unblock_recheck', 1); assert m.effective() == 0 and m.outputs() == (0, 1)
        cases += 1
    m = UnblockMachine(); m.schedule(scenes=bytes(7)+b'\x40')
    m.flash[SHEDULE+63*16:SHEDULE+64*16] = record(64)
    m.checked('user_unblock_recheck', 1); assert m.effective() == 2
    m = UnblockMachine(); m.schedule(); m.write(m.addr('log_timeKnown'), 0, 1)
    m.clock(7, 0); m.checked('user_unblock_recheck', 1)
    parse(m, 23, struct.pack('<I', 1789948800+12*3600)); assert m.effective() == 2
    m = UnblockMachine(); m.schedule(); m.stubs[m.addr('w25_read_available')] = lambda: m.ret(0)
    m.checked('user_unblock_recheck', 1); assert not m.effective()
    m.stubs[m.addr('w25_read_available')] = lambda: m.ret(1)
    m.checked('user_unblock_poll'); assert m.effective() == 2
    return {'holiday_and_denial_cases': cases, 'minute_and_midnight': 'PASS', 'time_and_busy': 'PASS'}


def offline():
    for mode in ('handUnBlock', 'sheduleUnBlock'):
        m = UnblockMachine()
        if mode == 'sheduleUnBlock': m.schedule()
        else: m.set_trigger(mode, 1)
        m.checked('user_unblock_recheck', 1); assert m.effective()
        for seconds in (1799, 1800, 1801):
            m.write(m.device+m.d['worktime'], seconds); m.checked('user_unblock_poll')
            assert bool(m.effective()) == (seconds < 1800), ('оффлайн 30 минут', mode, seconds)
        assert m.outputs()[1] == 1
        assert m.u.mem_read(m.device+m.d['triggers']+m.trigger[mode], 1) == b'\x01'
        parse(m, 23, struct.pack('<I', 1789948800+12*3600))
        m.checked('user_unblock_recheck', 1); assert not m.effective()
        m.stubs[m.addr('comm_server_registered')] = m.ret
        parse(m, 1, b'\x02', channel=1); assert not m.effective()
        m.loads.clear()
        parse(m, 1, b'\x00'+struct.pack('<I', 1789948800+12*3600), channel=1)
        assert m.effective() and m.outputs() == (1, 0)
        assert [(x[0], x[1]) for x in m.loads][-2:] == [(0, 3), (1, 0)]
        m.write(m.device+m.d['worktime'], 10000); m.checked('user_unblock_poll')
        m.write(m.device+m.d['registredOnServer'], 0, 1)
        m.write(m.device+m.d['worktime'], 11799); m.checked('user_unblock_poll'); assert m.effective()
        m.write(m.device+m.d['worktime'], 11800); m.checked('user_unblock_poll'); assert not m.effective()
    return {'manual_and_scene_timeout_registration': 'PASS'}


def restart_and_commands():
    m = UnblockMachine()
    m.u.mem_write(m.device+m.d['reserv']+32, struct.pack('<II', 0x36314853, 0xc9ceb7ac))
    m.triggers_command(hand=1)
    assert m.outputs() == (1, 0) and m.loads[-2][:2] == (0, 3) and m.loads[-1][:2] == (1, 0)
    # Новый ELF-контекст восстанавливает сохранённые триггеры настоящим user_init.
    r = UnblockMachine(); r.flash[:] = m.flash; r.booting = True
    r.write(r.addr('cfg_boot_done'), 0, 1)
    assert r.checked('cfg_boot_select', budget=12000000)
    for name in ('mem_recover', 'user_getNumbersCount', 'log_getRecordsCount',
                 'user_getTempIDRecordsCount', 'user_comm_boot', 'app_boot_metadata',
                 'WG_init', 'rs485_init', 'uart_setRecTimeout', 'fw_resume_after_boot'):
        r.stubs[r.addr(name)] = r.ret
    r.stubs[r.addr('w25_read_devID')] = lambda: r.ret(0x17c2)
    r.checked('user_init', budget=12000000)
    assert r.effective() == 1 and r.outputs() == (1, 0)
    assert r.loads[-2][:2] == (0, 3) and r.loads[-1][:2] == (1, 0)
    assert all(load[0] == 0 for load in r.loads[:-1]), 'старт замыкает линию перед открытием'
    # Настройки проходят настоящий разбор, A/B и применение снимка; модем не запускается.
    m.stubs[m.addr('comm_modem_valid_settings')] = lambda: m.ret(1)
    for name in ('comm_endpoints', 'user_comm_boot', 'rs485_init'): m.stubs[m.addr(name)] = m.ret
    m.write(m.addr('cfg_cached_active'), 0x1cd400); m.write(m.addr('cfg_boot_done'), 1, 1)
    c = m.addr('config')
    m.u.mem_write(c+m.cfg['server1'], b'gate.gsmgate.ru:15010\0'); m.u.mem_write(c+m.cfg['countryCode'], b'+7\0')
    m.flash[0x1cd400:0x1ce400] = bytes(4096)
    for mask in (1, 0, 3, 2):
        data = bytearray(m.u.mem_read(c, fields('config_s')['crc']+2))
        for i in range(2): data[m.cfg['rele']+11*i+m.rele['useInBlock']] = (mask >> i) & 1
        m.loads.clear(); parse(m, 12, data); m.finish_storage()
        assert m.effective() == 1
        assert m.outputs() == ((0, 1) if mask == 1 else (0, 0) if mask in (0,3) else (1, 0)), (mask, m.outputs())
        if mask == 0: assert all(x[1] == 0 for x in m.loads), 'без реле разблокировки нет открытия'
        if mask == 3: assert all(x[1] == 0 for x in m.loads), 'оба реле разблокировки только выключаются'
        for _ in range(10): m.checked('out_process')
    m.loads.clear(); m.command(14, record(1)+tail(bytes([1])+bytes(7)), 1); m.drain()
    assert any(x[:2] == (0, 3) for x in m.loads) and m.loads[-1][:2] == (1, 0)
    m.set_trigger('handUnBlock', 0); m.set_trigger('sheduleUnBlock', 1)
    m.write(m.addr('log_timeKnown'), 1, 1); m.checked('user_unblock_recheck', 0); assert m.effective() == 2
    m.command(14, record(1, blocked=1)+tail(bytes([1])+bytes(7)), 1); m.drain()
    assert not m.effective() and m.outputs()[1] == 1
    m = UnblockMachine(); m.set_relay(0, 'mode', 1); m.set_trigger('handUnBlock', 1)
    for _ in range(2): m.checked('user_unblock_recheck', 1); assert m.outputs() == (1, 0)
    assert not m.checked('out_getmodestate', 0)
    return {'startup_config12_schedule14_level': 'PASS'}
