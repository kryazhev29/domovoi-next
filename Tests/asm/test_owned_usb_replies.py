"""Whole production poll including the real nonblocking USB queue writer."""
import hashlib
import json
from storage_machine import ROOT
from test_service_runtime import ServiceMachine
from test_upload_runtime import UploadMachine, log, wire as upload_wire
from test_idclear_runtime import ClearMachine, wire as clear_wire
from test_photo_runtime import PhotoMachine
from verification_usb_try import init_transport


def transport(m, busy=False):
    m.stubs.pop(m.addr('usb_try_send'), None)
    init_transport(m, inp=0, out=int(busy), epoch=0)


def word(m, name):
    return int.from_bytes(m.u.mem_read(m.addr(name), 4), 'little')


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
    elif kind == 'clear':
        m = ClearMachine(); transport(m, busy); m.parse()
        expected = clear_wire(); active = m.clear_active
    else:
        m = PhotoMachine(); m.boot_media(); data = bytes(range(256)) * 2
        m.capture(data); transport(m, busy); m.packet(21, b''); m.drain()
        m.port(); expected = None
        active = m.active
    return m, active, expected


def run():
    results = {}
    for kind in ('crc_pack', 'crc_list', 'upload', 'clear', 'photo'):
        control, active, expected = setup(kind, False)
        costs = []
        for _ in range(40000):
            if not active(): break
            control.now += 1; control.checked('user_storage_poll')
            costs.append(control.steps)
            assert control.steps < 12000
        else: raise AssertionError(('not complete', kind))
        length = word(control, 'USART_Rx_ptr_in')
        assert length > 0
        packet = bytes(control.u.mem_read(control.addr('USART_Rx_Buffer'), length))
        if expected is not None: assert packet == expected, kind
        else:
            assert length == 534 and packet[20:-2] == bytes(range(256)) * 2
        # Retry exactly the same operation with a full real ring, without new RX.
        m, active, _ = setup(kind, True)
        rejected = []
        for _ in range(len(costs) + 3):
            m.now += 1; m.checked('user_storage_poll'); rejected.append(m.steps)
            assert m.steps < 12000 and word(m, 'USART_Rx_ptr_in') == 0
        assert active(), ('lost owned reply on busy', kind)
        m.write(m.addr('USART_Rx_ptr_out'), 0)
        for _ in range(3):
            m.now += 1; m.checked('user_storage_poll')
            if not active(): break
        assert not active(), ('retry did not complete', kind)
        assert word(m, 'USART_Rx_ptr_in') == length
        assert bytes(m.u.mem_read(m.addr('USART_Rx_Buffer'), length)) == packet
        results[kind] = {'packet_bytes': length, 'polls': len(costs),
                         'maximum_poll_instructions': max(costs),
                         'total_poll_instructions': sum(costs),
                         'busy_last_poll_instructions': rejected[-1],
                         'observed_stack': control.stack_peak}
    return {'results': results, 'status': 'PASS',
            'units': 'Unicorn instructions, real queue writer, NOR boundary modeled; not board time',
            'elf_sha256': hashlib.sha256((ROOT/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest()}


if __name__ == '__main__':
    result = run()
    (ROOT/'Tests/asm/owned_usb_replies_results.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result, indent=2))
