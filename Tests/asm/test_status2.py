"""Статус 2 (пакет 3) на настоящем ELF: входы/выходы, оператор и тип сети по Протокол2 от 27.10.2026."""
import struct

from storage_machine import crc16
from test_service_runtime import ServiceMachine

INFO = 0x21000900  # снимок comm_info_t: csq, creg, cgreg, net, operator_code[7], прочее
HEAD = struct.pack('<HBBBIBHBIB', 30, 255, 42, 3, 0x78563412, 17, 345, 71, 1793023029, 0x32) + bytes(range(1, 9))


def machine(status=0, net=0, operator='', bridge=0):
    m = ServiceMachine()
    for name, value, size in (('csq', 17, 1), ('balance', 345, 2), ('firmwareVersion', 71, 1),
                              ('buildDate', 1793023029, 4), ('bootVersion', 0x32, 1),
                              ('netType', net, 1), ('status', status, 2)):
        m.write(m.device + m.d[name], value, size)
    m.u.mem_write(m.device + m.d['peripheryState'], bytes(range(1, 9)))
    m.u.mem_write(INFO, bytes([17, 1, 1, net]) + operator.encode().ljust(7, b'\0') + bytes(5))
    m.stubs[m.addr('comm_info')] = lambda: m.ret(INFO)
    m.stubs[m.addr('comm_channel_bridge')] = lambda: m.ret(bridge)
    return m


def frame(m, channel):
    count = len(m.sent)
    m.checked('user_send_statusPackSecond', channel, 42)
    assert len(m.sent) == count + 1, m.sent
    sent = m.sent[-1]
    assert sent[:4] == b'SLDN' and len(sent) == 38, (len(sent), sent.hex())
    body = sent[4:-2]
    assert sent[-2:] == struct.pack('<H', crc16(body)), sent.hex()
    assert body[:len(HEAD)] == HEAD, body.hex()
    return tuple(body[len(HEAD):])


def run():
    for channel in (0, 1):
        assert frame(machine(), channel) == (0, 0, 0, 0, 0, 0)
        # Биты статуса 1: 3 вход 1, 2 вход 2, 1 выход 1, 0 выход 2; прочие биты в поля не попадают.
        for status, expect in ((1 << 3, (1, 0, 0, 0)), (1 << 2, (0, 1, 0, 0)), (1 << 1, (0, 0, 1, 0)),
                               (1 << 0, (0, 0, 0, 1)), (0xfff0, (0, 0, 0, 0)), (0xffff, (1, 1, 1, 1))):
            assert frame(machine(status=status), channel)[:4] == expect, (status, expect)
        # Тип сети comm_info 0/1/3/4 -> 0 нет, 1 2G, 2 3G, 3 LTE; сервер через мост -> 4 при любом модеме.
        for net, expect in ((0, 0), (1, 1), (3, 2), (4, 3), (2, 0), (7, 0)):
            assert frame(machine(net=net), channel)[5] == expect, (net, expect)
            assert frame(machine(net=net, bridge=1), channel)[5] == 4, net
        # Оператор по MCC+MNC: 250 01 МТС, 02 МегаФон, 99 Билайн, 20 Теле2, 11 Yota; пусто 0, прочее 255.
        for code, expect in (('', 0), ('25001', 1), ('25002', 2), ('25099', 3), ('25020', 4), ('25011', 5),
                             ('25062', 255), ('26201', 255), ('250011', 255), ('25001x', 255)):
            assert frame(machine(operator=code), channel)[4] == expect, (code, expect)
    # Всё сразу: вход 1 и выход 2 активны, Билайн в LTE через модем.
    assert frame(machine(status=(1 << 3) | 1, net=4, operator='25099'), 1) == (1, 0, 0, 1, 3, 3)
    return {'frame_38_bytes': 'PASS', 'inputs_outputs': 6, 'net_kinds': 12, 'operators': 10}


def mutations():
    from test_flash_mutations import run as mutate
    return mutate({'status2-wifi': ('run', [
        ('User/c/user_protocol.c', 'if (comm_channel_bridge()) return 4;', 'if (0) return 4;'),
    ]), 'status2-operator': ('run', [
        ('User/c/user_protocol.c', 'if (memcmp(code + 3, mnc[i], 2) == 0) return (uint8_t)(i + 1u);',
         'if (memcmp(code + 3, mnc[i], 2) == 0) return 1;'),
    ]), 'status2-outputs': ('run', [
        ('User/c/user_protocol.c', 'b[32] = device.status.stateOut1; b[33] = device.status.stateOut2;',
         'b[32] = device.status.stateOut2; b[33] = device.status.stateOut1;'),
    ]), 'status2-length': ('run', [
        ('User/c/user_protocol.c', 'packet_send(b, 30, (uint8_t)cch);', 'packet_send(b, 24, (uint8_t)cch);'),
    ])}, 'test_status2', 'status2-mutations')
