"""Собранная прошивка: диагностические пакеты и неизвестные команды профиля заказчика."""
import os
import struct
from storage_machine import StorageMachine, crc16


def command(m, code, channel, authorized):
    m.answers.clear()
    m.wires.clear()
    m.write(m.device + m.d['userAutorized'], authorized, 1)
    packet = struct.pack('<HBBB', 3, 0, 37, code)
    packet += struct.pack('<H', crc16(packet))
    m.u.mem_write(0x21000000, packet)
    m.checked('user_parse_comand', 0x21000000, channel)
    return list(m.answers), list(m.wires)


def run():
    enabled = os.environ.get('DOMOVOI_COMPARE_CONFIG', 'Release') != 'ReleaseCustomer'
    m = StorageMachine()
    m.wires = []

    def send():
        ptr, size, _, _ = m.args()
        m.wires.append(bytes(m.u.mem_read(ptr, size)))
        m.ret(1)

    m.stubs[m.addr('usb_send')] = send
    before = bytes(m.flash)
    names = ('prof_init', 'prof_add', 'prof_relay_on', 'prof_send_comm',
             'prof_card_mark',
             'usb_diag_fill', 'usb_diag_fill_regs', 'usb_diag_fill_errors',
             'usb_diag_fill_addr', 'comm_channel_diag', 'comm_server_diag',
             'comm_recover_diag', 'comm_sms_diag', 'stack_free_bytes')
    for name in names:
        assert (name in m.symbols) == enabled, name
    for name in ('fault_capture', 'fault_record', 'stack_paint', 'stack_canary_check'):
        assert name in m.symbols, name
    assert 'user_send_commDiag' in m.symbols and 'user_send_usbDiag' in m.symbols
    packets = []
    for authorized in (0, 1):
        answers, wires = command(m, 112, 0, authorized)
        payload = bytes((5, 0, 0, 99)) + bytes(78) + bytes((255, 255, 255, 255, 255))
        expected = struct.pack('<HBBB', 90, 0, 37, 112) + payload
        expected = b'SLDN' + expected + struct.pack('<H', crc16(expected))
        assert not answers and wires == [expected], [p.hex() for p in wires]
        # Серверный 112 не продлевает режим, USB продлевает и до авторизации.
        info = m.addr('info')
        assert bytes(m.u.mem_read(info+24, 4)) == struct.pack('<I', 60000)
        assert bytes(m.u.mem_read(info+31, 1)) == b'\x01'
        m.u.mem_write(info+24, bytes(4))
        m.u.mem_write(info+31, bytes(1))
        lease = bytes(m.u.mem_read(info, 36))
        _, wires = command(m, 112, 1, authorized)
        assert not wires
        assert bytes(m.u.mem_read(info, 36)) == lease
    for code, expected in ((111, [111, 113] if enabled else [111]), (112, [112])):
        answers, wires = command(m, code, 0, 0)
        from test_stage4_usb import frames
        wires = frames(b''.join(wires))
        assert not answers and [p[8] for p in wires] == expected
        packets.extend(p.hex() for p in wires)
        for packet in wires:
            assert packet[:4] == b'SLDN' and packet[7] == 37
            assert len(packet) == int.from_bytes(packet[4:6], 'little') + 8
            assert crc16(packet[4:-2]) == int.from_bytes(packet[-2:], 'little')
            if packet[8] == 111:
                assert len(packet) == 327 and struct.unpack_from('<I', packet, 9)[0] == 8
                assert len(packet[277:305]) == 28  # v[67..73]: SMS
                assert packet[305:-2] == bytes(20)  # v[74..78]: износ
                if not enabled:assert packet[21:305]==bytes(284)
    if not enabled:
        for channel in (0, 1):
            for authorized in (0, 1):
                reference, wire = command(m, 250, channel, authorized)
                assert reference and not wire
                for code in (113,):
                    answers, wires = command(m, code, channel, authorized)
                    assert not wires and len(answers) == len(reference)
                    assert [a[1:] for a in answers] == [a[1:] for a in reference]

    for mask in (0, 1):
        from unicorn.arm_const import UC_ARM_REG_PRIMASK
        m.u.reg_write(UC_ARM_REG_PRIMASK, mask)
        for name in ('usb_hung_armed', 'usb_unconf_armed'):
            m.write(m.addr(name), 1, 1)
        for name in ('usb_hung_ms', 'usb_unconf_ms'):
            m.write(m.addr(name), 123, 2)
        m.checked('usb_request_recovery')
        assert m.u.reg_read(UC_ARM_REG_PRIMASK) == mask
        for name in ('usb_hung_armed', 'usb_unconf_armed', 'usb_enabled'):
            assert m.u.mem_read(m.addr(name), 1) == b'\0', name
        assert m.u.mem_read(m.addr('usb_recovery_pending'), 1) == b'\1'
        assert m.u.mem_read(m.addr('usb_recovery_ms'), 1) == b'\x14'
        if enabled:
            assert m.u.mem_read(m.addr('usb_out_ago'), 2) == b'\xff\xff'
        else:
            assert 'usb_out_ago' not in m.symbols
    assert bytes(m.flash) == before
    return {'diagnostics': enabled, 'customer_cases': 4 if not enabled else 0,
            'bench_packets': [111, 112, 113] if enabled else [111, 112],
            'bench_hex': packets, 'usb_recovery_primask': [0, 1], 'nor_unchanged': True}
