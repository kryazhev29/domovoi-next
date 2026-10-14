"""SLDN 109 и точки съёма в настоящем IAR ELF; периферия моделируется."""
import json
import os
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent / 'asm'))
from test_usb_source import SourceMachine, init_transport
from storage_machine import StorageMachine, crc16
from iar_layout import fields
from iar_irq_support import ELFFile, DWARFExprParser, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2
from iar_image import image_paths, result_path


def offset(variable, *path):
    """Смещения вложенного состояния из DWARF, без копии структуры C."""
    with image_paths()[0].open('rb') as file:
        dwarf = ELFFile(file).get_dwarf_info()
        for cu in dwarf.iter_CUs():
            for die in cu.iter_DIEs():
                attr = die.attributes.get('DW_AT_name')
                if die.tag != 'DW_TAG_variable' or not attr or attr.value != variable.encode():
                    continue
                total = 0
                for name in path:
                    die = die.get_DIE_from_attribute('DW_AT_type')
                    while die.tag in ('DW_TAG_typedef', 'DW_TAG_const_type', 'DW_TAG_volatile_type'):
                        die = die.get_DIE_from_attribute('DW_AT_type')
                    die = next(m for m in die.iter_children() if m.attributes.get('DW_AT_name') and m.attributes['DW_AT_name'].value == name.encode())
                    value = die.attributes['DW_AT_data_member_location'].value
                    total += value if isinstance(value, int) else DWARFExprParser(cu.structs).parse_expr(value)[0].args[0]
                return total
    raise AssertionError(variable)


def machine(bits=7, ack=False):
    m = SourceMachine()
    m.call('usb_log_enable', bits)
    m.stubs[m.addr('timers_getMillis')] = lambda: m.ret(15903)
    if ack:
        def acknowledge():
            m.write(m.addr('USB_Tx_State'), 0, 1)
            m.ret()
        m.stubs[m.addr('SetEPTxValid')] = acknowledge
    return m


def wire(kind, body, lost=0, now=15903):
    payload = bytes((255, 0, 109, kind)) + struct.pack('<IH', now, lost) + body
    tail = struct.pack('<H', len(payload)) + payload
    return b'SLDN' + tail + struct.pack('<H', crc16(tail))


def send(m, kind=1, body=b'AT+CEREG?'):
    m.u.mem_write(0x21000000, body)
    m.call('usb_log', kind, 0x21000000, len(body))


def enabled_at_stream():
    # Поведенческая регрессия для старой базы: 108 уже была, потока 109 не было.
    m = SourceMachine()
    m.stubs[m.addr('user_command_ready')] = lambda: m.ret(1)
    m.stubs[m.addr('timers_getMillis')] = lambda: m.ret(15903)
    m.write(m.addr('device') + fields('device_s')['userAutorized'], 1, 1)
    m.u.mem_write(0x21000000, struct.pack('<HBBBB', 4, 255, 9, 108, 1))
    m.call('user_parse_comand', 0x21000000, 0)
    assert m.drain()[8] == 108
    m.packets.clear()
    m.stubs[0x21000f00] = m.ret
    m.u.mem_write(0x21000800, struct.pack('<II', 0x21000f01, 0))
    m.call('comm_at_init', 0x21000800, 1000)
    m.stubs[m.addr('comm_uart_peek')] = lambda: m.ret(0)
    m.stubs[m.addr('comm_uart_tx')] = lambda: m.ret(1)
    m.u.mem_write(0x21000000, b'AT+CSQ')
    m.u.mem_write(0x21000820, struct.pack('<IHBBI', 0x21000000, 6, 0, 0, 500))
    assert m.call('comm_at_start', 0x21000820, 10) == 0
    assert m.packets == [wire(1, b'AT+CSQ')], '108 включена, но команда AT не дала кадр 109'


def transport():
    m = machine()
    send(m)
    assert m.packets == [wire(1, b'AT+CEREG?')]
    assert not m.owned() and not m.busy(), 'журнал занял источник/память ответа'
    # Самый большой допустимый источник 1039 байт принимается во время журнала.
    answer = bytes((i * 17) & 255 for i in range(1039))
    m.u.mem_write(0x21000100, answer)
    assert m.call('usb_send', 0x21000100, len(answer))
    send(m)
    assert m.read(m.addr('lost'), 2) == 1
    assert m.drain() == wire(1, b'AT+CEREG?') + answer
    send(m)
    assert m.packets[-1] == wire(1, b'AT+CEREG?', lost=1)
    assert not m.read(m.addr('lost'), 2)
    m.call('EP1_IN_Callback')
    send(m, body=b'A' * 96)
    assert m.packets[-1] == wire(129, b'A' * 46)
    assert m.top - m.minimum <= 128
    return {'log_stack': m.top - m.minimum, 'instructions': m.steps,
            'cpu_upper_us_72MHz_8cycles': round(m.steps * 8 / 72, 1)}


def masks_and_loss():
    m = machine(0)
    for kind in (1, 16, 32):
        send(m, kind)
    assert not m.packets and not m.read(m.addr('lost'), 2)
    log_bits = m.addr('device') + fields('device_s')['logBits']
    for mask in (1, 2, 4, 7, 255):
        m.call('usb_log_enable', mask)
        assert m.read(log_bits, 1) == (mask & 7)
        for kind, bit in ((1, 1), (16, 2), (32, 4)):
            before = len(m.packets)
            send(m, kind)
            assert len(m.packets) - before == bool(mask & bit)
            m.call('EP1_IN_Callback')
    m.write(m.addr('bDeviceState'), 0)
    m.write(m.addr('lost'), 65534, 2)
    send(m)
    send(m)
    assert m.read(m.addr('lost'), 2) == 65535
    m.write(m.addr('bDeviceState'), 5)
    send(m)
    assert m.packets[-1] == wire(1, b'AT+CEREG?', lost=65535)
    assert m.read(m.addr('lost'), 2) == 0
    m.write(m.addr('usb_bus_resets'), 8)
    m.call('usb_log_session')
    assert not m.read(log_bits, 1)
    m.call('usb_log_enable', 7)
    m.write(0x40010c08, 0)
    m.call('usb_log_session')
    assert not m.read(log_bits, 1)
    # Настоящий case 108, пустой ACK с uniq 9, маска и сброс потерь.
    m = machine(0)
    m.write(m.addr('device') + fields('device_s')['userAutorized'], 1, 1)
    m.write(m.addr('lost'), 31, 2)
    m.stubs[m.addr('user_command_ready')] = lambda: m.ret(1)
    packet = struct.pack('<HBBBB', 4, 255, 9, 108, 255)
    m.u.mem_write(0x21000000, packet)
    m.call('user_parse_comand', 0x21000000, 0)
    assert m.drain() == b'SLDN' + struct.pack('<HBBB', 3, 255, 9, 108) + struct.pack('<H', crc16(struct.pack('<HBBB', 3, 255, 9, 108)))
    assert m.read(m.addr('device') + fields('device_s')['logBits'], 1) == 7
    assert m.read(m.addr('lost'), 2) == 0
    # Горячая смена категории не стирает потери уже активного журнала.
    m = machine(1)
    m.write(m.addr('lost'), 17, 2)
    m.call('usb_log_enable', 2)
    body = struct.pack('<BBIB', 1, 0, 6300, 0)
    send(m, 16, body)
    assert m.packets == [wire(16, body, lost=17)]
    assert m.read(m.addr('lost'), 2) == 0


def at_hooks():
    m = machine(1, ack=True)
    handler = 0x21000f00
    m.stubs[handler] = m.ret
    m.u.mem_write(0x21000800, struct.pack('<II', handler | 1, 0))
    m.call('comm_at_init', 0x21000800, 1000)
    m.stubs[m.addr('comm_uart_peek')] = lambda: m.ret(0)
    m.stubs[m.addr('comm_uart_tx')] = lambda: m.ret(1)
    m.u.mem_write(0x21000000, b'AT+CSQ')
    m.u.mem_write(0x21000820, struct.pack('<IHBBI', 0x21000000, 6, 0, 0, 500))
    assert m.call('comm_at_start', 0x21000820, 10) == 0
    assert m.packets[-1] == wire(1, b'AT+CSQ')
    for kind, body, detail, expected in ((0, b'+CSQ: 24,99', 0, 2), (1, b'RING', 0, 3)):
        m.u.mem_write(0x21000000, body)
        m.call('event', kind, 0x21000000, len(body), detail)
        assert m.packets[-1] == wire(expected, body)
    m.call('event', 2, 0, 10, 2)
    assert m.packets[-1] == wire(4, b'\x02\x0a\x00')
    m.call('event', 3, 0, 0, 4)
    assert m.packets[-1] == wire(4, b'\x04\0\0')
    # Payload передаёт только направление и длину.
    m.call('comm_at_init', 0x21000800, 1000)
    m.write(m.top, 10)  # Пятый аргумент ARM на стеке вызывающего.
    assert m.call('comm_at_payload', 0x21000000, 128, 3, 500) == 0
    assert m.packets[-1] == wire(5, b'\x01\x80\0')

    # Настоящий poll: двоичное тело QIRD и длинное PDU не становятся строками.
    def receive(data):
        position = [0]
        m.u.mem_write(0x21000000, data)
        def peek():
            m.write(m.u.reg_read(UC_ARM_REG_R0), 0x21000000 + position[0])
            m.ret(len(data) - position[0])
        def consume():
            position[0] += m.u.reg_read(UC_ARM_REG_R0)
            m.ret(1)
        m.stubs[m.addr('comm_uart_peek')] = peek
        m.stubs[m.addr('comm_uart_consume')] = consume
        m.stubs[m.addr('comm_uart_errors')] = lambda: m.ret(0)
        m.stubs[m.addr('comm_uart_tx_done')] = lambda: m.ret(1)
        for now in range(20, 40):
            m.call('comm_at_poll', now)
            if position[0] == len(data):
                m.call('comm_at_poll', now + 1)
                break
        assert position[0] == len(data)
    def start(text, mode):
        m.u.mem_write(0x21000840, text)
        m.u.mem_write(0x21000820, struct.pack('<IHBBI', 0x21000840, len(text), mode, 0, 500))
        m.call('comm_at_init', 0x21000800, 1000)
        m.stubs[m.addr('comm_uart_peek')] = lambda: m.ret(0)
        assert m.call('comm_at_start', 0x21000820, 10) == 0
    m.stubs[0x21000f10] = lambda: m.ret(m.u.reg_read(UC_ARM_REG_R1))
    m.write(0x21000804, 0x21000f11)
    start(b'AT+QIRD=0,3', 2)
    receive(b'+QIRD: 3\r\nXYZ\r\nOK\r\n')
    assert wire(5, b'\0\x03\0') in m.packets
    before = len(m.packets)
    start(b'AT+CMGR=1', 4)
    receive(b'+CMGR: 0,,160\r\n' + b'FA' * 160 + b'\r\nOK\r\n')
    assert all(b'FAFAFA' not in packet[16:-2] for packet in m.packets[before:])

    # Ресинхронизация: CMD появляется только после принятия служебной команды UART.
    start(b'AT', 0)
    before = len(m.packets)
    m.stubs[m.addr('comm_uart_tx')] = lambda: m.ret(0)
    m.call('comm_at_poll', 520)
    assert all(packet[16:-2] != b'AT+CMEE?' for packet in m.packets[before:])
    m.stubs[m.addr('comm_uart_tx')] = lambda: m.ret(1)
    m.call('comm_at_poll', 521)
    assert m.packets[-1] == wire(1, b'AT+CMEE?')




def gsm_and_events():
    m = machine(2, ack=True)
    m.write(m.addr('slm') + offset('slm', 'attempt', 'now'), 6300)
    m.call('comm_modem_record', 5, 0, 0)  # POWER -> SETUP
    assert m.packets[-1] == wire(16, struct.pack('<BBIB', 1, 0, 6300, 0))
    m.call('rc_schedule', 1, 60000)
    assert m.packets[-1] == wire(18, b'\x01' + struct.pack('<I', 60000))
    m.stubs[0x21000f00] = lambda: m.ret(1)
    m.write(m.addr('sv_link'), 0x21000f01)
    m.call('server_link', 0, 1)
    assert m.packets[-1] == wire(17, b'\0' + struct.pack('<I', 1))
    # Общий commit журнала используется синхронным и асинхронным добавлением.
    from test_stage5_log import boot, append
    from log_ring_fixture import install, BASE
    from test_upload_runtime import log
    m = StorageMachine()
    install(m, [])
    boot(m)
    init_transport(m)
    packets = []
    def pma():
        p, _, n, _ = m.args()
        packets.append(bytes(m.u.mem_read(p, n)))
        m.ret()
    m.stubs[m.addr('UserToPMABufferCopy')] = pma
    for name in ('SetEPTxCount', 'SetEPTxValid'):
        m.stubs[m.addr(name)] = m.ret
    m.call('usb_log_enable', 4)
    append(m, 1)
    assert len(packets) == 1 and packets[0][9] == 32
    body = bytes(m.flash[BASE + 32:BASE + 64])
    assert packets[0] == wire(32, body, now=m.now)


def run():
    result = {}
    for test in (enabled_at_stream, transport, masks_and_loss, at_hooks, gsm_and_events):
        result[test.__name__] = test() or 'PASS'
    return result


if __name__ == '__main__':
    result = run()
    result_path('usb-log.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('PASS USB log ELF:', result)
