"""Linked firmware: bench packets and customer unknown-command contract."""
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
             'prof_card_mark', 'user_send_usbDiag', 'user_send_commDiag',
             'usb_diag_fill', 'usb_diag_fill_regs', 'usb_diag_fill_errors',
             'usb_diag_fill_addr', 'comm_channel_diag', 'comm_server_diag',
             'comm_recover_diag', 'stack_free_bytes')
    for name in names:
        assert (name in m.symbols) == enabled, name
    for name in ('fault_capture', 'fault_record', 'stack_paint', 'stack_canary_check'):
        assert name in m.symbols, name
    packets = []
    if enabled:
        for code, expected in ((111, [111]), (112, [112, 113])):
            answers, wires = command(m, code, 0, 0)
            from test_stage4_usb import frames
            wires = frames(b''.join(wires))
            assert not answers and [p[8] for p in wires] == expected
            packets.extend(p.hex() for p in wires)
            for packet in wires:
                assert packet[:4] == b'SLDN' and packet[7] == 37
                assert len(packet) == int.from_bytes(packet[4:6], 'little') + 8
                assert crc16(packet[4:-2]) == int.from_bytes(packet[-2:], 'little')
    else:
        for channel in (0, 1):
            for authorized in (0, 1):
                reference, wire = command(m, 250, channel, authorized)
                assert reference and not wire
                for code in (111, 112, 113):
                    answers, wires = command(m, code, channel, authorized)
                    assert not wires and len(answers) == len(reference)
                    assert [a[1:] for a in answers] == [a[1:] for a in reference]
    # Actual linked USB recovery, including the customer ASM register path.
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
        assert m.u.mem_read(m.addr('usb_out_ago'), 2) == b'\xff\xff'
    assert bytes(m.flash) == before
    return {'diagnostics': enabled, 'customer_cases': 12 if not enabled else 0,
            'bench_packets': [111, 112, 113] if enabled else [],
            'bench_hex': packets, 'usb_recovery_primask': [0, 1], 'nor_unchanged': True}
