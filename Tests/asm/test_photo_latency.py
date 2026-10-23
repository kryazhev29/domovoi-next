"""Ответ 21 при занятом TX: настоящий разборщик и IAR ELF, без оборудования."""
import json
import struct
from test_photo_busy import BusyMachine, saved_photo, wire
from iar_image import result_path


def request(m, body, uniq=6):
    p = wire(21, body, uniq, 30)
    m.write(m.device + m.d['connectionhandle'], 0, 1)
    m.u.mem_write(0x21000000, p)
    m.checked('user_comm_rx', 0x21000000, len(p)); m.checked('user_comm_rx_done', len(p), 0)
    m.checked('server_rx_process')
    m.checked('server_rx_process')


def stale_reply(baseline=False):
    results = []
    for number, offset in ((1791991562, 19164), (12345, 15690)):
        m = BusyMachine(saved_photo()); m.boot_media()
        request(m, struct.pack('<IH', number, offset))
        assert m.active() and m.pget('requestId') == 6
        # Слот занял другой исходящий кадр после приёма 21, до конца скана.
        m.transport_busy = True
        for _ in range(400): m.step()
        assert not m.sent
        m.transport_busy = False
        for _ in range(20): m.step()
        replies = [p for p in m.sent if p[8:10] == bytes((11,1))]
        results.append(dict(cursor=[number, offset], reply_count=len(replies), release_ms=400))
        if baseline:
            assert not replies, 'исходная потеря не воспроизведена'
        else:
            assert len(replies) == 1 and replies[0][7] == 6, 'ответ 21 потерян при занятом TX'
            assert m.flash[0x20000c] == 255 and not m.active()
    return results


def lifecycle():
    results = 0
    for disconnect in (False, True):
        m = BusyMachine(saved_photo()); m.boot_media()
        request(m, struct.pack('<IH', 987, 0)); m.transport_busy = True
        # Повтор старого номера ищется по физическому кольцу до ожидания TX.
        for _ in range(600):
            m.step()
            if m.exec_state()[1] in (15,16):break
        else:raise AssertionError('cursor search did not finish')
        m.step() # Начало прежнего двухсекундного ожидания транспорта.
        if disconnect: m.checked('user_comm_session', 0)
        else: m.now += 2001
        for _ in range(20): m.step()
        m.transport_busy = False
        for _ in range(20): m.step()
        assert not m.sent and not m.active()
        request(m, struct.pack('<IH', 987, 0), uniq=8)
        for _ in range(600):
            m.step()
            if m.sent:break
        assert len(m.sent) == 1 and m.sent[0][7:10] == bytes((8, 11, 1))
        results += 1
    return results


def concurrent():
    from log_ring_fixture import install
    from test_upload_runtime import log
    # Настоящие 9 и 21 одним RX; TX держит кадр до моделируемого SEND OK.
    m = BusyMachine(saved_photo()); m.boot_media(); install(m, [log(1)])
    commit = m.commit
    def hold():
        commit(); m.transport_busy = True
    m.stubs[m.addr('comm_tx_commit')] = hold
    p = wire(9, uniq=5, ttl=30) + wire(21, struct.pack('<IH', 987, 0), 6, 30)
    m.write(m.device + m.d['connectionhandle'], 0, 1)
    m.u.mem_write(0x21000000, p)
    m.checked('user_comm_rx', 0x21000000, len(p)); m.checked('user_comm_rx_done', len(p), 0)
    m.checked('server_rx_process')
    for i in range(1000):
        if i % 100 == 0: m.transport_busy = False
        m.checked('server_rx_process'); m.step()
    assert len([p for p in m.sent if p[7:9] == bytes((5, 10))]) == 1
    assert len([p for p in m.sent if p[7:10] == bytes((6, 11, 1))]) == 1
    # Запрос во время съёмки не перезаписывает контекст камеры.
    m = BusyMachine(); m.boot_media()
    m.stubs[m.addr('user_send_answer_comand')] = m.answer
    m.begin_camera(116)
    m.stubs.pop(m.addr('user_send_answer_comand'))
    request(m, struct.pack('<IH', 0, 0))
    for _ in range(20): m.checked('server_rx_process'); m.step()
    assert any(p[7:10] == bytes((6, 11, 2)) for p in m.sent)
    m.data(b'a'*58); m.data(b'b'*58)
    assert m.events and m.flash[0x20000d:0x20000d+116] == b'a'*58+b'b'*58
    return 2


def mutations():
    from firmware_suite import ROOT
    from test_flash_mutations import run
    source = (ROOT/'User/c/photo_runtime.inc').read_text(encoding='utf-8')
    mutations = {}
    for phase in ('PR_REQUEST_ERROR', 'PR_OFFSET_ERROR'):
        old = 'if (!pj.quiet && photo->channel == CH_SERVER && comm_tx_busy()) {'
        new = f'if (flash_job.step != {phase} && !pj.quiet && photo->channel == CH_SERVER && comm_tx_busy()) {{'
        assert source.count(old) == 1
        mutations[phase] = ('stale_reply', [('User/c/photo_runtime.c',
            '#include "photo_runtime.inc"', source.replace(old, new))])
    return run(mutations, 'test_photo_latency', 'photo-latency-mutations')


def success_wire():
    m = BusyMachine(saved_photo(1)); m.boot_media()
    m.write(m.device + m.d['datetime'], 0x78563412)
    request(m, struct.pack('<IH', 12345, 1))
    for _ in range(2000):
        m.step()
        if m.sent: break
    assert m.sent == [wire(11, struct.pack('<BIH', 0, 0x78563412, 2048), 6, 5)]
    return '21 -> 11: code/time/buffer, complete CRC frame'


def run():
    return dict(success_wire=success_wire(), stale_reply=stale_reply(), lifecycle=lifecycle(), concurrent=concurrent())


if __name__ == '__main__':
    import argparse
    from firmware_suite import select_image
    p = argparse.ArgumentParser(); p.add_argument('--baseline', action='store_true')
    a = p.parse_args(); select_image()
    result = stale_reply(True) if a.baseline else run()
    result_path('photo-latency.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result, indent=2))
