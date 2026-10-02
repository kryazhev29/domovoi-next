"""Доработка: Кряжев А., 2026. Синтетический журнал: границы и происхождение RX."""
from pathlib import Path
import hashlib
import tempfile
import unittest
from modem_sim import extract


class Extraction(unittest.TestCase):
    def fixture(self, text):
        raw = b'SLDN\x03\x00\xff\x01' + text.encode('cp1251')
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / 'synthetic.txt'
            path.write_bytes(raw)
            fixture = extract(path)
        self.assertEqual(fixture['sha256'], hashlib.sha256(raw).hexdigest())
        return fixture

    def test_only_adjacent_duplicate(self):
        fixture = self.fixture('>>> GSM: Cmd AT\r<<< GSM: Answer \r\nOK\r\n-----\r\n'
                               '>>U Отладка\r\n>>> GSM: Recv \r\nOK\r\n-----\r\n'
                               '>>> GSM: Cmd AT+CSQ\r<<< GSM: Answer \r\nOK\r\n-----\r\n'
                               '>>> GSM: Recv \r\nRING\r\n-----\r\n'
                               '>>> GSM: Recv \r\nOK\r\n-----\r\n')
        self.assertEqual(fixture['duplicates_removed'], 1)
        self.assertEqual(len(fixture['events']), 6)
        self.assertEqual(fixture['events'][-1], {'rx': '\r\nOK\r\n'})
        self.assertNotIn('Отладка', str(fixture['events']))

    def test_prompt_and_missing_binary(self):
        fixture = self.fixture('>>> GSM: Cmd AT+QISEND=0,48\r<<< GSM: Answer \r\n> -----\r\n'
                               '>>> GSM: Cmd \r<<< GSM: Answer \r\nSEND OK\r\n-----\r\n'
                               '>>> GSM: Recv \r\n+RECEIVE: 0, 16\r\nIPD16:SLDN\x08-----\r\n')
        self.assertEqual(fixture['events'][1], {'rx': '> '})
        self.assertEqual(len(fixture['omitted']), 2)
        self.assertNotIn('SLDN', str(fixture['events']))
        self.assertNotIn('IPD16', str(fixture['events']))


if __name__ == '__main__':
    unittest.main()
