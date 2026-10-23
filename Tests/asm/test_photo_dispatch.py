"""Готовность фото и ограничение ожидания 21: реальный IAR ELF, без платы."""
import struct
from test_photo_busy import BusyMachine, saved_photo, wire
from test_photo_latency import request


def run():
    count = 0
    for start in (10000, 0xffffff00):
        for flag in ('store_save_requested', 'log_pendingCount', 'store_reset_requested'):
            m = BusyMachine(saved_photo()); m.boot_media(); m.now = start
            m.write(m.addr(flag), 1, 1)
            m.write(m.addr('log_timeKnown'), 1, 1)
            request(m, struct.pack('<IH', 0, 0))
            for elapsed in (0, 500, 999):
                m.now = (start + elapsed) & 0xffffffff
                m.checked('server_rx_process')
                assert not m.sent and not m.active(), (flag, elapsed)
            m.now = (start + 1000) & 0xffffffff
            m.checked('server_rx_process')
            assert [p[7:10] for p in m.sent] == [bytes((6, 11, 2))]
            assert m.checked('user_comm_rx_capacity') == 2048
            count += 1
    # Короткое сохранение заканчивается до предела: фото начинается без отказа.
    m = BusyMachine(saved_photo()); m.boot_media()
    m.write(m.addr('store_save_requested'), 1, 1)
    request(m, struct.pack('<IH', 0, 0))
    m.now += 500; m.write(m.addr('store_save_requested'), 0, 1)
    m.checked('server_rx_process')
    assert m.active() and not m.sent
    # Долгое владение W25: команда 8 за фото не теряется и проходит после отказа 21.
    m = BusyMachine(saved_photo()); m.boot_media()
    m.u.mem_write(m.addr('flash_job'), bytes((7, 1, 1))) # FX_TABLE, pending
    packet = wire(21, bytes(6), 6, 30) + wire(8, bytes((1, 1, 1)), 7, 30)
    m.u.mem_write(0x21000000, packet)
    m.checked('user_comm_rx', 0x21000000, len(packet)); m.checked('user_comm_rx_done', len(packet), 0)
    m.checked('server_rx_process')
    m.now += 999; m.checked('server_rx_process'); assert not m.sent
    m.now += 1; m.checked('server_rx_process')
    assert m.sent[0][7:10] == bytes((6, 11, 2))
    m.checked('server_rx_process')
    assert m.checked('user_comm_rx_capacity') == 2048 - len(wire(8, bytes((1, 1, 1)), 7, 30))
    m.u.mem_write(m.addr('flash_job'), bytes(4))
    m.checked('server_rx_process')
    assert m.checked('user_comm_rx_capacity') == 2048
    assert any(p[7:9] == bytes((7, 11)) for p in m.sent)
    # TX не позволяет потерять отказ; срок не продлевается при повторных проверках.
    m = BusyMachine(saved_photo()); m.boot_media()
    m.write(m.addr('store_save_requested'), 1, 1)
    request(m, bytes(6)); m.transport_busy = True
    m.now += 1200; m.checked('server_rx_process'); assert not m.sent
    m.transport_busy = False; m.checked('server_rx_process')
    assert m.sent[0][7:10] == bytes((6, 11, 2))
    # Новая TCP-сессия не наследует срок старого кадра.
    m.checked('user_comm_session', 0); request(m, bytes(6), 8)
    m.now += 999; m.checked('server_rx_process'); assert len(m.sent) == 1
    m.now += 1; m.checked('server_rx_process'); assert len(m.sent) == 2
    # Срок начинается после полного CRC-кадра, не с первого фрагмента.
    m = BusyMachine(saved_photo()); m.boot_media()
    m.write(m.addr('store_save_requested'), 1, 1)
    packet = wire(21, bytes(6), 6, 30)
    for fragment, delay in ((packet[:10], 0), (packet[10:], 5000)):
        m.now += delay; m.u.mem_write(0x21000000, fragment)
        m.checked('user_comm_rx', 0x21000000, len(fragment)); m.checked('user_comm_rx_done', len(fragment), 0)
        m.checked('server_rx_process')
    m.now += 999; m.checked('server_rx_process'); assert not m.sent
    m.now += 1; m.checked('server_rx_process'); assert len(m.sent) == 1
    # Бит фото использует прежний антидребезг статуса в 1 с.
    m = BusyMachine(); m.write(m.addr('status_reported_ms'), 10000, 4)
    assert not m.checked('status_event_due', 0x2000, 10999)
    assert m.checked('status_event_due', 0x2000, 11000)
    assert not m.checked('status_event_due', 0, 11000)
    return dict(timeout=count, short_wait=1, following_8=1, tx=1, session=1, fragmented=1, status=3)
