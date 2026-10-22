"""Настоящие RX/диагностический TX из ELF: ограничение вызовов и независимая проверка кадров."""
import os
import struct
from test_usb_source import SourceMachine
from storage_machine import crc16
from iar_irq_support import UC_ARM_REG_PRIMASK
from iar_layout import fields


def packet(command, uid=73):
    b = struct.pack('<HBBB', 3, 0, uid, command)
    return b'SLDN' + b + struct.pack('<H', crc16(b))


def frames(wire):
    result = []
    while wire:
        assert wire[:4] == b'SLDN'
        size = int.from_bytes(wire[4:6], 'little') + 8
        frame, wire = wire[:size], wire[size:]
        assert len(frame) == size
        assert crc16(frame[4:-2]) == int.from_bytes(frame[-2:], 'little')
        result.append(frame)
    return result


def run(baseline=False):
    costs = {}
    for command in (101, 106, 111, 112, 115):
        if command == 111 and os.environ.get('DOMOVOI_TEST_DIAG') == '0':
            continue
        m = SourceMachine()

        m.u.mem_write(m.bottom, bytes.fromhex('a5a55a5a') * ((m.top - m.bottom) // 4))
        m.write(m.addr('device') + fields('device_s')['userAutorized'], 1, 1)
        m.stubs[m.addr('user_command_ready')] = lambda: m.ret(1)
        m.u.mem_write(m.addr('usb_buffer'), packet(command))
        m.write(m.addr('usb_buffer_count'), 11, 2)
        m.call('usb_check_data', budget=20000000)
        costs[str(command)] = m.steps
        if not baseline:
            assert m.steps < 21600, (command, m.steps)
        wire = frames(m.drain())
        assert [p[8] for p in wire] == ([111, 113] if not baseline and command == 111 else [command]), [p[8] for p in wire]
        if command in (111, 112):
            assert all(p[7] == 73 for p in wire), wire
        if command == 115:  # список сетей без выбранного Wi-Fi-модема: ответ 2 одним байтом
            assert wire[0][9:-2] == b'\x02', wire[0]
        if command == 106:
            assert len(wire[0]) == 523
            assert wire[0][9:-2] == bytes(m.u.mem_read(m.addr('config'), 512))
        assert m.read(m.addr('usb_buffer_count'), 2) == 0


    for mask in (0, 1):
        m = SourceMachine(); m.configure(mask)
        m.stubs[m.addr('user_command_ready')] = lambda: m.ret(1)
        response = bytes((i * 19 + 7) & 255 for i in range(1038))
        m.u.mem_write(0x21000000, response)
        def answer():

            from iar_irq_support import UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_PC
            m.u.reg_write(UC_ARM_REG_R0, 0x21000000)
            m.u.reg_write(UC_ARM_REG_R1, len(response))
            m.u.reg_write(UC_ARM_REG_PC, m.addr('usb_send') | 1)
        m.stubs[m.addr('user_parse_comand')] = answer
        m.u.mem_write(m.addr('usb_buffer'), packet(101))
        m.write(m.addr('usb_buffer_count'), 11, 2)
        m.call('usb_check_data')
        costs['1038_' + str(mask)] = m.steps
        assert m.steps < 21600
        assert m.u.reg_read(UC_ARM_REG_PRIMASK) == mask
        if not baseline:

            m.call('usb_check_data')
            assert m.steps < 500
            assert m.read(m.addr('usb_buffer_count'), 2) == 0
            m.u.mem_write(m.addr('usb_buffer'), packet(101))
            m.write(m.addr('usb_buffer_count'), 11, 2)
            m.call('usb_check_data')
            assert m.steps < 500 and m.read(m.addr('usb_buffer_count'), 2) == 11
        assert m.drain() == response
        assert max(map(len, m.packets)) <= 64
        if not baseline:

            m.packets.clear()
            m.call('usb_check_data')
            assert m.steps < 21600 and m.read(m.addr('usb_buffer_count'), 2) == 0
            assert m.drain() == response
    return costs


if __name__ == '__main__':
    import argparse
    from firmware_suite import select_image
    p = argparse.ArgumentParser()
    p.add_argument('--build-root')
    p.add_argument('--baseline', action='store_true')
    a = p.parse_args()
    select_image(a.build_root)
    print(run(a.baseline))
