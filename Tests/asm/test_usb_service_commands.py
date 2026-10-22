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
    return out


if __name__ == '__main__':
    print(run())
