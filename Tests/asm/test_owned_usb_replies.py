"""Полный рабочий poll с настоящей неблокирующей очередью ответов USB."""
import hashlib
import json
import struct
from storage_machine import ROOT, crc16
from test_service_runtime import ServiceMachine
from test_upload_runtime import UploadMachine, log, wire as upload_wire
from test_idclear_runtime import ClearMachine, wire as clear_wire
from test_photo_runtime import PhotoMachine
from test_fw_service_runtime import FirmwareMachine
from test_usb_source import init_transport


def transport(m, busy=False):
    m.stubs.pop(m.addr('usb_try_send'), None)
    init_transport(m, epoch=0)
    m.usb_wire=[]
    def copy():
        p, _, n, _ = m.args()
        assert 0 < n <= 64
        m.usb_wire.append(bytes(m.u.mem_read(p,n))); m.ret(0)
    m.stubs[m.addr('UserToPMABufferCopy')]=copy
    for name in ('SetEPTxCount','SetEPTxValid'):
        m.stubs[m.addr(name)]=lambda: m.ret(0)
    if busy:
        m.u.mem_write(0x21000f00,b'P')
        m.write(m.addr('usb_tx_length'),1,2)
        m.write(m.addr('usb_tx_source'),0x21000f00)


def tx_length(m):
    p=int.from_bytes(m.u.mem_read(m.addr('usb_tx_source'),4),'little')
    return int.from_bytes(m.u.mem_read(m.addr('usb_tx_length'),2),'little') if p and p!=0x21000f00 else 0


def payload(m, length):
    p=int.from_bytes(m.u.mem_read(m.addr('usb_tx_source'),4),'little')
    return bytes(m.u.mem_read(p,length))


def check_pma(m, packet):
    assert not m.checked('work_acquire',1,11), 'pending source was reused'
    m.checked('Handle_USBAsynchXfer')
    for _ in range(64):
        if not m.checked('USB_Tx_SourceBusy'): break
        m.checked('EP1_IN_Callback')
    else: raise AssertionError('передача USB не завершилась за 64 пакета')
    assert b''.join(m.usb_wire)==packet, 'owner changed before final IN'


def setup(kind, busy):
    if kind.startswith('crc'):
        m = ServiceMachine(8192, 1601536)
        transport(m, busy)
        command = 17 if kind == 'crc_pack' else 18
        expected = m.expected(command)
        m.packet(command)
        active = m.active
    elif kind == 'upload':
        m = UploadMachine(); cells = [log(i + 1) for i in range(32)]
        m.fixture(10, cells); transport(m, busy); m.request()
        expected = upload_wire(10, 7, 0, cells)
        active = lambda: bool(m.ptr())
    elif kind == 'fw_setup':
        m = FirmwareMachine(); m.fixture(); transport(m, busy); m.begin(2, 0, True)
        body = struct.pack('<HBBBBBIH', 11, 255, 7, 11, 19, 0,
                           m.read_word(m.device+47), 2048)
        expected = b'SLDN' + body + struct.pack('<H', crc16(body))
        active = lambda: bool(m.ptr())
    elif kind == 'clear':
        m = ClearMachine(); transport(m, busy); m.parse()
        expected = clear_wire(); active = m.clear_active
    else:
        m = PhotoMachine(); m.boot_media(); data = bytes(range(256)) * 2
        m.capture(data); transport(m, busy); m.packet(21, b''); m.drain()
        m.port(); expected = None
        active = lambda: m.active() or ('tx_length' in m.runtime_fields and m.rget('tx_length',2))
    return m, active, expected


def run(instruction_limit=12000):
    results = {}
    for kind in ('crc_pack', 'crc_list', 'upload', 'clear', 'photo', 'fw_setup'):
        control, active, expected = setup(kind, False)
        costs = []
        for _ in range(40000):
            if not active(): break
            control.now += 1; control.checked('user_storage_poll')
            if kind == 'photo' and control.rget('tx_length',2): control.checked('photo_poll')
            costs.append(control.steps)
            if instruction_limit is not None:
                assert control.steps < instruction_limit, (kind, control.steps)
        else: raise AssertionError(('not complete', kind))
        length = tx_length(control)
        assert length > 0
        packet = payload(control,length)
        if expected is not None: assert packet == expected, kind
        else:
            assert length == 534 and packet[20:-2] == bytes(range(256)) * 2

        m, active, _ = setup(kind, True)
        rejected = []
        for _ in range(len(costs) + 3):
            m.now += 1; m.checked('user_storage_poll'); rejected.append(m.steps)
            if kind == 'photo' and m.rget('tx_length',2): m.checked('photo_poll')
            if instruction_limit is not None:
                assert m.steps < instruction_limit, (kind, m.steps)
            assert tx_length(m) == 0
        assert active(), ('lost owned reply on busy', kind)
        m.checked('USB_Tx_Reset')
        for _ in range(3):
            m.now += 1; m.checked('user_storage_poll')
            if kind == 'photo' and m.rget('tx_length',2): m.checked('photo_poll')
            if not active(): break
        assert not active(), ('retry did not complete', kind)
        assert tx_length(m) == length
        assert payload(m,length) == packet
        check_pma(control,packet); check_pma(m,packet)
        results[kind] = {'packet_bytes': length, 'polls': len(costs),
                         'maximum_poll_instructions': max(costs),
                         'total_poll_instructions': sum(costs),
                         'busy_last_poll_instructions': rejected[-1],
                         'observed_stack': control.stack_peak}
    return {'results': results, 'status': 'PASS',
            'units': 'Unicorn instructions, real queue writer, NOR boundary modeled; not board time',
            'instruction_limit': instruction_limit,
            'elf_sha256': hashlib.sha256(__import__('iar_image').image_paths()[0].read_bytes()).hexdigest()}


if __name__ == '__main__':
    result = run()
    (__import__('iar_image').result_path('owned_usb_replies_results.json')).write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result, indent=2))
