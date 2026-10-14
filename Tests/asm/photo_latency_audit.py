"""Офлайн-архив и чувствительность времени съёмки; не аппаратное измерение."""
import argparse
import hashlib
import json
import math
import struct
import zipfile
from pathlib import Path
from test_photo_runtime import PhotoMachine, HEAD
from firmware_suite import select_image
from iar_image import result_path


class TimedCamera(PhotoMachine):
    # Параметры модели: NOR program 1 мс, erase 40 мс, проход 1 мс.
    # Проверка стёртого сектора, как в драйвере, занимает 64 прохода по 64 Б.
    def __init__(self, baud, turnaround):
        super().__init__()
        self.baud = baud; self.turnaround = turnaround
        self.serial = 0; self.io_times = {'erase': 0, 'write': 0}
        self.operations = {'erase': 0, 'write': 0}
    def uart_send(self):
        n = self.args()[1]
        super().uart_send()
        dt = math.ceil(n*10000/self.baud)
        self.now += dt; self.serial += dt
    def start(self):
        super().start()
        self.io_start = self.now
        self.io_kind = 'write' if self.io[1] else 'erase'
        self.due = self.now + (1 if self.io[1] else 40)
        self.verify_left = 1 if self.io[1] else 64
        self.operations[self.io_kind] += 1
    def poll(self):
        self.io_budget += 1
        if self.now < self.due: self.ret(1); return
        self.verify_left -= 1
        if self.verify_left: self.ret(1); return
        a, p, n, data, _ = self.io
        if p: assert bytes(self.u.mem_read(p, n)) == data
        assert self.flash[a:a+len(data)] == data
        self.io_times[self.io_kind] += self.now-self.io_start
        self.io = None; self.ret(2)


def capture(data, baud, turnaround):
    m = TimedCamera(baud, turnaround); m.boot_media()
    start = m.now; m.begin_camera(len(data)); startup = m.now-start
    wire = quiet = camera = 0
    for off in range(0, len(data), m.packet_size-6):
        payload = data[off:off+m.packet_size-6]
        m.next_request()
        dt = math.ceil((len(payload)+6)*10000/baud)
        wire += dt; quiet += 2; camera += turnaround
        m.now += dt+2+turnaround
        # data() использует реальный автомат, включая границы NOR-страниц.
        m.data(payload)
    assert m.flash[0x200000+HEAD:0x200000+HEAD+len(data)] == data
    total = m.now-start
    return dict(bytes=len(data), packet_size=m.packet_size, packets=math.ceil(len(data)/(m.packet_size-6)), baud=baud,
                assumptions=dict(camera_turnaround_ms=turnaround, erase_busy_ms=40,
                                 program_busy_ms=1, poll_ms=1, uart_quiet_ms=2),
                total_ms=total, startup_including_first_erase_ms=startup,
                jpeg_wire_ms=wire, command_wire_ms=m.serial,
                camera_wait_ms=camera, quiet_ms=quiet,
                nor_ms=m.io_times, operations=m.operations,
                sha256=hashlib.sha256(data).hexdigest())


def audit(path):
    with zipfile.ZipFile(path) as z:
        rows = [json.loads(x) for x in z.read('traffic/867442070217472_2026-10-14.jsonl').splitlines()]
        photos = {n: z.read(f'photos/867442070217472_{n}.jpg') for n in (1791995602, 1791995675)}
        recovered = {n: bytearray(len(b)) for n, b in photos.items()}
        covered = {n: set() for n in photos}; counts = {n: 0 for n in photos}
        for r in rows:
            if r.get('kind') != 'decoded' or r.get('command') != 14: continue
            b = bytes.fromhex(r['payload'])
            if not b or not b[0]: continue
            number, size, off = struct.unpack_from('<IIH', b, 1)
            if number not in photos: continue
            data = b[11:]; assert size == len(photos[number]) and off+len(data) <= size
            recovered[number][off:off+len(data)] = data
            covered[number].update(range(off, off+len(data))); counts[number] += 1
        for n in photos:
            assert len(covered[n]) == len(photos[n]) and recovered[n] == photos[n]
        events = [r for r in rows if r.get('kind') in ('decoded', 'frame')
                  and '2026-10-14T16:33:40' <= r['utc'] <= '2026-10-14T16:35:00'
                  and r.get('command') in (9, 10, 11, 21)]
        timing = [capture(data, baud, wait) for data in photos.values()
                  for baud, wait in ((115200, 0), (115200, 60), (9600, 0))]
        m = PhotoMachine(); m.boot_media()
        for number, data in photos.items():
            m.write(m.device+m.d['datetime'], number)
            _, stamp = m.capture(data); assert stamp == number
        def stream(number, offset):
            m.sent.clear(); m.packet(21, struct.pack('<IH', number, offset), channel=1, uniq=14)
            m.drain(); assert m.answers[-1][1] == 0
            for _ in range(100):
                m.port(); m.drain()
                if m.sent and not m.sent[-1][9]: break
            else: raise AssertionError('поток не завершён')
            return b''.join(p[20:-2] for p in m.sent if p[9])
        assert stream(0, 0) == b''.join(photos.values())
        result_path('two-photos.stream').write_bytes(b''.join(m.sent))
        # Разрыв и повтор частичного курсора не теряют и не меняют байты.
        m.checked('photo_connection_lost')
        expected = photos[1791995602][2560:]+photos[1791995675]
        assert stream(1791995602, 2560) == expected
        assert stream(1791995602, 2560) == expected
        result = dict(archive=str(path), reconstructed_frames=counts,
                      two_photos_and_resume_bytes=True,
                      model_only=timing, command_timeline=events)
        result_path('archive-camera-audit.json').write_text(json.dumps(result, indent=2)+'\n')
        print(json.dumps(dict(reconstructed_frames=counts, model_only=timing), indent=2))


if __name__ == '__main__':
    p = argparse.ArgumentParser(); p.add_argument('archive', type=Path)
    a = p.parse_args(); select_image(); audit(a.archive)
