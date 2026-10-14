"""Курсор потерянного TCP-окна из C-модели: настоящие команда 21/фото/Flash в IAR ELF."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from test_photo_busy import saved_photo
from test_photo_runtime import PhotoMachine, HEAD, crc16, struct
from iar_image import result_path
import json


def run(cursor):
    size = 18570
    data = bytes(i % 251 for i in range(size))
    m = PhotoMachine(saved_photo(size)); m.boot_media()
    m.packet(21, channel=1); m.drain()
    for _ in range(1000):
        m.port(); m.drain()
        packets = [p for p in m.sent if p[8] == 14 and p[9]]
        if len(packets) >= 12: break
    else: raise AssertionError('first window did not fill')
    assert 0 < cursor < 12*512
    delivered = b''.join(p[20:-2] for p in packets)[:cursor]
    m.checked('user_comm_session', 0); m.drain()
    assert m.flash[0x200000+HEAD-1] == 255 and not m.pget('pending')
    m.sent.clear()
    m.packet(21, struct.pack('<IH', 12345, cursor), channel=1); m.drain()
    assert m.rget('resume_offset', 2) == cursor
    assert m.flash[0x200000+HEAD-1] == 255
    for _ in range(1000):
        m.port(); m.drain()
        packets = [p for p in m.sent if p[8] == 14]
        if packets and not packets[-1][9]: break
    else: raise AssertionError('resumed photo did not finish')
    offset = cursor
    for p in packets:
        assert crc16(p[4:-2]) == int.from_bytes(p[-2:], 'little')
        if not p[9]: continue
        assert int.from_bytes(p[18:20], 'little') == offset
        delivered += p[20:-2]; offset += len(p)-22
    assert delivered == data and offset == size
    m.packet(21, struct.pack('<IH', 12345, size), channel=1); m.drain()
    for _ in range(100): m.port(); m.drain()
    assert m.flash[0x200000:0x201000] == b'\xff'*4096 and not m.rget('cleanup')
    result = dict(cursor=cursor, bytes=len(delivered), identical=True, final_ack=size, erased=True)
    result_path('tx-window-photo-elf.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    print('photo ELF: real command 21, session cancellation, resume and final ACK PASS')


if __name__ == '__main__':
    run(int(sys.argv[1]))
