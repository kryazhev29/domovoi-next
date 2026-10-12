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
        self.stubs.pop(self.addr('timers_getMillis'))
        self.gpio_changes = []
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
        self.gpio_changes.append((self.now, address, old, new))

    @property
    def now(self): return self._now

    @now.setter
    def now(self, value):
        self._now = value & 0xffffffff
        self.write(self.addr('milliSeconds'), self._now)

    # Настоящие миллисекунды/таймер и исполнитель импульсов, шаг не более 100 мс.
    def advance(self, ms):
        while ms:
            step = min(ms, 100); ms -= step; self.now += step
            self.checked('timers_set', 0, self.checked('timers_get', 0) + step)
            for _ in range(self.checked('timers_take', 0, 100)):
                self.checked('out_process')
            self.checked('user_unblock_poll')

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
            if not self.u.mem_read(self.addr('store')+fields('store_state_s')['intent'], 1)[0] and not self.exec_active(12) and not self.exec_active(6): return
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
    assert m.effective() == 1 and m.outputs() == (1, 1)
    assert m.status_outputs() == 3
    opening = next(i for i, load in enumerate(m.loads) if load[:2] == (0, 3))
    assert m.loads[opening] == (0, 3, 1, 0, 1), m.loads
    m.advance(1000)
    breaking = next(i for i, load in enumerate(m.loads) if load[:2] == (1, 0))
    assert opening < breaking, 'линия разорвана до открытия'
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
    for holiday in (0, 1, 2, 3, 4, 255):
        for today_holiday in (False, True):
            m = UnblockMachine(); m.schedule(holiday=holiday)
            if today_holiday:
                stamp = int.from_bytes(m.u.mem_read(m.device+m.d['datetime'], 4), 'little')
                m.flash[HOLIDAY:HOLIDAY+4] = struct.pack('<I', stamp)
            m.checked('user_unblock_recheck', 1)
            expected = holiday not in (2, 3) or (holiday == 2 and today_holiday) or (holiday == 3 and not today_holiday)
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
        assert m.effective() and m.outputs() == (1, 1)
        m.advance(1000)
        assert m.outputs() == (0, 0) and m.loads[-1][:2] == (1, 0)
        m.write(m.device+m.d['worktime'], 10000); m.checked('user_unblock_poll')
        m.write(m.device+m.d['registredOnServer'], 0, 1)
        m.write(m.device+m.d['worktime'], 11799); m.checked('user_unblock_poll'); assert m.effective()
        m.write(m.device+m.d['worktime'], 11800); m.checked('user_unblock_poll'); assert not m.effective()
    return {'manual_and_scene_timeout_registration': 'PASS'}


def restart_and_commands():
    m = UnblockMachine()
    m.u.mem_write(m.device+m.d['reserv']+32, struct.pack('<II', 0x36314853, 0xc9ceb7ac))
    m.triggers_command(hand=1)
    assert m.outputs() == (1, 1) and any(x[:2] == (0, 3) for x in m.loads)
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
    assert r.effective() == 1 and r.outputs() == (0, 0)
    assert not any(x[0] == 0 for x in r.loads), 'старт повторил управление'
    r.advance(1000); assert r.outputs() == (0, 0)
    m.advance(1000)
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
        m.advance(2000)
        assert m.outputs() == (0, 0), (mask, m.outputs())
        assert all(x[1] != 3 for x in m.loads), 'команда 12 повторила импульс'
    m.loads.clear(); m.command(14, record(1)+tail(bytes([1])+bytes(7)), 1); m.drain()
    assert not any(x[0] == 0 for x in m.loads), 'команда 14 повторила управление'
    assert m.outputs() == (0, 0)
    m.loads.clear(); m.command(16, bytes(4), 1); m.drain()
    assert not any(x[0] == 0 for x in m.loads), 'команда 16 повторила управление'
    m.set_trigger('handUnBlock', 0); m.set_trigger('sheduleUnBlock', 1)
    m.write(m.addr('log_timeKnown'), 1, 1); m.checked('user_unblock_recheck', 0); assert m.effective() == 2
    m.command(14, record(1, blocked=1)+tail(bytes([1])+bytes(7)), 1); m.drain()
    assert not m.effective() and m.outputs()[1] == 1
    m = UnblockMachine(); m.set_relay(0, 'mode', 1); m.set_trigger('handUnBlock', 1)
    for _ in range(2): m.checked('user_unblock_recheck', 1); assert m.outputs() == (0, 0)
    m.advance(1000); assert m.outputs() == (0, 0)
    assert not m.checked('out_getmodestate', 0)
    return {'startup_config12_schedule14_level': 'PASS'}


def pulse_delay():
    m = UnblockMachine(); m.checked('user_unblock_recheck', 1)
    m.set_relay(0, 'tPause', 1); m.set_relay(0, 'cnt', 2)
    m.set_trigger('handUnBlock', 1); m.checked('user_unblock_recheck', 0)
    assert m.effective() and m.outputs() == (1, 1), 'ИК разорвана во время импульса'
    m.loads.clear(); m.u.mem_write(0x21000600, b'1234567890\0')
    m.put_group(1); m.put_id(1, b'1234567890')
    m.checked('user_checkIdAndControl', 0x21000600, 2)
    m.checked('user_incomingcall', 0x21000600, 1)
    assert not m.checked('user_activateOut', 1, 1, 2, 0x21000600)
    parse(m, 8, b'\x01\x01\x01'); assert m.answers[-1] == (8, 2, 0, 37)
    assert not m.loads and not m.events, 'команда дёрнула реле во время ожидания'
    for _ in range(39):
        m.advance(100)
        assert m.checked('out_getmodestate', 0) and m.outputs()[1] == 1, 'ИК разорвана до конца серии'
    m.advance(100)
    assert not m.checked('out_getmodestate', 0) and m.outputs()[1] == 0, 'ИК не разорвана после серии'
    assert m.system_events == [30], 'начало должно попасть в журнал один раз'
    return {'pulse_series_and_wait_guards': 'PASS'}


def level_restore():
    for relay in (0, 1):
        for initial in (0, 1):
            m = UnblockMachine(mask=1 << (1-relay)); m.checked('user_unblock_recheck', 1)
            m.set_relay(relay, 'mode', 1); m.checked('out_load', relay, initial, 0, 0, 0)
            m.now = 0xfffffe00  # Выдержка выдерживает переполнение millis.
            m.set_trigger('handUnBlock', 1); m.checked('user_unblock_recheck', 0)
            assert m.outputs() == (1, 1), 'ИК разорвана при включении уровня'
            m.advance(999); assert m.outputs() == (1, 1), 'ИК разорвана раньше 1 с'
            m.checked('user_unblock_recheck', 0)  # Не сбрасывает время и исходный уровень.
            m.advance(1); assert m.outputs()[1-relay] == 0, 'ИК не разорвана через 1 с'
            m.loads.clear(); m.checked('user_unblock_recheck', 1)
            assert m.loads == [(1-relay, 0, 0, 0, 0)], 'восстановление повторило уровень'
            m.advance(1000); assert m.outputs()[1-relay] == 0
            m.loads.clear(); m.set_trigger('handUnBlock', 0); m.checked('user_unblock_recheck', 0)
            assert m.outputs()[1-relay] == 1 and m.outputs()[relay] == initial, 'уровень не возвращён'
            assert m.loads[:2] == [(1-relay, 1, 0, 0, 0), (relay, initial, 0, 0, 0)], 'возврат уровня раньше замыкания ИК'
            assert m.system_events == [30, 31]
    return {'level_1s_restore_off_on_both_relays_wrap': 'PASS'}


def cancel_wait():
    for level in (0, 1):
        for initial in ((0, 1) if level else (0,)):
            m = UnblockMachine(); m.checked('user_unblock_recheck', 1)
            m.set_relay(0, 'mode', level); m.checked('out_load', 0, initial, 0, 0, 0)
            m.set_trigger('handUnBlock', 1); m.checked('user_unblock_recheck', 0)
            m.gpio_changes.clear(); m.advance(500)
            m.set_trigger('handUnBlock', 0); m.checked('user_unblock_recheck', 0)
            assert not m.effective() and m.outputs()[1] == 1
            assert m.outputs()[0] == (initial if level else 1), 'отмена нарушила реле управления'
            m.advance(1500)
            assert m.outputs() == (initial if level else 0, 1), 'ожидание не отменено'
            assert not any(a == 0x40010814 and old & 1 and not new & 1
                           for _, a, old, new in m.gpio_changes), 'ИК размыкалась при отмене'
            assert m.system_events == [30, 31]
    return {'cancel_pulse_and_level': 'PASS'}


def waiting_rechecks():
    m = UnblockMachine(); m.checked('user_unblock_recheck', 1)
    m.set_trigger('handUnBlock', 1); m.checked('user_unblock_recheck', 0); m.advance(400)
    m.loads.clear()
    for restart in (0, 1, 0, 1): m.checked('user_unblock_recheck', restart)
    m.triggers_command(hand=1)
    assert not any(x[:2] == (0, 3) for x in m.loads), 'recheck повторил импульс'
    assert m.outputs()[1] == 0, 'восстановление не разорвало ИК сразу'
    m.advance(600); assert m.outputs() == (0, 0), 'recheck перезапустил ожидание'
    assert m.system_events == [30]
    for relay in (0, 1):
        r = UnblockMachine(mask=1 << (1-relay)); r.checked('user_unblock_recheck', 1)
        r.set_trigger('handUnBlock', 1); r.checked('user_unblock_recheck', 0); r.advance(1000)
        r.loads.clear(); r.checked('user_unblock_recheck', 1)
        assert r.loads == [(1-relay, 0, 0, 0, 0)], 'восстановление повторило импульс'
        r.advance(900); assert r.outputs() == (0, 0)
        r.advance(100); assert r.outputs() == (0, 0) and r.system_events == [30]
    # Реле с идущим импульсом становится ИК: нельзя погасить его назначением.
    for mask in (1, 3, 0):
        m = UnblockMachine(); m.checked('user_unblock_recheck', 1)
        m.set_trigger('handUnBlock', 1); m.checked('user_unblock_recheck', 0); m.advance(400)
        m.loads.clear()
        for i in range(2): m.set_relay(i, 'useInBlock', (mask >> i) & 1)
        m.checked('user_unblock_recheck', 0)
        assert m.outputs()[0] == 1 and m.checked('out_getmodestate', 0), 'смена назначения оборвала импульс'
        assert not any(x[1] == 3 for x in m.loads), 'смена назначения повторила импульс'
        m.advance(500); assert m.outputs()[0] == 1
        m.advance(100); assert not m.checked('out_getmodestate', 0)
        assert m.outputs() == (0, 0), (mask, m.outputs())
    # Повторная смена не останавливает прежний импульс, даже когда он был ИК.
    m = UnblockMachine(); m.checked('user_unblock_recheck', 1)
    m.set_trigger('handUnBlock', 1); m.checked('user_unblock_recheck', 0)
    for mask in (1, 2):
        for i in range(2): m.set_relay(i, 'useInBlock', (mask >> i) & 1)
        m.checked('user_unblock_recheck', 0); m.advance(400)
        assert m.checked('out_getmodestate', 0) and m.outputs()[0 if mask == 1 else 1] == 1
    m.advance(200); assert m.outputs() == (0, 0)
    for mask in (0, 3):
        m = UnblockMachine(mask); m.checked('user_unblock_recheck', 1)
        m.loads.clear(); m.set_trigger('handUnBlock', 1); m.checked('user_unblock_recheck', 0)
        assert m.effective() and m.outputs() == (0, 0)
        assert not any(x[1] == 3 for x in m.loads), 'оба ИК или ни одного: открытия нет'
    # Восстановление остаётся без управления и после отложенного чтения сцен.
    m=UnblockMachine(); m.schedule(); m.fail_read()
    m.checked('user_unblock_recheck',1); assert not m.effective()
    m.checked('user_unblock_poll'); assert m.effective() == 2 and m.outputs() == (0,0)
    assert not any(x[0] == 0 for x in m.loads), 'отложенное восстановление повторило управление'
    return {'restart_store_recheck_and_relay_changes': 'PASS'}


def sms(m, text, accepted=1):
    """Настоящий разбор SMS; перехватывается только отправка текста модему."""
    replies = []
    def send():
        p = m.args()[0]
        replies.append(bytes(m.u.mem_read(p, 128)).split(b'\0', 1)[0])
        m.ret()
    m.stubs[m.addr('user_sendsms')] = send
    m.stubs[m.addr('log_addSystemEventID')] = lambda: m.ret(1)
    m.write(m.addr('config') + m.cfg['pinCode'], 1234)
    m.u.mem_write(0x21000400, b'+79031234567\0')
    m.u.mem_write(0x21000500, text + b'\0')
    assert m.checked('user_smsreceived', 0x21000400, 0x21000500) == accepted
    return replies


def sms_outputs():
    for out in range(2):
        command = b'#setout 1234,' + str(out + 1).encode()
        enabled = b'SETOUT: Out ' + str(out + 1).encode() + b' enabled'
        blocked = [b'SETOUT: Error, Out is blocked']
        m = UnblockMachine(mask=0)
        m.set_relay(out, 'tPause', 2)
        m.set_relay(out, 'cnt', 2)
        m.checked('out_load', out ^ 1, 1, 0, 0, 0)
        assert sms(m, command) == [enabled]
        assert m.loads[-1] == (out, 3, 1, 2, 2) and m.outputs() == (1, 1)
        assert not m.events, 'SMS не добавляет события управления'
        m.advance(1100)
        assert m.outputs()[out] == 0, 'SMS-импульс должен выключиться сам'
        state = m.outputs(), bytes(m.u.mem_read(m.addr('out_time'), 4)), list(m.loads)
        assert sms(m, command) == blocked
        assert state == (m.outputs(), bytes(m.u.mem_read(m.addr('out_time'), 4)), m.loads)
        m.advance(2000)
        assert m.outputs()[out] == 1
        m.advance(1000)
        assert m.outputs()[out] == 0
        m.advance(2000)
        assert m.checked('out_getmodestate', out) == 0
        assert sms(m, command) == [enabled]
        for reason in ('onCmdServer', 'useInBlock', 'unblock'):
            m = UnblockMachine(mask=0)
            if reason == 'unblock':
                m.triggers_command(hand=1)
                assert m.effective() == 1
            else:
                m.set_relay(out, reason, int(reason == 'useInBlock'))
            before = m.outputs(), list(m.loads)
            assert sms(m, command) == blocked, reason
            assert before == (m.outputs(), m.loads) and not m.events, reason
        m = UnblockMachine(mask=0)
        m.set_relay(out, 'mode', 1)
        assert sms(m, command) == [enabled]
        assert sms(m, command) == [enabled]
        m.advance(5000)
        assert m.outputs()[out] == 1, 'уровень включается, а не инвертируется'
    return {'both_relays_pulse_pause_repeats_level_and_guards': 'PASS'}


def sms_format():
    m = UnblockMachine(mask=0)
    for arg in (b'', b'257', b'1abc', b'12', b'01', b'+1', b'-255', b'1,2', b'1 2'):
        assert sms(m, b'#setout 1234,' + arg) == [b'SETOUT: Error "Format not correct"'], arg
        assert not m.loads
    assert sms(m, b'#setout 1234,   2') == [b'SETOUT: Out 2 enabled']
    return {'exact_output_number': 'PASS'}


def capture_id():
    # Известный ID после захвата снова открывает; неизвестный даёт 65 только после захвата.
    for known in (False, True):
        m = UnblockMachine(mask=0)
        m.put_group(1)
        if known:
            m.put_id(1, b'0000123456')
        parse(m, 6, b'\x02\x01\x00')
        assert m.answers[-1][:2] == (6, 0)
        m.checked('WG_readedID', 0x123456)
        assert m.events == [(78, b'0000123456', 0)] and not m.loads
        assert bytes(m.u.mem_read(m.device + m.d['timeToGetID'], 2)) == b'\0\0'
        packets = []
        m.stubs[m.addr('comm_tx_acquire')] = lambda: m.ret(0x21000800)
        def commit():
            packets.append(bytes(m.u.mem_read(0x21000800, m.args()[0])))
            m.ret(1)
        m.stubs[m.addr('comm_tx_commit')] = commit
        m.checked('reader_reply_poll')
        assert len(packets) == 1 and packets[0][8:20] == b'\x08\x01' + b'0000123456'
        m.checked('reader_reply_poll')
        assert len(packets) == 1
        m.events.clear()
        m.checked('WG_readedID', 0x123456)
        assert m.events[0][0] == (71 if known else 65), m.events
        assert bool(m.loads) == known
    # Чужой интерфейс захвата тоже не разрешает проход по Wiegand.
    m = UnblockMachine(mask=0)
    m.put_group(1)
    m.put_id(1, b'0000123456')
    parse(m, 6, b'\x01\x03\x01')
    m.checked('WG_readedID', 0x123456)
    assert not m.events and not m.loads
    m.write(m.addr('id_endScan'), 0, 1)
    m.checked('timers_set', 3, 1000)
    m.checked('user_loop', budget=1000000)
    assert bytes(m.u.mem_read(m.device + m.d['timeToGetID'], 2)) == b'\0\0'
    assert m.u.mem_read(m.addr('reader_out'), 1)[0] == 255
    m.checked('WG_readedID', 0x123456)
    assert m.loads and m.events[-1][0] == 71
    # Захват не мешает звонку и команде сервера.
    for caller in ('phone', 'server'):
        m = UnblockMachine(mask=0)
        m.put_group(1)
        m.put_id(1, b'0000123456')
        parse(m, 6, b'\x02\x01\x00')
        if caller == 'phone':
            m.u.mem_write(0x21000600, b'0000123456\0')
            # Телефонный доступ проверяется настоящим путём временного ID.
            assert m.checked('user_addIdToTempBuffer', 0x21000600, 1) == 1
            m.checked('user_incomingcall', 0x21000600, 1)
        else:
            parse(m, 8, b'\x01\x01\x00')
            assert m.answers[-1][:2] == (8, 0)
        assert m.loads and m.u.mem_read(m.device + m.d['timeToGetID'], 1)[0] == 2
    return {'capture_packet_event_only_and_resume': 'PASS'}


def registration_write():
    from test_flash_executor import hold_clear, release_clear
    m = UnblockMachine(mask=0)
    hold_clear(m)
    before = bytes(m.flash)
    assert sms(m, b'#add 1234,0123456789', accepted=0) == []
    assert bytes(m.flash) == before and not m.events
    release_clear(m)
    assert sms(m, b'#add 1234,0123456789') == [b'ADD: 0123456789 - Ok']
    assert m.flash[0x1e8002:0x1e800c] == b'0123456789'
    for arg in (b'', b'123456789', b'12345678901', b'a234567890', b'123456789a'):
        m = UnblockMachine(mask=0)
        assert sms(m, b'#add 1234,' + arg) == [b'ADD: Error "Format not correct"']
        assert not m.events
    for origin in ('sms', 'auto'):
        for fault in ('none', 'full', 'read', 'write'):
            m = UnblockMachine(mask=0)
            m.write(m.addr('mem') + 8 * 8 + 4, 96)
            start, size = struct.unpack('<II', m.u.mem_read(m.addr('mem') + 8 * 8, 8))
            if fault == 'full':
                m.write(m.device + m.d['newAddressTempID'], start + size)
                m.flash[start:start+size] = bytes(size)
            elif fault == 'read':
                m.stubs[m.addr('w25_read_data')] = lambda: m.ret(0)
            elif fault == 'write':
                m.stubs[m.addr('w25_write_data')] = lambda: m.ret(0)
            before = bytes(m.flash[start:start+size])
            if origin == 'sms':
                replies = sms(m, b'#add 1234,0123456789')
                assert replies == ([b'ADD: 0123456789 - Ok'] if fault == 'none' else [b'ADD: Error']), fault
            else:
                # status.autoRegistration — бит 5, определён контрактом структуры status_s.
                status = int.from_bytes(m.u.mem_read(m.device + m.d['status'], 2), 'little')
                m.write(m.device + m.d['status'], status | (1 << 5), 2)
                m.u.mem_write(0x21000600, b'0123456789\0')
                m.checked('user_checkIdAndControl', 0x21000600, 2, budget=2000000)
            additions = [e[0] for e in m.events if e[0] in (67, 68)]
            assert additions == ([67 if origin == 'sms' else 68] if fault == 'none' else []), (origin, fault, m.events)
            assert (bytes(m.flash[start:start+size]) != before) == (fault == 'none')
    return {'invalid_ID_and_write_failures_no_false_success': 'PASS'}


def repeats255():
    for pause in (0, 1):
        m = UnblockMachine()
        m.set_relay(0, 'cnt', 255)
        m.set_relay(0, 'tPause', pause)
        m.triggers_command(hand=1)
        rises = 1
        previous = 1
        for _ in range(2550 * (pause + 1)):
            m.checked('out_process')
            state = m.outputs()[0]
            rises += int(state and not previous)
            previous = state
        assert m.outputs()[0] == 0 and m.checked('out_getmodestate', 0) == 0
        assert rises == (255 if pause else 1), rises
        m.checked('user_unblock_poll')
        assert m.outputs() == (0, 0), 'ИК размыкается после конечной серии'
    return {'255_finite_pulses_and_unblock_completion': 'PASS'}


def gate_sensor():
    def setup(input_index=0, mask=2):
        m = UnblockMachine(mask)
        m.write(m.addr('config') + m.cfg['inputMode'] + input_index, 8, 1)
        m.adc = [0, 0]; m.adc_ok = 1
        m.stubs[m.addr('adc_startInjectedConvertion')] = lambda: m.ret(m.adc_ok)
        # Каналы платы IN1=8, IN2=9; проверяется настоящий порог user_readinputs.
        m.stubs[m.addr('adc_getinjectedvalue_mV')] = lambda: m.ret(m.adc[m.args()[0]-8])
        return m
    def sample(m, one, two=0):
        m.adc = [one, two]; m.checked('user_readinputs')
    def pulses(m): return [x for x in m.loads if x[1] == 3]
    for input_index in (0, 1):
        m = setup(input_index); m.set_trigger('handUnBlock', 1)
        m.checked('user_unblock_recheck', 1)
        assert m.outputs() == (0, 0) and not pulses(m)
        def level(v): sample(m, v if input_index == 0 else 0, v if input_index == 1 else 0)
        for v in (3000, 3001, 3000, 3001): level(v)
        assert not pulses(m), 'дребезг открыл ворота'
        m.adc_ok = 0; level(3001); assert not pulses(m)
        m.adc_ok = 1; level(3001)
        assert pulses(m) == [(0, 3, 1, 0, 1)], 'датчик не дал одиночный импульс'
        assert m.outputs()[1] == 0, 'датчик замкнул ИК'
        for _ in range(5): level(3300)
        m.checked('user_unblock_recheck', 1); level(3300)
        assert len(pulses(m)) == 1, 'закрытый датчик повторил импульс'
        level(0); level(3300); level(3300)
        assert len(pulses(m)) == 1, 'дребезг снял защёлку'
        level(0); level(0); level(3300); level(3300)
        assert len(pulses(m)) == 2, 'новое закрытие не открыло ворота'
        m.set_trigger('handUnBlock', 0); m.checked('user_unblock_recheck', 0)
        level(0); level(0); level(3300); level(3300)
        assert len(pulses(m)) == 2, 'датчик сработал без разблокировки'
    # Включение команды 5 уже даёт открытие; подтверждение датчика не дублирует его.
    m=setup(); sample(m,3300); sample(m,3300); m.triggers_command(hand=1)
    sample(m,3300); sample(m,3300); assert len(pulses(m)) == 1
    # При включении функции через настоящую команду 12 закрытие обнаружится в опросах.
    m=setup(); m.write(m.addr('config')+m.cfg['inputMode'],0,1)
    m.set_trigger('handUnBlock',1); m.checked('user_unblock_recheck',1)
    m.stubs[m.addr('comm_modem_valid_settings')] = lambda: m.ret(1)
    for name in ('comm_endpoints','user_comm_boot','rs485_init'): m.stubs[m.addr(name)] = m.ret
    c=m.addr('config'); m.u.mem_write(c+m.cfg['server1'],b'gate.gsmgate.ru:15010\0')
    m.u.mem_write(c+m.cfg['countryCode'],b'+7\0')
    data=bytearray(m.u.mem_read(c,fields('config_s')['crc']+2)); data[m.cfg['inputMode']]=8
    parse(m,12,data); m.finish_storage(); sample(m,3300); sample(m,3300)
    assert len(pulses(m)) == 1
    m.command(14,record(1)+tail(bytes([1])+bytes(7)),1); m.drain(); sample(m,3300)
    assert len(pulses(m)) == 1, 'команда 14 повторила закрытие'
    # Функция не назначена или отсутствует отдельное реле управления.
    for mask in (0,2,3):
        m=setup(mask=mask); m.set_trigger('handUnBlock',1); m.checked('user_unblock_recheck',1)
        if mask == 2: m.write(m.addr('config')+m.cfg['inputMode'],0,1)
        sample(m,3300); sample(m,3300); assert not pulses(m)
    # Команда 14 включает ранее отсутствовавшую сцену: только датчик открывает.
    m=setup(); m.set_trigger('sheduleUnBlock',1); m.write(m.addr('log_timeKnown'),1,1)
    sample(m,3300); sample(m,3300)
    m.command(14,record(1)+tail(bytes([1])+bytes(7)),1); m.drain()
    assert m.effective() == 2 and not pulses(m) and m.outputs()[1] == 0
    sample(m,3300); assert len(pulses(m)) == 1
    # Датчик сохраняет режим уровня и исходное состояние при снятии.
    for initial in (0,1):
        m=setup(); m.set_relay(0,'mode',1); m.checked('out_load',0,initial,0,0,0)
        m.set_trigger('handUnBlock',1); m.checked('user_unblock_recheck',1)
        sample(m,3300); sample(m,3300); assert m.outputs() == (1,0)
        m.set_trigger('handUnBlock',0); m.checked('user_unblock_recheck',0)
        assert m.outputs() == (initial,1)
    return {'gate_sensor_ADC_bounce_latch_inputs_config12_schedule14_inactive': 'PASS'}
