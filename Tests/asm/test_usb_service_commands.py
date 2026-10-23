"""Служебные USB-команды 107 и 108 подтверждаются только по USB: сервер их ответов не получает."""
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
    return out


if __name__ == '__main__':
    print(run())
