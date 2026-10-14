"""Серверный ACK журнала рядом с фото: IAR ELF, NOR и RX без платы."""
from test_photo_busy import BusyMachine, saved_photo
from test_photo_latency import request
from log_ring_fixture import install
from test_upload_runtime import log


def machine():
    m = BusyMachine(saved_photo()); install(m, [log(1), log(2)])
    m.boot_media()
    m.write(m.addr('cfg_boot_done'), 1, 1)
    m.write(m.addr('cfg_cached_active'), 0x1f5000)
    m.server(9, uniq=40)
    for _ in range(500):
        m.step()
        if any(p[7:9] == bytes((40, 10)) for p in m.sent): break
    else: raise AssertionError('журнал не отправлен')
    assert int.from_bytes(m.u.mem_read(m.addr('log_pending32') + 4, 4), 'little')
    assert m.u.mem_read(m.addr('log_pendingCount'), 1) == b'\0'
    return m


def stream(baseline=False):
    m = machine(); m.record_changes = True; request(m, bytes(6), 41)
    frames = []; ack_at = None; config_before_end = False; cursor_published = False
    for i in range(3000):
        m.checked('server_rx_process'); m.step(10)
        chunks = [p for p in m.sent if p[8] == 14 and p[9] == 1]
        while len(frames) < len(chunks): frames.append((m.now, chunks[len(frames)]))
        if len(frames) >= 4 and ack_at is None:
            ack_at = m.now; m.server(9, uniq=42)
        cursor_published |= int.from_bytes(m.u.mem_read(m.addr('log_cursor32') + 4, 4), 'little') == 3
        ended = any(p[8:10] == bytes((14, 0)) for p in m.sent)
        if m.checked('mem_config_result') == 1 and not ended: config_before_end = True
        if ended and any(p[7:9] == bytes((42, 10)) for p in m.sent): break
    else: raise AssertionError('фото/ACK не закончились')
    data = b''.join(p[20:-2] for _, p in frames)
    assert data == bytes(i % 251 for i in range(15689))
    assert cursor_published
    gap = max(b[0]-a[0] for a,b in zip(frames,frames[1:]))
    erases = [a for kind,a,_ in m.changes if kind == "erase" and 0x1d0000 <= a < 0x1e8000]
    assert len(erases) == (48 if baseline else 24), len(erases)
    if baseline: assert config_before_end and gap > 1000
    else: assert not config_before_end and gap < 200
    return dict(max_gap_model_ms=gap, frames=len(frames), log_pending=0,
                config_during_photo=config_before_end, cursor_published=cursor_published, journal_erases=len(erases))


def grace():
    for start in (10000, 0xffffff00):
        m = machine(); m.now = start; m.server(9, uniq=42)
        for dt in (0, 500, 999):
            m.now = (start + dt) & 0xffffffff
            if dt == 500: m.server(9, uniq=42) # Повтор не продлевает окно.
            m.checked('user_storage_poll')
            assert not m.checked('mem_config_result')
        m.now = (start + 1000) & 0xffffffff; m.checked('user_storage_poll')
        assert m.checked('mem_config_result') == 1
    # ACK пришёл раньше 21: короткое окно даёт фото начать передачу.
    m = machine(); m.server(9, uniq=42); m.now += 500; request(m, bytes(6), 41)
    for _ in range(100):
        m.checked('server_rx_process'); m.step()
        if any(p[8:10] == bytes((14, 1)) for p in m.sent): break
    else: raise AssertionError('21 не стартовала внутри окна')
    assert not any(p[7:11] == bytes((41, 11, 21, 2)) for p in m.sent)
    # Разрыв TCP отменяет намерение, не публикуя неподтверждённый курсор.
    m.checked('user_comm_session', 0)
    for _ in range(100): m.step()
    assert int.from_bytes(m.u.mem_read(m.addr('log_cursor32') + 4, 4), 'little') == 0
    assert not m.checked('mem_config_result')
    # TTL сохраняется даже когда ACK ждёт приоритетное фото.
    m = machine(); m.server(9, uniq=42, ttl=1); m.now += 1000
    m.checked('user_storage_poll'); assert not m.checked('mem_config_result')
    # USB не получает серверную задержку в одну секунду.
    m = machine(); m.checked('log_ring_sent', 0, 40, 3)
    m.checked('upload_runtime_request', 10, 0, 42, 30)
    assert m.checked('mem_config_result') == 1
    return 6


def run():
    return dict(stream=stream(), grace=grace())
