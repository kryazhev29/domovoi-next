"""Контракты единственного исполнителя в связанном IAR ELF."""
from storage_machine import StorageMachine

NONE, IDCLEAR, EXPIRY, FWSERVICE, UPLOAD, INDEX, CRC, TABLE, PHOTO, FWBLOCK, LOG, CONFIG, STORE, FACTORY = range(14)
IDLE, PENDING, OK, ERROR, CANCELLED = range(5)


def state(m):
    return tuple(m.u.mem_read(m.addr('flash_job'), 3))


def context(m):
    return int.from_bytes(m.u.mem_read(m.addr('flash_job') + 4, 4), 'little')


def finish(m, result=OK):
    m.checked('flash_exec_finish', result)
    assert m.checked('flash_exec_take_result') == result
    assert m.checked('flash_exec_take_result') == IDLE
    assert state(m) == (NONE, 0, IDLE)


def hold_clear(m):
    assert m.checked('idclear_runtime_begin', 0, 73)
    assert state(m) == (IDCLEAR, 1, PENDING)
    assert not m.io_budget and not m.read_budget


def release_clear(m):
    m.checked('idclear_runtime_cancel')
    m.checked('flash_exec_poll', m.now)
    assert state(m) == (IDCLEAR, 1, CANCELLED)
    finish(m, CANCELLED)


def second_begin():
    # Каждая принятая работа исключает второй begin без чтения и записи.
    for kind in range(1, 13):
        m = StorageMachine()
        before = dict(m.counts)
        args = 0
        slot = {IDCLEAR:7, EXPIRY:3, FWSERVICE:4, UPLOAD:9, TABLE:14, FWBLOCK:13}.get(kind,0)
        if slot or kind in (INDEX, PHOTO, LOG):
            import struct
            from test_work_memory import take
            pointer = take(m, slot) if slot else 0
            assert pointer or kind in (INDEX, PHOTO, LOG)
            args = 0x21000800
            step = 7 if kind == INDEX else 1
            if kind == LOG:
                pointer=0x21000400
                m.u.mem_write(pointer,bytes(32))
            m.u.mem_write(args, struct.pack('<IB3x', pointer, step))
        assert m.checked('flash_exec_begin', kind, args)
        accepted = state(m)
        assert accepted == (kind, step if args else 0, PENDING)
        assert m.checked('flash_exec_take_result') == PENDING
        for other in range(1, 13):
            assert not m.checked('flash_exec_begin', other, args if other == kind else 0), 'second begin occupied flash'
            assert state(m) == accepted, 'second begin replaced current kind/step'
            assert m.counts == before and m.io is None
        m.checked('flash_exec_finish', OK)
        assert state(m) == (kind, accepted[1], OK)
        assert not m.checked('flash_exec_begin', STORE, 0)
        assert m.checked('flash_exec_take_result') == OK
        assert m.checked('flash_exec_take_result') == IDLE
        if slot:
            from test_work_memory import drop
            drop(m, slot, pointer)
    return {'kinds': 12, 'conflicts': 144, 'I/O': 0}


def admission_and_result():
    m = StorageMachine()
    m.u.mem_write(0x21000400, bytes(64))
    assert m.checked('w25_async_start', 0x350000, 0x21000400, 64, 0)
    assert not m.checked('flash_exec_begin', STORE, 0)
    assert state(m) == (NONE, 0, IDLE)
    while m.io is not None:
        m.now += 1
        m.checked('w25_async_poll', m.now)
    for result in (OK, ERROR, CANCELLED):
        assert m.checked('flash_exec_begin', STORE, 0)
        m.checked('flash_exec_finish', result)
        assert not m.checked('flash_exec_idle')
        finish(m, result)
        assert m.checked('flash_exec_idle')
    assert state(m) == (NONE, 0, IDLE)
    return {'physical_busy': 'rejected', 'results_once': 3}


def migrated_lifetimes():
    from test_idclear_runtime import ClearMachine, BASE
    from test_service_runtime import ServiceMachine
    from test_work_memory import owned
    m = ClearMachine()
    hold_clear(m)
    pointer = context(m)
    m.checked('flash_exec_poll', 0)
    assert state(m) == (IDCLEAR, 2, PENDING) and m.io is not None
    m.checked('flash_exec_cancel')
    for _ in range(20):
        m.now += 1
        before = dict(m.counts)
        assert not m.checked('idclear_runtime_begin', 0, 74)
        assert not m.checked('service_crc_begin_pack', 0, 74)
        assert not m.checked('idindex_runtime_begin')
        assert m.counts == before and context(m) == pointer
        m.checked('flash_exec_poll', m.now)
        assert m.io_budget <= 1 and m.read_budget <= 64
        if state(m)[2] != PENDING:break
        assert owned(m) & (1 << 6)
    else:raise AssertionError('cancel did not reach safe boundary')
    assert state(m) == (IDCLEAR, 1, CANCELLED)
    assert m.io is None and not m.wires and not owned(m)
    assert m.flash[BASE:BASE+4096] == b'\xff' * 4096
    assert m.flash[BASE+4096:BASE+8192] == b'\x5a' * 4096
    finish(m, CANCELLED)
    m = ServiceMachine(63)
    m.packet(17)
    for _ in range(1000):
        m.now += 1
        m.checked('flash_exec_poll', m.now)
        assert m.read_budget <= 256 and not m.io_budget
        if state(m)[2] != PENDING:break
    else:raise AssertionError('CRC did not finish')
    assert state(m) == (CRC, 4, OK) and m.sent == [m.expected(17)]
    assert not m.checked('flash_exec_idle')
    retained = context(m)
    before = dict(m.counts)
    m.checked('flash_exec_poll', m.now)
    assert state(m) == (CRC, 4, OK) and context(m) == retained and m.counts == before
    finish(m)
    return {'cancel_after_W25_verify': True, 'CRC_oracle': True, 'retained_result': True}


def typed_admission():
    import struct
    from test_idindex_runtime import IndexMachine
    from test_work_memory import owned
    m = IndexMachine()
    m.setup((1,))
    for _ in range(3000):
        m.tick_index()
        if state(m) == (INDEX, 7, PENDING):break
    else:raise AssertionError('INDEX read quantum not reached')
    m.write(m.device + m.d['FWblockIdx'], 1, 2)
    m.write(m.device + m.d['FWblockCount'], 2, 2)
    m.write(m.device + m.d['cchFWupdate'], 0, 1)
    m.u.mem_write(0x21000800, struct.pack('<H', 1) + bytes(1024))
    before = dict(m.counts)
    assert m.checked('fw_take_block', 0x21000800, 1026, 0) == 254, 'second FW begin occupied WORK'
    assert state(m) == (INDEX, 7, PENDING)
    assert not m.exec_active(FWBLOCK) and not owned(m)
    assert m.counts == before and m.io is None
    m = StorageMachine()
    m.u.mem_write(0x21000800, bytes(64))
    m.checked('log_async_begin', 0x21000800)
    assert state(m) == (LOG, 1, PENDING)
    saved = bytes(m.u.mem_read(m.job, 44))
    m.u.mem_write(0x21000800, b'\x5a' * 64)
    before = dict(m.counts)
    m.checked('log_async_begin', 0x21000800)
    assert bytes(m.u.mem_read(m.job, 44)) == saved, 'second LOG begin replaced source'
    assert state(m) == (LOG, 1, PENDING) and m.counts == before
    return {'FW_during_INDEX_read': 'no WORK/I/O', 'LOG_second_begin': 'source retained'}


def background_order():
    from test_stage5_crc import fixture, SyncMachine, BASE
    m = fixture(SyncMachine(), 0, unknown=True, total=8192)
    m.flash[BASE:BASE+128] = bytes(range(128))
    m.write(m.addr('id_used_bytes'), 128)
    m.checked('idindex_runtime_suspend')
    m.checked('idindex_scanned', 0, 1)
    m.checked('user_storage_poll')
    assert m.counts['read'] == 1, 'pick_next order lost CRC background quantum'
    assert m.read_budget == 64 and m.io_budget == 1
    assert state(m) == (INDEX, 2, PENDING) and m.io is not None
    return {'CRC_quantum': 64, 'next_kind_same_call': 'INDEX', 'async_actions': 1}


def mutations():
    from test_flash_mutations import run
    path = 'User/c/user_store.c'
    cases = {
        'executor-order': ('background_order', [(path,
            '    if (!below_crc && idcrc_background_begin()) return;\n    if (idindex_runtime_begin()) return;',
            '    if (idindex_runtime_begin()) return;\n    if (!below_crc && idcrc_background_begin()) return;')]),
        'executor-second-begin': ('second_begin', [(path,
            '  if (!flash_exec_idle()) return false;\n  void *context', '  void *context')]),
    }
    return run(cases, 'test_flash_executor', 'executor-mutations')


def run():
    return {'second_begin': second_begin(), 'admission': admission_and_result(),
            'order': background_order(), 'lifetimes': migrated_lifetimes(),
            'typed_admission': typed_admission()}
