"""Служебные USB-команды 107 и 108 подтверждаются только по USB: сервер их ответов не получает."""
import struct
from storage_machine import *


def run():
    out = []
    for cmd in (107, 108):
        for channel in (0, 1):
            m = StorageMachine(); sent = []
            m.stubs[m.addr('user_send_anyData')] = lambda m=m, sent=sent: (sent.append(tuple(m.args())), m.ret())[-1]
            m.command(cmd, b'', 0, channel, stamp=0)
            assert len(sent) == (1 if channel == 0 else 0), (cmd, channel, sent)
            if sent: assert sent[0][0] == cmd and sent[0][3] == 0, sent
            out.append({'cmd': cmd, 'channel': channel, 'replies': len(sent)})
    # Неизвестная команда получает ответ 11 с кодом 255 по обоим каналам (плата №1, 23.10.2026).
    for channel in (0, 1):
        m = StorageMachine(); m.command(99, b'', 0, channel, stamp=0)
        assert m.answers and m.answers[-1][:2] == (99, 255), (channel, m.answers)
        out.append({'cmd': 99, 'channel': channel, 'answer': 255})
    # Ответы 11 серверу при занятом слоте не теряются: очередь на три уходит по порядку первыми
    # свободными проходами, четвёртый отбрасывается, новый сеанс TCP очередь сбрасывает
    # (плата №1, 23.10.2026: ответ на команду 2 пропал, когда слот занял статус).
    from test_service_runtime import ServiceMachine
    m = ServiceMachine(); m.stubs.pop(m.addr('user_send_answer_comand'), None)
    m.stubs[m.addr('comm_server_online')] = lambda: m.ret(1)
    refused = []
    def acquire():
        m.write(m.args()[0], 1040, 2)
        if refused: refused.pop(); m.ret(0)
        else: m.ret(m.transport)
    m.stubs[m.addr('comm_tx_acquire')] = acquire
    def answer(uniq):
        body = struct.pack('<HBBBBIH', 10, 255, uniq, 11, 255, int.from_bytes(m.u.mem_read(m.device+m.d['datetime'], 4), 'little'),
                           2048 - int.from_bytes(m.u.mem_read(m.addr('server_rx_count'), 2), 'little'))
        return b'SLDN' + body + struct.pack('<H', crc16(body))
    refused[:] = [1, 1, 1, 1]
    for uniq in (7, 8, 9, 10): m.command(99, b'', 0, 1, uniq=uniq, stamp=0)
    assert not m.sent and not refused, m.sent
    for uniq in (7, 8, 9):
        expected = answer(uniq); m.checked('answer_reply_poll')
        assert m.sent[-1] == expected and len(m.sent) == uniq - 6, (uniq, m.sent)
    m.checked('answer_reply_poll'); assert len(m.sent) == 3, 'четвёртый ответ должен быть отброшен'
    refused[:] = [1]; m.command(99, b'', 0, 1, uniq=11, stamp=0); assert len(m.sent) == 3
    m.checked('user_comm_session', 0); m.checked('answer_reply_poll')
    assert len(m.sent) == 3, 'ответ прошлого сеанса не должен уйти'
    out.append({'cmd': 99, 'channel': 1, 'deferred_answers': 3, 'dropped': 1, 'session_reset': True})
    return out


if __name__ == '__main__':
    print(run())
