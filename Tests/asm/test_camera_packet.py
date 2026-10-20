"""C328: реальные JPEG, длинные кадры, Flash и неизменный поток 14; без платы."""
import hashlib
import json
import struct
import sys
from pathlib import Path
from test_photo_runtime import PhotoMachine, HEAD, fields
from photo_latency_audit import capture
from iar_image import result_path
from iar_irq_support import Machine

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'Tools/Photo'))
from camera_emulator import C328


def uart_packet(size, baud):
    m = Machine(); buffer = 0x21000000; usart = 0x40013800
    m.u.mem_write(buffer, b'\xa5' * (size + 8))
    m.call('uart_init', baud, 0, buffer, size)
    assert m.read(usart + 20) == 0  # USART1: RXNE IRQ, без DMA.
    data = bytes(i & 255 for i in range(size))
    for i, byte in enumerate(data):
        m.write(usart, 0x20); m.write(usart + 4, byte)
        m.call('USART1_IRQHandler')
        if i % 11 == 0:m.call('uart_timer1ms')
    assert m.call('uart_received') == size
    assert m.call('uart_getstate') == 0
    m.call('uart_timer1ms'); assert m.call('uart_getstate') == 0
    m.call('uart_timer1ms'); assert m.call('uart_getstate') == 1
    m.write(usart, 0x20); m.write(usart + 4, 0xee)
    m.call('USART1_IRQHandler')
    assert m.call('uart_received') == size
    assert bytes(m.u.mem_read(buffer, size + 8)) == data + b'\xa5' * 8
    m.call('uart_clear'); assert m.call('uart_received') == 0


def stream(m, number=0, offset=0):
    m.sent.clear()
    m.packet(21, struct.pack('<IH', number, offset), channel=1, uniq=14)
    m.drain()
    assert m.answers[-1][1] == 0
    for _ in range(100):
        m.port(); m.drain()
        if m.sent and not m.sent[-1][9]:
            return b''.join(p[20:-2] for p in m.sent if p[9])
    raise AssertionError('поток 14 не завершён')


def run():
    m = PhotoMachine(); m.boot_media(); m.record_changes = True
    for baud in (115200,57600,38400):
        uart_packet(m.packet_size, baud)
    # Полные JPEG снимаются при мосте в последней строке таблицы; обычная скорость 115200.
    m.u.mem_write(m.addr('config')+fields('config_s')['rs485Periphery']+14,b'\1\1')
    photos = []
    model = []
    for number in (1791995602, 1791995675):
        data = (ROOT / f'Tests/fixtures/camera/{number}.jpg').read_bytes()
        photos.append(data)
        m.write(m.device + m.d['datetime'], number)
        m.begin_camera(len(data))
        assert m.uart_calls[-1][0] == 57600
        address = m.pget('address', 4)
        camera = C328(data)
        for command in m.commands:
            camera.feed(command)
        assert camera.packet_size == m.packet_size
        for off in range(0, len(data), m.packet_size - 6):
            m.next_request()
            replies = camera.feed(m.commands[-1])
            assert len(replies) == 1
            frame = replies[0]
            assert frame[4:-2] == data[off:off + m.packet_size - 6]
            # Неверные checksum и нулевой хвост не должны запускать запись.
            for index in (-2, -1):
                bad = bytearray(frame); bad[index] ^= 1
                m.feed(bad)
                assert not m.active() and m.pget('written', 2) == off
            # Заголовок и длинный хвост приходят раздельно, с тишиной между ними.
            for part in (frame[:3], frame[3:4], frame[4:]):
                m.feed(part)
            assert m.active()
            m.drain()
            assert m.pget('written', 2) == min(off + m.packet_size - 6, len(data))
        assert m.events[-1] == number
        assert m.flash[address + HEAD:address + HEAD + len(data)] == data
        for baud in (115200,57600,38400):
            for delay in (0, 60):
                model.append(capture(data, baud, delay))
    assert stream(m) == b''.join(photos)
    raw = b''.join(m.sent)
    result_path('camera-packet.stream').write_bytes(raw)
    # Сравниваем весь занятый диапазон Flash, включая заголовки и хвосты секторов.
    media = bytes(m.flash[0x200000:0x208000])
    result_path('camera-packet.flash').write_bytes(media)
    m.checked('photo_connection_lost')
    for offset in (1, 249, 250, 505, 506, 511, 512, 2560, len(photos[0])-1):
        expected = photos[0][offset:] + photos[1]
        assert stream(m, 1791995602, offset) == expected
        assert stream(m, 1791995602, offset) == expected
    writes = [(a, len(b)) for kind, a, b in m.changes if kind == 'write']
    assert all(0 < n <= 64 and (a & 255) + n <= 256 for a, n in writes)
    result = dict(packet_size=m.packet_size, sizes=list(map(len, photos)),
                  flash_sha256=hashlib.sha256(media).hexdigest(),
                  stream_sha256=hashlib.sha256(raw).hexdigest(),
                  page_bounded_writes=len(writes), model=model)
    result_path('camera-packet.json').write_text(json.dumps(result, indent=2)+'\n')
    return result


if __name__ == '__main__':
    from firmware_suite import select_image
    select_image()
    print(json.dumps(run(), indent=2))
