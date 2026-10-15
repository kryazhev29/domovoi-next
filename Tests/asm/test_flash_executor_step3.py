"""Источники шага 3 до verify, публикация после OK и постоянные курсоры PHOTO."""
from storage_machine import StorageMachine,fields
from test_flash_executor import TABLE, PHOTO, FWBLOCK, LOG, PENDING, OK, state
from test_work_memory import owned


def reject_second(m):
    accepted = state(m)
    before = dict(m.counts)
    for kind in range(1, 13):
        assert not m.checked('flash_exec_begin', kind, 0)
        assert state(m) == accepted and m.counts == before


def log_fixture():
    from test_stage5_log import boot
    from test_upload_runtime import log
    m = StorageMachine()
    boot(m)
    source = m.addr('log_pending')
    m.u.mem_write(source, log(123))
    m.write(m.addr('log_pendingCount'), 1, 1)
    m.write(m.addr('log_timeKnown'), 1, 1)
    m.checked('log_async_begin', source)
    assert state(m) == (LOG, 1, PENDING)
    m.u.mem_write(source, b'\xa5' * 32)
    return m


def source_until_verify():
    m = log_fixture()
    waits = 0
    for _ in range(1000):
        reject_second(m)
        m.now += 1
        m.checked('flash_exec_poll', m.now)
        if m.io:
            _, pointer, size, data, _ = m.io
            if pointer and m.job <= pointer < m.job + 32:
                waits += 1
                assert bytes(m.u.mem_read(pointer, size)) == data, 'LOG source reused before verify'
        if state(m)[2] != PENDING:
            break
    else:
        raise AssertionError('LOG did not finish')
    assert waits >= 6 and state(m)[2] == OK and m.io is None
    assert int.from_bytes(m.flash[0x1d0020:0x1d0024], 'little') == 123
    assert m.checked('flash_exec_take_result') == OK
    assert not owned(m)
    return {'LOG_verify_waits': waits, 'input_reused': True}


def publication_after_ok():
    m = log_fixture()
    before = m.u.mem_read(m.device + m.d['logsCount'], 2)
    for _ in range(1000):
        m.now += 1
        m.checked('flash_exec_poll', m.now)
        count = m.u.mem_read(m.device + m.d['logsCount'], 2)
        if count != before:
            assert state(m)[2] == OK and m.io is None, 'LOG published before verified OK'
            assert int.from_bytes(count, 'little') == 1
            break
        assert state(m)[2] == PENDING
    else:
        raise AssertionError('LOG did not publish')
    assert m.u.mem_read(m.addr('log_pendingCount'), 1) == b'\0'
    assert m.checked('flash_exec_take_result') == OK
    return {'LOG_publication': 'OK, W25 idle'}


def table_and_fw():
    from test_table_runtime import rows
    m = StorageMachine()
    m.command(13, rows([1, 2]), 2)
    assert state(m) == (TABLE, 5, PENDING)
    snapshot = int.from_bytes(m.u.mem_read(m.job, 4), 'little')
    assert snapshot and owned(m) == 1 << 13
    for _ in range(5000):
        reject_second(m)
        assert not m.checked('flash_exec_read_allowed')
        assert not m.u.mem_read(m.device + m.d['numbersCount'], 2).strip(b'\0')
        m.now += 1
        m.checked('flash_exec_poll', m.now)
        if state(m)[2] != PENDING:
            break
        assert int.from_bytes(m.u.mem_read(m.job, 4), 'little') == snapshot
        assert owned(m) == 1 << 13
    else:
        raise AssertionError('TABLE did not finish')
    assert state(m)[2] == OK and m.io is None and not owned(m)
    assert int.from_bytes(m.u.mem_read(m.device + m.d['numbersCount'], 2), 'little') == 2
    assert m.checked('flash_exec_take_result') == OK

    m = StorageMachine()
    m.write(m.device + m.d['FWblockCount'], 2, 2)
    m.write(m.device + m.d['cchFWupdate'], 1, 1)
    payload = bytes((i * 13 + 7) & 255 for i in range(1024))
    m.u.mem_write(0x21000000, bytes(2) + payload)
    replies = []
    def reply():
        assert state(m)[2] == OK and m.io is None
        replies.append(m.args())
        m.ret()
    m.stubs[m.addr('user_send_statusUpdateFW')] = reply
    assert m.checked('fw_take_block', 0x21000000, 1026, 1) == 252
    assert state(m) == (FWBLOCK, 2, PENDING)
    m.u.mem_write(0x21000000, b'\xa5' * 1026)
    for _ in range(500):
        reject_second(m)
        m.now += 1
        m.checked('flash_exec_poll', m.now)
        if state(m)[2] != PENDING:
            break
        assert m.u.mem_read(m.device + m.d['FWblockIdx'], 2) == b'\0\0'
    else:
        raise AssertionError('FWBLOCK did not finish')
    assert m.flash[512:1536] == payload and len(replies) == 1
    assert not owned(m) and m.checked('flash_exec_take_result') == OK
    return {'TABLE_records': 2, 'FWBLOCK_bytes': 1024}


def photo_persistent():
    from test_photo_runtime import PhotoMachine
    from test_photo_edges import frame
    m = PhotoMachine()
    m.boot_media()
    m.begin_camera(58)
    m.next_request()
    m.feed(frame(b'x' * 58))
    original = m.event
    def event():
        assert state(m)[2] == OK and m.io is None, 'PHOTO published before OK'
        original()
    m.stubs[m.addr('log_addPhotoEvent')] = event
    for _ in range(500):
        if not m.active():
            break
        reject_second(m)
        m.tick()
    else:
        raise AssertionError('PHOTO did not finish')
    assert len(m.events) == 1 and not m.rget('fault')
    persistent_size=fields('photo_runtime_s')['tx_until']+4
    saved = bytes(m.u.mem_read(m.addr('pr'), persistent_size))
    m.u.mem_write(0x21000400, bytes(32))
    m.checked('log_async_begin', 0x21000400)
    assert state(m)[0] == LOG
    assert bytes(m.u.mem_read(m.addr('pr'), persistent_size)) == saved, 'PHOTO persistent overwritten by LOG'
    m.checked('flash_exec_finish', OK)
    assert m.checked('flash_exec_take_result') == OK
    assert m.rget('last_number', 4) == m.events[0] and m.rget('initialized')
    return {'PHOTO_camera': 'verified', 'persistent_after_LOG': True}


def table_recheck():
    from test_unblock import UnblockMachine, record, tail
    m = UnblockMachine()
    m.schedule()
    m.checked('user_unblock_recheck', 1)
    m.advance(1000)
    assert m.effective() == 2
    m.command(14, record(1, blocked=1) + tail(bytes([1]) + bytes(7)), 1)
    for _ in range(4000):
        m.now += 1
        m.checked('flash_exec_poll', m.now)
        if state(m)[2] == OK:
            break
    else:
        raise AssertionError('TABLE did not reach OK')
    assert state(m)[0] == TABLE and m.effective() == 2
    m.checked('user_storage_poll')
    assert state(m) == (0, 0, 0)
    assert not m.effective() and m.outputs()[1] == 1, 'TABLE recheck before releasing job'
    return {'scene_recheck': 'after take_result, gates open'}


def mutations():
    from test_flash_mutations import run
    path = 'User/log_ring.c'
    cases = {
        'step3-source': ('source_until_verify', [(path,
            '  log_job.after = after;',
            '  log_job.after = after;\n  memset(&log_job.cell, 0, sizeof(log_job.cell));')]),
        'step3-publication': ('publication_after_ok', [(path,
            '    result=w25_async_poll(now);\n    if(result==1) return 1;',
            '    result=w25_async_poll(now);\n    if (result == 1 && log_job.after == LOG_PUBLISH) return log_publish();\n    if(result==1) return 1;')]),
    }
    return run(cases, 'test_flash_executor_step3', 'executor-step3-mutations')


def run():
    return {'source': source_until_verify(), 'publication': publication_after_ok(),
            'TABLE_FW': table_and_fw(), 'PHOTO': photo_persistent(),
            'TABLE_recheck': table_recheck()}
