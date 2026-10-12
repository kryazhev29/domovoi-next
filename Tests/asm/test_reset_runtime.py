"""Перезапуск и срок пробы в связанном ELF; плата и сервер не используются."""
import struct

from storage_machine import StorageMachine
from test_app_update_elf import machine, JOURNAL, image_crc
from unicorn import UC_HOOK_MEM_WRITE


class ResetReached(Exception):
    pass


class ResetMachine(StorageMachine):
    """Время NOR задано моделью; user_loop, хранение и SYSRESETREQ выполняет ELF."""
    def __init__(self):
        super().__init__()
        self.u.mem_map(0x1ffff000, 0x1000)
        self.write(0x1ffff7e0, 128, 2)
        self.write(0x08007ffc, 0x32, 1)
        self.write(0x08007ffd, 139, 1)
        self.write(self.addr('mem') + 6 * 8, 0x1cd400)
        self.write(self.addr('mem') + 6 * 8 + 4, 4096)
        self.write(self.addr('store_runtime'), 0, 1)
        assert self.checked('mem_read', 0x1cd400, 0x21000000, 1)
        self.write(self.addr('log_timeKnown'), 1, 1)
        self.write(self.device + self.d['datetime'], 1791331200)
        self.stubs.pop(self.addr('wdt_reload'))
        self.last_reload = self.now
        self.reloads = self.max_gap = 0
        self.reset_at = None
        self.ready_at = 0
        self.u.hook_add(UC_HOOK_MEM_WRITE, self.watch, begin=0x40003000, end=0x40003003)
        self.u.hook_add(UC_HOOK_MEM_WRITE, self.watch, begin=0xe000ed0c, end=0xe000ed0f)
        self.checked('log_getRecordsCount', budget=1000000)
        self.startup = {'format_ms': self.now, 'max_reload_gap_ms': self.max_gap}
        assert self.max_gap < 3000
        self.write(self.addr('store_runtime'), 1, 1)
        self.now = self.last_reload = self.max_gap = self.reloads = 0

    def watch(self, u, access, address, size, value, data):
        if address == 0x40003000 and value == 0xaaaa:
            self.max_gap = max(self.max_gap, self.now - self.last_reload)
            self.last_reload = self.now
            self.reloads += 1
        elif address == 0xe000ed0c:
            assert value & 0xffff0004 == 0x05fa0004, 'неверный SYSRESETREQ'
            assert self.now - self.last_reload < 3000, 'IWDG истёк перед SYSRESETREQ'
            assert self.io is None and not self.read8('log_pendingCount'), 'сброс до сохранения журнала'
            self.reset_at = self.now
            raise ResetReached()

    def read8(self, name):
        return int.from_bytes(self.u.mem_read(self.addr(name), 1), 'little')

    def start(self):
        address, source, length, now = self.args()
        super().start()
        if self.io is not None:
            self.ready_at = self.now + (4 if source else 400)

    def poll(self):
        if not self.read8('store_runtime'):
            self.now = max(self.now, self.ready_at)
        if self.now < self.ready_at:
            self.io_budget += 1
            self.ret(1)
        else:
            super().poll()

    def advance(self, milliseconds=10):
        self.now += milliseconds
        for index in range(6):
            address = self.addr('timer') + 4 * index
            value = int.from_bytes(self.u.mem_read(address, 4), 'little')
            self.write(address, value + milliseconds)
        assert self.now - self.last_reload < 3000, 'IWDG истёк до SYSRESETREQ'

    def candidate(self):
        # Максимально допустимый образ; CRC и обслуживание сторожа исполняются целиком.
        data = struct.pack('<II', 0x20004ff8, 0x08008209) + bytes(0x17e00 - 8)
        self.flash[:32] = struct.pack('<IBHBIIII8s', 0xffffffff, 139, 71, 255,
                                     len(data), image_crc(data), 0xffffffff, 1791331200, b'\xff' * 8)
        self.flash[512:512 + len(data)] = data


def reset_paths():
    results = []
    for mode in ('command', 'update', 'queued', 'sync'):
        m = ResetMachine()
        if mode == 'update':
            m.candidate()
            m.write(m.device + m.d['rebootToUpdate'], 1, 1)
        elif mode == 'command':
            m.write(m.device + m.d['timeToReset'], 1, 1)
        else:
            for _ in range(16 if mode == 'queued' else 1):
                assert m.checked('log_addSystemEvent', 33)
            m.checked('user_saveindex')
        transitions = []
        try:
            if mode in ('queued', 'sync'):
                if mode == 'sync':
                    m.write(m.addr('store_runtime'), 0, 1)
                m.call('user_savereset', budget=1000000)
            for _ in range(1500):
                m.advance()
                m.checked('user_loop', budget=8000000)
                m.call('user_storage_poll', budget=100000)
                state = (m.exec_state(), bytes(m.u.mem_read(m.addr('cfg_job') + 30, 5)).hex())
                if not transitions or transitions[-1][1] != state:
                    transitions.append((m.now, state))
            raise AssertionError(('перезапуск не дошёл до SYSRESETREQ за 15 с модели', mode, m.exec_state(), m.read8('store_reset_requested'), m.checked('mem_config_result'), transitions[-16:]))
        except ResetReached:
            pass
        assert m.reloads and m.max_gap < 3000
        if mode == 'update':
            assert m.flash[:4] == struct.pack('<I', 0x4e465744), 'кандидат не опубликован'
            assert m.reloads >= 0x17e00 // 128, 'CRC образа не обслуживал сторож'
        results.append({'path': mode, 'reset_ms': m.reset_at,
                        'max_reload_gap_ms': m.max_gap, 'reloads': m.reloads,
                        'startup': m.startup})
    return results


def scheduled_reset():
    from iar_layout import fields
    from test_flash_executor import hold_clear, release_clear
    for reason in ('call', 'download', 'install', 'flash', 'disabled', 'early'):
        m = StorageMachine(); requested = []
        m.stubs[m.addr('user_savereset')] = lambda: (requested.append('reset'), m.ret())[-1]
        m.stubs[m.addr('log_addSystemEvent')] = lambda: m.ret(1)
        m.stubs[m.addr('app_boot_request')] = lambda: m.ret(0xffffffff)
        m.write(m.addr('config') + fields('config_s')['restartTime'], 0 if reason == 'disabled' else 1, 1)
        m.write(m.device + m.d['worktime'], 3599 if reason == 'early' else 3600, 4)
        m.stubs[m.addr('comm_call_active')] = lambda: m.ret(reason == 'call')
        if reason == 'download': m.write(m.device + m.d['FWinProgress'], 1, 1)
        if reason == 'install': m.write(m.device + m.d['rebootToUpdate'], 1, 1)
        if reason == 'flash': hold_clear(m)
        def minute():
            m.checked('timers_set', 5, 60000)
            m.checked('user_loop', budget=1000000)
        minute(); assert not requested, reason
        if reason == 'flash': release_clear(m)
        m.stubs[m.addr('comm_call_active')] = lambda: m.ret(0)
        for field in ('FWinProgress', 'rebootToUpdate'):
            m.write(m.device + m.d[field], 0, 1)
        m.write(m.addr('config') + fields('config_s')['restartTime'], 1, 1)
        m.write(m.device + m.d['worktime'], 3600, 4)
        minute(); assert requested == ['reset'], reason
    return {'deferred_until_idle_then_next_minute': True}


def mutations():
    from test_flash_mutations import run
    return run({
        'reset-never-finishes': ('reset_paths', [('User/c/user_store.c',
            'if (log_pendingCount || store_save_requested) {',
            'if (log_pendingCount || store_save_requested || store_reset_requested) {')]),
        'watchdog-not-serviced': ('reset_paths', [('Modules/wdt_stm32f10x.c',
            'IWDG->KR = 0xAAAAu;', '(void)0;')]),
        'trial-out-of-bounds': ('trial_clock', [('Bootloader/app_update.c',
            'return trial && (uint32_t)(timers_getMillis() - trial_started) >= TRIAL_SECONDS * 1000u;',
            'return trial && timers_get(6) >= TRIAL_SECONDS * 1000u;')]),
    }, 'test_reset_runtime', 'reset-runtime-mutations')


def trial_clock():
    results = []
    for started in (100, 0xfffffff0):
        m = machine()
        m.now = started
        m.flash[JOURNAL + 4:JOURNAL + 8] = bytes(4)
        m.flash[JOURNAL + 24:JOURNAL + 28] = bytes(4)
        m.checked('app_boot_metadata')
        assert m.checked('app_trial_active') == 1
        assert not m.checked('app_trial_expired')

        # Настоящие start/poll оставляют data после завершения записи W25.
        # SPI и содержимое NOR — модель; дескриптор и таймер пробы — код ELF.
        for name in ('w25_async_start', 'w25_async_poll', 'w25_async_active'):
            m.stubs.pop(m.addr(name), None)
        for name in ('spi_send_byte', 'spi_send'):
            m.stubs[m.addr(name)] = m.ret
        m.stubs[m.addr('spi_ready')] = lambda: m.ret(1)
        m.stubs[m.addr('w25_read_status')] = lambda: m.ret(2)
        source = m.addr('log_pending')
        m.u.mem_write(source, bytes(16))
        m.flash[0x1d0000:0x1d0010] = bytes(16)
        assert m.checked('w25_async_start', 0x1d0000, source, 16, m.now)
        assert m.checked('w25_async_poll', m.now) == 2
        assert not m.checked('w25_async_active')
        assert not m.checked('app_trial_expired'), 'запись W25 преждевременно завершила пробу'

        # В ReleaseCustomer за timer[5] расположен указатель передачи модема.
        assert m.checked('comm_uart_tx', source, 16)
        assert not m.checked('app_trial_expired'), 'передача UART преждевременно завершила пробу'

        for elapsed in (1799999, 1800000):
            m.now = (started + elapsed) & 0xffffffff
            assert bool(m.checked('app_trial_expired')) == (elapsed >= 1800000), 'срок пробы не равен 30 минутам'
        m.checked('app_trial_confirm')
        assert not m.checked('app_trial_active') and not m.checked('app_trial_expired')
        results.append({'start': started, 'deadline_ms': 1800000})
    return results
