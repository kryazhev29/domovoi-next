import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import struct
import tempfile
import threading
import unittest
from unittest.mock import patch
from boot_tool import crc16, application
from desktop_transport import Link, transfer, identify, Cancelled, InstallUnconfirmed


def reply(command, count=0, hw=139, status=0, version=0x31):
    body = bytes([0x55, command, status, version, hw, 0])+struct.pack('<I', count)
    return b'SLGT'+struct.pack('<H', 10)+body+struct.pack('<H', crc16(body))


class Device:
    def __init__(self, hw=139, fail_commit=False, old=False):
        self.hw, self.fail_commit, self.old = hw, fail_commit, old
        self.buffer = bytearray(); self.commands = []; self.offset = 0
    def __enter__(self): return self
    def __exit__(self, *args): pass
    def reset_input_buffer(self): self.buffer.clear()
    def write(self, data):
        command = data[7]; self.commands.append(command)
        if command == 0x22 and self.fail_commit: raise OSError('disconnect')
        if command == 0x21:
            self.offset = struct.unpack_from('<I', data, 8)[0]+len(data)-14
        self.buffer.extend(reply(command, self.offset, self.hw, version=0x30 if self.old else 0x31))
    def read(self, count):
        value = self.buffer[:3]; del self.buffer[:3]; return bytes(value)


class DesktopTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.image = Path(self.tmp.name)/'139.14.1726300000.bin'
        self.image.write_bytes(struct.pack('<II', 0x20001000, 0x08008209)+bytes(768))
    def tearDown(self): self.tmp.cleanup()
    def test_transfer_and_partial_frames(self):
        device = Device(); events = []
        transfer('MOCK', self.image, lambda k,v: events.append(k), threading.Event(), lambda *a,**kw:device)
        self.assertEqual(device.commands.count(0x22), 1)
        self.assertEqual(events[-1], 'accepted')
        self.assertNotIn('success', events)
        self.assertEqual(device.offset, len(self.image.read_bytes()))
    def test_wrong_hw_and_old_loader_never_erase(self):
        for device in (Device(hw=11), Device(old=True)):
            with self.assertRaises(ValueError):
                transfer('MOCK', self.image, lambda *a:None, threading.Event(), lambda *a,**kw:device)
            self.assertEqual(device.commands, [0])
    def test_cancel_never_commit(self):
        device = Device(); cancel = threading.Event()
        def notify(kind, value):
            if kind == 'progress': cancel.set()
        with self.assertRaises(Cancelled):
            transfer('MOCK', self.image, notify, cancel, lambda *a,**kw:device)
        self.assertNotIn(0x22, device.commands)
    def test_lost_commit_is_uncertain_and_not_retried(self):
        device = Device(fail_commit=True)
        with self.assertRaises(InstallUnconfirmed):
            transfer('MOCK', self.image, lambda *a:None, threading.Event(), lambda *a,**kw:device)
        self.assertEqual(device.commands.count(0x22), 1)
    def test_block_timeout_retries_same_offset(self):
        device = Device(); original = Link.exchange; attempts = []
        def exchange(link, cmd, payload=b'', *args, **kwargs):
            if cmd == 0x21:
                attempts.append(payload)
                if len(attempts) == 1: raise TimeoutError()
            return original(link, cmd, payload, *args, **kwargs)
        with patch.object(Link, 'exchange', exchange):
            transfer('MOCK', self.image, lambda *a:None, threading.Event(), lambda *a,**kw:device)
        self.assertEqual(attempts[0], attempts[1])
    def test_parser_skips_noise_bad_crc_and_stale_ack(self):
        device = Device()
        device.write = lambda data: None
        bad = bytearray(reply(0x21, 256)); bad[-1] ^= 1
        device.buffer.extend(b'noise'+bad+reply(0x21, 128)+reply(0x21, 256))
        value = Link(device).exchange(0x21, expected_count=256)
        self.assertEqual(struct.unpack_from('<I', value, 6)[0], 256)
    def test_identification_no_writes_to_flash(self):
        device = Device()
        info = identify('MOCK', lambda *a,**kw:device)
        self.assertEqual(info['hw'], 139)
        self.assertEqual(device.commands, [0, 0x23])
    def test_file_changed_after_preview_opens_no_port(self):
        snapshot = application(self.image)
        self.image.write_bytes(self.image.read_bytes()+b'1234')
        with self.assertRaisesRegex(ValueError, 'изменился'):
            transfer('MOCK', self.image, lambda *a:None, threading.Event(),
                     lambda *a,**kw:self.fail('Port must not open'), expected_image=snapshot)


if __name__ == '__main__': unittest.main()
