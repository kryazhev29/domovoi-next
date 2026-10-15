"""Главные контракты шагов 4–7 единого исполнителя, без модели платы."""
import struct

from test_flash_executor import state, STORE, FWSERVICE, NONE, PENDING
from test_memory_runtime import config_machine
from test_fw_service_runtime import FirmwareMachine


def nested_config():
    m = FirmwareMachine()
    m.fixture()
    m.begin(2)
    seen = False
    for _ in range(3000):
        if m.checked('mem_config_result') == 1:
            seen = True
            assert state(m)[0] == FWSERVICE, 'A/B changed the accepted kind'
            assert not m.sent, 'reply preceded A/B verify'
        m.tick_fw()
        if m.phase() == 16:
            assert state(m)[0] == NONE, 'ready reply retained kind'
            break
    else:
        raise AssertionError('FW-service did not finish A/B')
    assert seen
    m.drain()
    assert len(m.sent) == 1 and m.sent[0][8:10]==bytes((11,0))
    return {'nested_kind': 'FWSERVICE', 'reply_after_verify': True}


def factory_reset():
    outcomes = []
    for channel, fail, reset in ((0, False, False), (0, True, False), (0, False, True),
                                  (1, False, False), (1, True, False),
                                  (2, False, False), (2, True, False)):
        m, _, _ = config_machine()
        # Небольшие реальные диапазоны, включая частичный сектор таблицы.
        blocks = {2: (0x41000, 4096), 3: (0x1c9000, 4096),
                  4: (0x1ca800, 2048), 5: (0x1cc000, 1024),
                  7: (0x1d0000, 4096), 8: (0x1e8000, 4096), 9: (0x200000, 4096)}
        for mb, (address, size) in blocks.items():
            m.u.mem_write(m.addr('mem') + 8 * mb, struct.pack('<II', address, size))
            m.flash[address:address + size] = b'\x5a' * size
        m.flash[0x1ca000:0x1ca800] = b'\x37' * 2048
        m.write(m.addr('store_runtime'), 1, 1)
        m.stubs[m.addr('user_comm_boot')] = m.ret
        m.stubs[m.addr('w25_read_devID')] = lambda: m.ret(0x17c2)
        # Завершение factory не вызывает стартовое форматирование журнала.
        def startup_format():
            raise AssertionError('factory used startup synchronous format')
        m.stubs[m.addr('log_getRecordsCount')] = startup_format
        m.stubs[m.addr('log_addSystemEvent')] = lambda: m.ret(1)
        zero_fields = {'FWinProgress': 1, 'updateTimeout': 2, 'FWblockCount': 2, 'FWblockIdx': 2,
                       'numbersCount': 2, 'tempIDsCount': 2, 'logsCount': 2, 'countPhoto': 2}
        for name, width in zero_fields.items(): m.write(m.device + m.d[name], 7, width)
        m.u.mem_write(m.device + m.d['IMEIGSM'], b'867442070217472\0')
        m.write(m.device + m.d['firmwareVersion'], 71, 2)
        ready = False; messages = []
        m.stubs[m.addr('comm_tx_busy')] = lambda: m.ret(not ready)
        number = 0x21000c00
        m.u.mem_write(number, b'89035551122\0')
        def sms_reply():
            if ready:
                args = m.args()
                messages.append(tuple(bytes(m.u.mem_read(p, 32)).split(b'\0', 1)[0] for p in args[:2]))
            m.ret(ready)
        m.stubs[m.addr('user_comm_sms')] = sms_reply
        assert m.checked('store_factory_request', channel, 73, 0, number if channel == 2 else 0)
        m.checked('user_factoryReset')
        assert state(m)[0] == STORE and not m.answers
        if reset: m.write(m.addr('usb_bus_resets'), 1, 4)
        if fail:
            m.fault = ('poll', 1)
        for _ in range(5000):
            m.now += 1
            m.checked('user_storage_poll')
            if state(m)[0] != STORE:
                break
            assert not m.answers and not messages, 'factory success before completion'
        else:
            raise AssertionError('factory reset did not finish')
        for _ in range(3): m.checked('user_storage_poll')
        if channel or reset: assert not m.answers and not messages, 'занятый или старый канал получил ответ'
        ready = True
        for _ in range(3): m.checked('user_storage_poll')
        assert m.answers == ([] if reset or channel == 2 else [(3, 2 if fail else 0, channel, 73)]), m.answers
        assert messages == ([(b'89035551122', b'DEFAULT: Error' if fail else b'DEFAULT: Complete')] if channel == 2 else []), messages
        if not fail:
            for address, size in blocks.values():
                assert m.flash[address:address + size] == b'\xff' * size
            assert m.flash[0x1ca000:0x1ca800] == b'\x37' * 2048
            # Сохранённый и рабочий device очищены одинаково; FW не возобновится после старта.
            active = int.from_bytes(m.u.mem_read(m.addr('cfg_cached_active'), 4), 'little')
            saved = m.flash[active + 512:active + 1024]
            for name, width in zero_fields.items():
                offset = m.d[name]
                assert saved[offset:offset+width] == bytes(width), ('saved', name)
                assert bytes(m.u.mem_read(m.device + offset, width)) == bytes(width), ('live', name)
            for offset in (m.d['newAddressTempID'], m.d['newAddressLog']):
                assert saved[offset:offset+4] == bytes(m.u.mem_read(m.device + offset, 4))
            assert saved[m.d['IMEIGSM']:m.d['IMEIGSM']+15] == b'867442070217472'
            assert saved[m.d['firmwareVersion']:m.d['firmwareVersion']+2] == struct.pack('<H', 71)
            m.checked('user_storage_poll')
            assert len(m.answers) + len(messages) == (0 if reset else 1)
        outcomes.append({'channel': channel, 'failure': fail, 'USB_reset': reset})
    return outcomes


def mutations():
    from test_flash_mutations import run
    return run({'step4-nested-config': ('nested_config', [
        ('User/c/memory_runtime.c',
         'flash_job.kind != FX_STORE && flash_job.kind != FX_FWSERVICE &&',
         'flash_job.kind != FX_STORE &&'),
    ]), 'step6-short-write': ('short_write_ownership', [
        ('Bootloader/app_update.c',
         'if (!trial || !flash_exec_begin(FX_STORE, &args)) return;',
         'if (!trial) return;'),
    ])}, 'test_flash_executor_steps47', 'executor-steps47-mutations')


def run_step4():
    return {'nested_config': nested_config(), 'factory': factory_reset(), 'completion': store_completion()}


def store_completion():
    from test_access_reads import AccessMachine, IDS
    from storage_machine import fields
    peaks = []
    for fail, reset in ((False, False), (True, False), (False, True)):
        m = AccessMachine(); m.put_id(1, b'0123456789')
        assert m.checked('counter_boot', budget=1000000)
        record = bytearray(m.flash[IDS:IDS+32])
        record[fields('cellID_s')['cntPass']] = 0
        m.u.mem_write(0x21000400, bytes(record))
        assert m.checked('store_id_begin', IDS, 0x21000400)
        m.checked('store_access_reply', 0, 97)
        offsets = fields('store_io_s')
        for name, value in (('outputs', 3), ('type', 6), ('count', 0), ('started', 1), ('event', 74)):
            m.write(m.job + offsets[name], value, 2 if name == 'event' else 1)
        if fail: m.stubs[m.addr('w25_write_data')] = lambda: m.ret(0)
        if reset: m.write(m.addr('usb_bus_resets'), 1, 4)
        for _ in range(3000):
            m.checked('user_storage_poll')
            if state(m)[0] == NONE: break
            assert not m.loads and not m.events and not m.answers
        else: raise AssertionError('ID completion stuck')
        m.checked('user_storage_poll')
        assert len(m.loads) == (0 if fail else 2), m.loads
        if fail: assert not m.checked('flash_exec_read_allowed')
        assert m.events == ([] if fail else [(74, b'0123456789', 0), (131, b'0123456789', 0),
                                            (79, b'0123456789', 0), (139, b'0123456789', 255)]), m.events
        assert m.answers == ([] if reset else [(8, 2 if fail else 0, 0, 97)]), m.answers
        peaks.append(m.stack_peak)
    return {'id_events_outputs_reply_preserved': True, 'stack_peak': max(peaks)}


def reads_between_steps():
    from test_unblock import UnblockMachine
    from test_flash_executor import hold_clear, release_clear, finish, LOG
    m = UnblockMachine(mask=0)
    m.put_group(1)
    m.put_id(1, b'0000000042')
    m.u.mem_write(0x21000800, bytes(32))
    m.u.mem_write(0x21000900, struct.pack('<IB3x', 0x21000800, 1))
    assert m.checked('flash_exec_begin', LOG, 0x21000900)
    assert m.checked('flash_exec_read_allowed')
    m.u.mem_write(0x21000a00, b'0000000042\0')
    assert m.checked('user_access_ready', 0x21000a00)
    packet = struct.pack('<HBBB', 16, 255, 7, 8) + bytes((1, 1, 0)) + b'0000000042'
    m.u.mem_write(0x21000b00, packet)
    assert m.checked('user_command_ready', 0x21000b00)
    m.checked('user_checkIdAndControl', 0x21000a00, 2)
    assert m.outputs()[0] == 1
    assert m.checked('w25_async_start', 0x300000, 0, 0, 0)
    assert not m.checked('flash_exec_read_allowed')
    assert not m.checked('user_access_ready', 0x21000a00)
    while m.io is not None:
        m.now += 1
        m.checked('w25_async_poll', m.now)
    finish(m)
    hold_clear(m)
    assert not m.checked('flash_exec_read_allowed')
    release_clear(m)
    return {'pure_card_between_steps': True, 'physical_busy_and_tables': 'WAIT'}


def run():
    return {**run_step4(), 'reads': reads_between_steps(), 'short_write': short_write_ownership()}


def short_write_ownership():
    from storage_machine import StorageMachine
    from test_flash_executor import hold_clear, release_clear
    m = StorageMachine()
    m.write(0x08007ffc, 0x40, 1)
    m.flash[0x20004:0x20008] = bytes(4)
    m.flash[0x20018:0x2001c] = bytes(4)
    m.checked('app_boot_metadata')
    assert m.checked('app_trial_active')
    writes = []
    def write():
        writes.append(state(m))
        assert state(m)[0] == STORE and state(m)[2] == PENDING, 'write bypassed executor'
        m.ret(0)
    m.stubs[m.addr('w25_write_data')] = write
    hold_clear(m)
    m.checked('app_trial_confirm')
    assert not writes and m.checked('app_trial_active'), 'BUSY changed the trial marker'
    release_clear(m)
    m.checked('app_trial_confirm')
    assert len(writes) == 1 and m.checked('app_trial_active') and state(m)[0] == NONE
    m.stubs[m.addr('w25_write_data')] = m.sync_write
    m.checked('app_trial_confirm')
    assert not m.checked('app_trial_active') and state(m)[0] == NONE
    return {'BUSY': 'retained', 'IO-fault': 'trial retained', 'success': 'STORE released'}
