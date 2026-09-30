"""Опрос C328/OV528: последовательность и ограничения User/photo.c."""
import threading
import time

BAUD_RATES = (115200, 57600, 38400, 19200, 9600)
PACKET_SIZE = 64
PHOTO_LIMIT = 65535
SYNC = bytes.fromhex('AA 0D 00 00 00 00')
SYNC_ACK = bytes.fromhex('AA 0E 0D 00 00 00')
FINISH = bytes.fromhex('AA 0E 00 00 F0 F0')


class CameraError(Exception):
    pass


class Cancelled(CameraError):
    pass


class C328Client:
    """Транспорт: read() с ограниченным ожиданием, write(bytes).

    8N1, 640x480, 58 байт данных в пакете; 50 мс/30 запросов SYNC,
    2 с/3 запроса остальных стадий, общий срок 60 с. Никаких сбросов
    камеры, смены её скорости или команд контроллеру.
    """
    def __init__(self, port, stop=None, emit=None, clock=time.monotonic, pause=None):
        self.port = port
        self.stop = stop if stop is not None else threading.Event()
        self.emit = emit or (lambda *args: None)
        self.clock = clock
        self.pause = pause or self.stop.wait
        self.buffer = bytearray()
        self.last_send = -float('inf')
        self.deadline = float('inf')

    def _check(self):
        if self.stop.is_set():
            raise Cancelled('Остановлено пользователем')
        if self.clock() >= self.deadline:
            raise CameraError('Превышен общий срок съёмки: 60 секунд')

    def _wait(self, seconds):
        self._check()
        self.pause(max(0, min(seconds, self.deadline - self.clock())))
        self._check()

    def _send(self, data, spaced=False):
        self._check()
        if spaced:
            self._wait(max(0, self.last_send + .05 - self.clock()))
        self.port.write(data)
        self.last_send = self.clock()
        self.emit('tx', data)

    def _receive(self):
        self._check()
        data = self.port.read()
        self._check()
        if data:
            self.emit('rx', data)
            self.buffer.extend(data)
        else:
            self._wait(.002)

    def _control_frame(self):
        # USB может склеить ACK+SYNC или разделить команду на несколько чтений.
        while self.buffer and self.buffer[0] != 0xAA:
            del self.buffer[0]
        if len(self.buffer) < 6:
            return None
        frame = bytes(self.buffer[:6])
        del self.buffer[:6]
        if frame[1] == 0x0F:
            raise CameraError(f'Камера ответила NAK: {frame.hex(" ").upper()}')
        return frame

    def _controls_until(self, end, accept):
        while self.clock() < end:
            self._check()
            frame = self._control_frame()
            if frame is not None:
                if accept(frame):
                    return frame
            else:
                self._receive()
        return None

    def _synchronize(self):
        self.emit('stage', 'Синхронизация C328')
        seen = set()

        def accept(frame):
            if frame[1:3] == b'\x0e\x0d':
                seen.add('ack')
            if frame[1] == 0x0D:
                seen.add('sync')
            return len(seen) == 2

        for attempt in range(30):
            self.buffer.clear()
            self._send(SYNC, spaced=True)
            if self._controls_until(self.last_send + .05, accept):
                self._send(SYNC_ACK)
                self.emit('stage', 'Камера ответила: синхронизация успешна')
                return
        raise CameraError('Нет ответа C328 после 30 запросов SYNC. Проверьте порт, скорость, TX/RX и питание.')

    def synchronize(self):
        self.deadline = self.clock() + 60
        self.buffer.clear()
        self._synchronize()

    def _command(self, command, title, accept=None):
        self.emit('stage', title)
        if accept is None:
            accept = lambda frame: frame[1:3] == bytes((0x0E, command[1]))
        for attempt in range(3):
            self.buffer.clear()
            self._send(command, spaced=True)
            result = self._controls_until(self.last_send + 2, accept)
            if result is not None:
                return result
            self.emit('info', f'{title}: нет ответа, попытка {attempt + 1}/3')
        raise CameraError(f'{title}: нет ответа после 3 запросов')

    def _packet(self, number, remaining):
        request = bytes((0xAA, 0x0E, 0, 0, number & 255, number >> 8))
        for attempt in range(3):
            self.buffer.clear()
            self._send(request)
            end = self.last_send + 2
            while self.clock() < end:
                self._check()
                if len(self.buffer) >= 6 and self.buffer[:2] == b'\xaa\x0f':
                    raise CameraError(f'Камера ответила NAK при чтении пакета {number}')
                if len(self.buffer) >= 4:
                    packet_id = int.from_bytes(self.buffer[:2], 'little')
                    length = int.from_bytes(self.buffer[2:4], 'little')
                    if not 1 <= length <= PACKET_SIZE - 6:
                        self.buffer.clear()
                        self.emit('info', f'Пакет {number}: неверная длина; ожидаем повтор')
                    elif len(self.buffer) >= length + 6:
                        frame = bytes(self.buffer[:length + 6])
                        del self.buffer[:length + 6]
                        if (packet_id == number and length <= remaining and
                                frame[-1] == 0 and frame[-2] == sum(frame[:-2]) & 255):
                            return frame[4:-2]
                        self.emit('info', f'Пакет {number}: отвергнут ID/длина/контрольная сумма')
                        continue
                self._receive()
            self.emit('info', f'Пакет {number}: нет верного ответа, попытка {attempt + 1}/3')
        raise CameraError(f'Пакет {number}: нет верного ответа после 3 запросов')

    def capture(self):
        self.deadline = self.clock() + 60
        self.buffer.clear()
        self._synchronize()
        self._command(bytes.fromhex('AA 01 00 07 00 07'), 'Настройка JPEG 640×480')
        self._command(bytes.fromhex('AA 06 08 40 00 00'), 'Размер пакета: 64 байта')
        self._command(bytes.fromhex('AA 05 00 00 00 00'), 'Снимок')
        header = self._command(bytes.fromhex('AA 04 01 00 00 00'), 'Запрос JPEG',
                               lambda frame: frame[1:3] == b'\x0a\x01')
        size = int.from_bytes(header[3:6], 'little')
        if not 1 <= size <= PHOTO_LIMIT:
            raise CameraError(f'Размер JPEG {size}: МК допускает 1…65535 байт')
        self.emit('stage', f'Получение JPEG: {size} байт')
        jpeg = bytearray()
        number = 0
        percent = -1
        while len(jpeg) < size:
            jpeg.extend(self._packet(number, size - len(jpeg)))
            number += 1
            current = len(jpeg) * 100 // size
            if current != percent:
                self.emit('progress', (len(jpeg), size))
                percent = current
        self._send(FINISH)
        if not jpeg.startswith(b'\xff\xd8') or not jpeg.endswith(b'\xff\xd9'):
            raise CameraError('Данные получены, но отсутствуют границы JPEG FFD8…FFD9; файл не сохранён')
        return bytes(jpeg)


class DemoPort:
    """Тот же эмулятор, которым проверяется прошивка; реальный COM не открывает."""
    def __init__(self, jpeg):
        from camera_emulator import C328
        self.camera = C328(jpeg)
        self.pending = bytearray()

    def write(self, data):
        for reply in self.camera.feed(data):
            self.pending.extend(reply)

    def read(self):
        data = bytes(self.pending[:512])
        del self.pending[:512]
        return data

    def close(self):
        pass


def list_ports():
    import winreg
    result = set()
    try:
        with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r'HARDWARE\DEVICEMAP\SERIALCOMM') as key:
            index = 0
            while True:
                try:
                    _, value, _ = winreg.EnumValue(key, index)
                except OSError:
                    break
                if value.upper().startswith('COM'):
                    result.add(value.upper())
                index += 1
    except FileNotFoundError:
        pass
    return sorted(result, key=lambda name: (int(name[3:]) if name[3:].isdigit() else 99999, name))
