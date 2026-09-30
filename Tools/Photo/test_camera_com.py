"""Проверки клиента и окна: эмулятор, без реального COM и внешней сети."""
from pathlib import Path
import queue
import tempfile
import threading
import time
import tkinter as tk
import unittest
from unittest.mock import patch

from camera_emulator import C328
from camera_com import C328Client, CameraError, Cancelled, SYNC, FINISH
from camera_com_gui import CameraApp, DEMO, save_jpeg

JPEG = (Path(__file__).parent / 'sample_640x480.jpg').read_bytes()


class Clock:
    def __init__(self):
        self.now = 0

    def __call__(self):
        return self.now

    def sleep(self, seconds):
        self.now += seconds


class Wire:
    def __init__(self, clock, fragment=512, camera=None, change=None, read_delay=.001):
        self.clock = clock
        self.fragment = fragment
        self.camera = camera or C328(JPEG)
        self.change = change or (lambda command, reply: reply)
        self.pending = bytearray()
        self.sent = []
        self.closed = False
        self.read_delay = read_delay

    def write(self, command):
        self.sent.append((self.clock(), command))
        for reply in self.camera.feed(command):
            self.pending.extend(self.change(command, reply))

    def read(self):
        self.clock.sleep(self.read_delay)
        data = bytes(self.pending[:self.fragment])
        del self.pending[:self.fragment]
        return data

    def close(self):
        self.closed = True


class ProtocolTests(unittest.TestCase):
    def client(self, **kwargs):
        clock = Clock()
        wire = Wire(clock, **kwargs)
        client = C328Client(wire, clock=clock, pause=clock.sleep)
        return client, wire

    def test_capture_exact_mk_sequence_and_timing(self):
        client, wire = self.client()
        self.assertEqual(client.capture(), JPEG)
        commands = [data for _, data in wire.sent]
        self.assertEqual(commands[:6], [bytes.fromhex(value) for value in (
            'AA 0D 00 00 00 00', 'AA 0E 0D 00 00 00', 'AA 01 00 07 00 07',
            'AA 06 08 40 00 00', 'AA 05 00 00 00 00', 'AA 04 01 00 00 00')])
        self.assertEqual(commands[-1], FINISH)
        for index, command in enumerate(commands[6:-1]):
            self.assertEqual(command, b'\xaa\x0e\0\0' + index.to_bytes(2, 'little'))
        self.assertEqual(len(commands[6:-1]), (len(JPEG) + 57) // 58)
        for index in range(2, 6):
            self.assertGreaterEqual(wire.sent[index][0] - wire.sent[index-1][0], .049999)
        self.assertLess(wire.sent[7][0] - wire.sent[6][0], .05)

    def test_fragmented_replies_and_two_captures(self):
        client, _ = self.client(fragment=3)
        self.assertEqual(client.capture(), JPEG)
        self.assertEqual(client.capture(), JPEG)

    def test_reverse_sync_order_and_noise(self):
        def change(command, reply):
            return b'noise' + reply[6:] + reply[:6] if command == SYNC else reply
        client, _ = self.client(change=change)
        self.assertEqual(client.capture(), JPEG)

    def test_silent_sync_sends_exactly_30(self):
        client, wire = self.client(camera=C328(JPEG, silent=True))
        with self.assertRaisesRegex(CameraError, '30 запросов'):
            client.capture()
        self.assertEqual([data for _, data in wire.sent], [SYNC] * 30)
        self.assertLess(wire.clock(), 1.7)

    def test_sync_needs_both_ack_and_sync(self):
        client, _ = self.client(change=lambda cmd, reply: reply[:6] if cmd == SYNC else reply)
        with self.assertRaisesRegex(CameraError, '30 запросов'):
            client.synchronize()

    def test_command_retries_exactly_three(self):
        client, wire = self.client(change=lambda cmd, reply: b'' if cmd[1] == 1 else reply)
        with self.assertRaisesRegex(CameraError, '3 запросов'):
            client.capture()
        self.assertEqual(sum(data[1] == 1 for _, data in wire.sent), 3)

    def test_bad_packet_retried_and_recovered(self):
        damaged = [False]
        def change(command, reply):
            if command == b'\xaa\x0e\0\0\0\0' and not damaged[0]:
                damaged[0] = True
                return reply[:-2] + bytes((reply[-2] ^ 1, 0))
            return reply
        client, wire = self.client(change=change)
        self.assertEqual(client.capture(), JPEG)
        self.assertEqual(sum(data == b'\xaa\x0e\0\0\0\0' for _, data in wire.sent), 2)

    def test_persistent_corruption_fails_without_finish(self):
        client, wire = self.client(camera=C328(JPEG, error_packet=0))
        with self.assertRaisesRegex(CameraError, 'Пакет 0'):
            client.capture()
        self.assertNotIn(FINISH, [data for _, data in wire.sent])
        self.assertEqual(sum(data == b'\xaa\x0e\0\0\0\0' for _, data in wire.sent), 3)

    def test_wrong_packet_id_trailer_and_oversized_length(self):
        mutations = [lambda p: b'\x01\0' + p[2:],
                     lambda p: p[:-1] + b'\x01',
                     lambda p: p[:2] + b'\xff\xff' + p[4:]]
        for mutation in mutations:
            with self.subTest(mutation=mutation):
                client, _ = self.client(change=lambda cmd, reply: mutation(reply)
                                       if cmd == b'\xaa\x0e\0\0\0\0' else reply)
                with self.assertRaisesRegex(CameraError, 'Пакет 0'):
                    client.capture()

    def test_nak(self):
        client, _ = self.client(change=lambda cmd, reply: b'\xaa\x0f\x01\0\0\0' if cmd[1] == 1 else reply)
        with self.assertRaisesRegex(CameraError, 'NAK'):
            client.capture()

    def test_invalid_photo_size(self):
        for size in (0, 65536):
            client, _ = self.client(change=lambda cmd, reply: b'\xaa\x0a\x01' + size.to_bytes(3, 'little')
                                   if cmd[1] == 4 else reply)
            with self.assertRaisesRegex(CameraError, '65535'):
                client.capture()

    def test_total_60_seconds_even_with_valid_packets(self):
        client, wire = self.client()
        original = wire.read
        def slow():
            if wire.sent and wire.sent[-1][1][:4] == b'\xaa\x0e\0\0':
                wire.clock.sleep(.1)
            return original()
        wire.read = slow
        with self.assertRaisesRegex(CameraError, '60 секунд'):
            client.capture()
        self.assertNotIn(FINISH, [data for _, data in wire.sent])

    def test_cancel_during_sync(self):
        client, wire = self.client(camera=C328(JPEG, silent=True))
        def cancel():
            client.stop.set()
            return b''
        wire.read = cancel
        with self.assertRaises(Cancelled):
            client.capture()
        self.assertEqual(len(wire.sent), 1)


class WindowTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = tk.Tk()
        self.root.withdraw()
        self.app = CameraApp(self.root, output=Path(self.temp.name))

    def tearDown(self):
        self.app.stop_event.set()
        if self.app.worker is not None:
            self.app.worker.join(3)
            self.assertFalse(self.app.worker.is_alive())
        self.root.after_cancel(self.app.after_id)
        self.root.destroy()
        self.temp.cleanup()

    def pump_until(self, predicate, timeout=8):
        end = time.monotonic() + timeout
        while not predicate() and time.monotonic() < end:
            self.root.update()
            time.sleep(.01)
        self.root.update()
        self.assertTrue(predicate())

    def finished(self):
        return (self.app.worker is not None and not self.app.worker.is_alive()
                and self.app.events.empty())

    def test_window_single_picture_and_preview(self):
        self.app.port.set(DEMO)
        self.app.start('single')
        self.pump_until(self.finished)
        self.assertEqual(self.app.count, 1)
        self.assertEqual(self.app.last_file.read_bytes(), JPEG)
        self.assertIsNotNone(self.app.preview_image)
        self.assertEqual(str(self.app.stop_button['state']), 'disabled')
        self.assertTrue((Path(self.temp.name) / 'camera_com.log').exists())
        self.assertFalse(list(Path(self.temp.name).glob('*.part')))

    def test_cycle_stop_releases_port_and_controls(self):
        clock = Clock()
        wire = Wire(clock)
        self.app.transport_factory = lambda name, baud: wire
        self.app.port.set('COM99')
        self.app.interval.set('.1')
        self.app.start('cycle')
        self.pump_until(lambda: self.app.count >= 2)
        self.app.stop()
        self.pump_until(self.finished)
        self.assertTrue(wire.closed)
        self.assertEqual(str(self.app.start_buttons[0]['state']), 'normal')

    def test_connection_failure_is_visible(self):
        def fail(*args):
            raise OSError('Порт занят')
        self.app.transport_factory = fail
        self.app.port.set('COM99')
        self.app.start('single')
        self.pump_until(self.finished)
        self.assertIn('Порт занят', self.app.status.get())
        self.assertEqual(self.app.count, 0)

    def test_invalid_port_does_not_open_anything(self):
        with patch('camera_com_gui.messagebox.showerror') as alert:
            self.app.port.set('')
            self.app.start('single')
            alert.assert_called_once()
        self.assertIsNone(self.app.worker)

    def test_bad_jpeg_not_saved(self):
        with self.assertRaises(OSError):
            save_jpeg(b'\xff\xd8bad\xff\xd9', Path(self.temp.name), 'bad')
        self.assertFalse(list(Path(self.temp.name).glob('*')))


if __name__ == '__main__':
    unittest.main(verbosity=2)
