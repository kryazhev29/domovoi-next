"""UI state tests; synthetic data, no serial ports opened."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import struct
import tempfile
import tkinter as tk
import unittest
from desktop_app import App


class UiTests(unittest.TestCase):
    def setUp(self):
        self.root = tk.Tk(); self.root.withdraw()
        self.app = App(self.root, demo=True)
        self.app.demo = False  # refresh already used only synthetic ports
        self.tmp = tempfile.TemporaryDirectory()
        self.file = Path(self.tmp.name)/'139.14.1726300000.bin'
        self.file.write_bytes(struct.pack('<II',0x20001000,0x08008209)+bytes(512))
    def tearDown(self):
        self.root.destroy(); self.tmp.cleanup()
    def state(self): return str(self.app.update_button['state'])
    def device(self, hw=139, status=0):
        self.app.devices['COM5'] = dict(port='COM5',version=0x31,hw=hw,status=status,error=0)
        self.app.port.set('COM5'); self.app.selected()
    def test_gating_and_invalid_file(self):
        self.assertEqual(self.state(), 'disabled')
        self.app.load_image(self.file); self.assertEqual(self.state(), 'disabled')
        self.device(); self.assertEqual(self.state(), 'normal')
        self.device(hw=11); self.assertEqual(self.state(), 'disabled')
        self.device(status=7); self.assertEqual(self.state(), 'disabled')
        self.device(); self.file.write_bytes(b'bad'); self.app.load_image(self.file)
        self.assertEqual(self.state(), 'disabled')
    def test_accepted_requires_reidentification(self):
        self.app.load_image(self.file); self.device()
        self.app.events.put(('accepted','Проверьте версию на сервере.'))
        self.app.events.put(('done',None)); self.app.drain()
        self.assertEqual(self.state(),'disabled')
        self.assertIn('не подтверждён', self.app.status.get())
    def test_commit_disables_cancellation(self):
        self.app.events.put(('commit','Установка')); self.app.drain()
        self.assertEqual(str(self.app.stop_button['state']), 'disabled')
        self.app.stop(); self.assertFalse(self.app.cancel.is_set())


if __name__ == '__main__': unittest.main()
