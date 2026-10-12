from test_flash_executor import hold_clear, release_clear
"""E01–E09 на связанном IAR ELF; NOR и транспорт заменены границами модели."""
import struct

from unicorn import UC_HOOK_CODE
from storage_machine import StorageMachine, fields, crc16
from test_unblock import UnblockMachine
from test_stage5_wiegand import frame


def wiegand():
    m = UnblockMachine(mask=0)
    m.write(m.addr('config') + m.cfg['wiegandType'], 1, 1)
    m.stubs[m.addr('idindex_runtime_begin')] = lambda:m.ret(0)
    ids = []
    m.u.hook_add(UC_HOOK_CODE, lambda u, a, n, x: ids.append(m.args()[0]),
                 begin=m.addr('WG_readedID'), end=m.addr('WG_readedID'))
    m.put_group(1)
    for i, value in enumerate((123456, 654321), 1):
        m.put_id(i, b'%010X' % value)

    def send(value):
        raw = frame(value)
        for bit in range(25, -1, -1):
            m.now += 2
            m.checked('EXTI3_IRQHandler' if raw & (1 << bit) else 'EXTI2_IRQHandler')
        m.now += 26
        m.checked('user_loop')
        assert m.u.mem_read(m.addr('wgBitCount'), 1) == b'\0', 'кадр остался в приёмнике'

    hold_clear(m)
    send(123456); send(654321)
    assert ids == [] and m.outputs() == (0, 0)
    release_clear(m)
    m.checked('user_loop'); assert ids == [123456] and m.outputs()[0] == 1, (ids, m.outputs(), m.events, m.loads)
    m.checked('user_loop'); assert ids == [123456, 654321]
    # Свободная флеш: реле включается в том же проходе после паузы 26 мс.
    send(123456); assert ids == [123456, 654321, 123456]
    hold_clear(m)
    for value in (123456, 654321, 123456, 654321, 999999): send(value)
    assert int.from_bytes(m.u.mem_read(m.addr('wg_overflows'), 4), 'little') == 1
    release_clear(m)
    for _ in range(5): m.checked('user_loop')
    assert ids[-4:] == [123456, 654321, 123456, 654321]
    return {'frames': len(ids), 'overflow': 1, 'free_flash_relay_passes': 1}


def scenes():
    m = UnblockMachine(); m.schedule(); m.checked('user_unblock_recheck', 1)
    assert m.effective() == 2
    outputs = m.outputs()
    m.clock(7, 0)
    hold_clear(m)
    reads = m.counts['read']; m.checked('user_unblock_recheck', 0)
    assert m.effective() == 2 and m.outputs() == outputs
    assert m.u.mem_read(m.addr('unblock_pending'), 1) == b'\1'
    assert m.counts['read'] == reads
    release_clear(m); m.checked('user_unblock_poll')
    assert m.effective() == 0
    # Отказ хвоста, записи, повторного хвоста и праздников также сохраняет выполненную сцену.
    for failure in (1, 2, 3, 4):
        m = UnblockMachine(); m.schedule(holiday=3); m.checked('user_unblock_recheck', 1)
        assert m.effective() == 2
        m.fail_read(failure); m.checked('user_unblock_recheck', 0)
        assert m.effective() == 2 and m.u.mem_read(m.addr('unblock_pending'), 1) == b'\1'
        m.fault = None; m.checked('user_unblock_poll')
        assert m.u.mem_read(m.addr('unblock_pending'), 1) == b'\0'
    return {'owner_bit7': True, 'read_failures': 4}


def server_frame():
    m = StorageMachine(); hold_clear(m)
    body = struct.pack('<HBBBB', 4, 0, 7, 2, 60)
    packet = b'SLDN' + body + struct.pack('<H', crc16(body))
    m.u.mem_write(0x21000000, packet)
    assert m.checked('user_comm_rx', 0x21000000, len(packet)) == len(packet)
    m.checked('user_comm_rx_done', len(packet), 0); m.checked('server_rx_process')
    assert not m.answers
    saved = bytes(m.u.mem_read(m.addr('server_rx_buffer'), len(packet)))
    m.now = 31000; m.checked('server_rx_begin_pass')
    assert bytes(m.u.mem_read(m.addr('server_rx_buffer'), len(packet))) == saved
    assert m.checked('user_comm_rx_capacity') == 2048 - len(packet)
    m.u.mem_write(0x21000000, b'\xa5' * 2048)
    assert m.checked('user_comm_rx', 0x21000000, 2048) == 2048 - len(packet)
    assert m.checked('user_comm_rx', 0x21000000, 1) == 0
    assert bytes(m.u.mem_read(m.addr('server_rx_buffer'), len(packet))) == saved
    m.checked('user_comm_rx_done', 2048 - len(packet), 0)
    release_clear(m)
    m.checked('server_rx_process'); m.checked('server_rx_process')
    assert m.answers == [(2, 0, 1, 7)], m.answers
    assert m.u.mem_read(m.device + m.d['timeToReset'], 1) == b'\x3c'
    m = StorageMachine(); m.u.mem_write(0x21000000, packet[:6])
    m.checked('user_comm_rx', 0x21000000, 6); m.checked('user_comm_rx_done', 6, 0)
    m.now = 31000; m.checked('server_rx_begin_pass')
    assert m.checked('user_comm_rx_capacity') == 2048
    return {'ready_wait_ms': 31000, 'executions': 1, 'backpressure': True}


def rollback():
    m = UnblockMachine(); m.write(m.addr('trial'), 1, 1)
    m.stubs[m.addr('expiry_runtime_begin')] = lambda: m.ret(0)
    resets, writes = [], []
    m.stubs[m.addr('user_savereset')] = lambda: (resets.append(True), m.ret())
    def failed_write():
        writes.append(m.args()[0]); m.ret(0)
    m.stubs[m.addr('w25_write_data')] = failed_write
    # Срок пробы вышел: метка старта на 30 минут раньше текущих миллисекунд.
    m.write(m.addr('trial_started'), (m.checked('timers_getMillis') - 1800000) & 0xffffffff)
    def second():
        m.checked('timers_set', 3, 1000); m.checked('user_loop')
    second()
    assert writes == [0x20024] and not resets
    assert m.flash[0x20024:0x20028] == b'\xff'*4
    second(); assert len(writes) == 2 and not resets
    m.stubs[m.addr('w25_write_data')] = m.sync_write
    second()
    assert resets == [True] and m.flash[0x20024:0x20028] == bytes(4)
    return {'failed_attempts': 2, 'reset_after_verified_write': True}


def factory_reset():
    m = UnblockMachine(); starts = []
    m.stubs[m.addr('expiry_runtime_begin')] = lambda: m.ret(0)
    m.stubs[m.addr('user_writeConfigAndDevice')] = lambda: m.ret(1)
    m.stubs[m.addr('store_factory_begin')] = lambda: (starts.append(True), m.ret(1))
    m.stubs[m.addr('user_initvariables')] = m.ret
    timer = m.device + m.d['timeToDefault']
    m.write(timer, 2, 1); hold_clear(m)
    def second():
        m.checked('timers_set', 3, 1000); m.checked('user_loop')
    second(); second(); second()
    assert m.u.mem_read(timer, 1) == b'\1' and not starts
    # Прямой вход (в том числе SMS) также удерживает запрос при занятой флеш.
    m.checked('user_factoryReset')
    assert m.u.mem_read(timer, 1) == b'\1' and not starts
    release_clear(m); second()
    assert starts == [True] and m.u.mem_read(timer, 1) == b'\0'
    second(); assert starts == [True]
    return {'busy_seconds': 3, 'reset_starts': 1, 'direct_busy_request': True}


def firmware_events():
    # Очередь из 16 обычных событий не должна поглотить ошибку загрузчика.
    for result, event in ((1, 133), (8, 137)):
        m = UnblockMachine(); m.write(0x08007ffc, 0x32, 1)
        m.stubs.pop(m.addr('log_addSystemEvent'))
        m.write(m.device + m.d['registredOnServer'], 1, 1)
        m.flash[0x2000c:0x20010] = struct.pack('<I', result)
        for _ in range(16): assert m.checked('log_addSystemEvent', 13) == 1
        def second():
            m.checked('timers_set', 3, 1000); m.checked('user_loop')
        m.stubs[m.addr('expiry_runtime_begin')] = lambda: m.ret(0)
        second()
        assert m.flash[0x20014:0x20018] == b'\xff'*4, (result, m.u.mem_read(m.addr('log_pendingCount'), 1), m.flash[0x20014:0x20018])
        assert m.u.mem_read(m.addr('log_pendingCount'), 1) == b'\x10'
        # Освободить одно место: здесь проверяется приём, долговечность — прежними тестами журнала.
        m.write(m.addr('log_pendingCount'), 15, 1)
        m.stubs[m.addr('w25_write_data')] = lambda: m.ret(0)
        second()
        pending = m.addr('log_pending') + 15*32
        assert m.u.mem_read(pending + fields('cellLog_s')['logType'], 1) == bytes([event])
        assert m.flash[0x20014:0x20018] == b'\xff'*4
        m.stubs[m.addr('w25_write_data')] = m.sync_write
        second()
        assert m.flash[0x20014:0x20018] == bytes(4)
        assert m.u.mem_read(m.addr('log_pendingCount'), 1) == b'\x10', 'дубль после отказа метки'
    # Другие FW-источники удерживаются в малой очереди; новые команды ждут.
    m = StorageMachine()
    for _ in range(16): assert m.checked('log_addSystemEvent', 13) == 1
    m.checked('fw_event_request', 133, 9); m.checked('fw_event_request', 137, 0)
    assert m.checked('flash_exec_idle') and not m.checked('store_command_allowed', 19)
    m.checked('fw_event_poll'); assert m.checked('fw_event_waiting')
    m.write(m.addr('log_pendingCount'), 14, 1)
    m.checked('fw_event_poll'); m.checked('fw_event_poll')
    off = fields('cellLog_s')['logType']
    assert [m.u.mem_read(m.addr('log_pending') + i*32 + off, 1)[0] for i in (14, 15)] == [133, 137]
    assert not m.checked('fw_event_waiting')
    return {'boot_events': [133, 137], 'reported_retry_without_duplicate': True, 'runtime_fifo': 2}


def manual_read():
    m = StorageMachine(); m.stubs[m.addr('w25_read_data')] = lambda: m.ret(0)
    assert m.checked('manual_stage_read', 0, 0x21000000, 16) == 0
    m.stubs[m.addr('w25_read_data')] = m.read
    assert m.checked('manual_stage_read', 0, 0x21000000, 16) == 1
    assert m.checked('manual_stage_read', 32768, 0x21000000, 1) == 0
    return {'io_failure': 0, 'success': 1, 'range_guard': True}


def manual_write():
    m = StorageMachine(); m.u.mem_write(0x21000000, b'\x5a'*16)
    assert m.checked('flash_exec_begin', 12, 0) # Владелец — команда USB 110.
    m.stubs[m.addr('w25_write_data')] = lambda: m.ret(0)
    assert m.checked('manual_stage_write', 0, 0x21000000, 16) == 0
    m.stubs[m.addr('w25_write_data')] = m.sync_write
    assert m.checked('manual_stage_write', 0, 0x21000000, 16) == 1
    assert m.checked('manual_stage_write', 32768, 0x21000000, 1) == 0
    return {'io_failure': 0, 'success': 1, 'range_guard': True}


def manual_erase():
    m = StorageMachine(); m.stubs[m.addr('w25_erase_sector')] = lambda: m.ret(0)
    assert m.checked('flash_exec_begin', 12, 0)
    assert m.checked('manual_stage_erase', 0) == 0
    m.stubs[m.addr('w25_erase_sector')] = m.sync_erase
    assert m.checked('manual_stage_erase', 0) == 1
    assert m.checked('manual_stage_erase', 1) == 0
    assert m.checked('manual_stage_erase', 32768) == 0
    return {'io_failure': 0, 'success': 1, 'range_guards': 2}
