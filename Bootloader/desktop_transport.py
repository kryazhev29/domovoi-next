"""Desktop transport. Importing this module never opens a serial port."""
import struct
import time
from boot_tool import application, crc16, crc32, frame


class Cancelled(Exception):
    pass


class InstallUnconfirmed(Exception):
    pass


STATUS = {1: 'Образ отклонён загрузчиком', 2: 'Ошибка чтения или записи Flash',
          3: 'Не удалось подготовить резервную копию', 4: 'Ошибка установки прошивки',
          5: 'Выполнен откат прошивки', 6: 'Требуется восстановление приложения',
          7: 'Неподдерживаемый объём Flash: требуется 128 КиБ',
          8: 'Пробная прошивка не подтвердилась, выполнен откат',
          9: 'Идёт пробный запуск новой прошивки: дождитесь подтверждения или отката'}


class Link:
    def __init__(self, device, cancel=None):
        self.device = device
        self.cancel = cancel
        self.pending = bytearray()

    def exchange(self, command, payload=b'', timeout=3, check=True,
                 expected_count=None, cancellable=True):
        self.device.write(frame(command, payload))
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if cancellable and self.cancel and self.cancel.is_set():
                raise Cancelled('Передача остановлена до запроса установки.')
            self.pending.extend(self.device.read(128))
            while True:
                pos = self.pending.find(b'SLGT')
                if pos < 0:
                    self.pending[:] = self.pending[-3:]
                    break
                del self.pending[:pos]
                if len(self.pending) < 6:
                    break
                size = struct.unpack_from('<H', self.pending, 4)[0]
                if not 2 <= size <= 592:
                    del self.pending[0]
                    continue
                if len(self.pending) < size + 8:
                    break
                body = bytes(self.pending[6:6+size])
                received_crc = struct.unpack_from('<H', self.pending, 6+size)[0]
                del self.pending[:size+8]
                if received_crc != crc16(body) or body[:2] != bytes([0x55, command]):
                    continue
                if len(body) != 10:
                    raise ValueError('Неизвестный формат ответа загрузчика.')
                if check and body[2]:
                    raise ValueError(STATUS.get(body[2], f'Ошибка {body[2]}'))
                count = struct.unpack_from('<I', body, 6)[0]
                if expected_count is not None and count != expected_count:
                    if count < expected_count:
                        continue  # delayed acknowledgement of a previous block
                    raise ValueError('Контроллер подтвердил неожиданное смещение.')
                return body
        raise TimeoutError(f'Нет ответа на команду 0x{command:02X}. Проверьте USB и режим загрузчика.')


def identify(port, factory=None, cancel=None):
    if factory is None:
        from serial import Serial
        factory = Serial
    with factory(port, 115200, timeout=.1, write_timeout=2) as device:
        device.reset_input_buffer()
        link = Link(device, cancel)
        info = link.exchange(0, timeout=1.2, check=False)
        error = link.exchange(0x23, timeout=1.2, check=False)
        return dict(port=port, version=info[3], hw=info[4], status=info[2], error=error[2])


def transfer(port, path, notify, cancel, factory=None, expected_image=None):
    """Only block writes are retried. COMMIT is never repeated after ambiguity."""
    if factory is None:
        from serial import Serial
        factory = Serial
    hw, fw, stamp, data = application(path)
    if expected_image is not None and (hw, fw, stamp, data) != expected_image:
        raise ValueError('Файл изменился после проверки. Выберите и проверьте его заново.')
    checksum = crc32(data)
    with factory(port, 115200, timeout=.1, write_timeout=3) as device:
        device.reset_input_buffer()
        link = Link(device, cancel)
        info = link.exchange(0)
        if info[3] < 0x31 or info[4] != hw:
            raise ValueError('HW файла не совпадает с платой либо загрузчик старее 0x31.')
        if cancel.is_set():
            raise Cancelled('Обновление отменено.')
        notify('stage', 'Подготовка внешней Flash — ожидайте')
        link.exchange(0x20, struct.pack('<II', len(data), checksum) + path.name.encode('ascii') + b'\0', 40)
        started = time.monotonic()
        for offset in range(0, len(data), 256):
            if cancel.is_set():
                raise Cancelled('Передача остановлена. Запрос установки не отправлен.')
            block = data[offset:offset+256]
            for attempt in range(3):
                try:
                    link.exchange(0x21, struct.pack('<I', offset)+block, 3,
                                  expected_count=offset+len(block))
                    break
                except TimeoutError:
                    if attempt == 2:
                        raise
                    notify('log', f'Нет подтверждения блока {offset}; повтор {attempt+1}/2')
            notify('progress', (offset+len(block), len(data), time.monotonic()-started))
        if cancel.is_set():
            raise Cancelled('Передача остановлена. Запрос установки не отправлен.')
        notify('commit', 'Проверка образа и запрос установки — питание не отключать')
        try:
            link.exchange(0x22, timeout=40, cancellable=False)
        except (TimeoutError, OSError) as exc:
            raise InstallUnconfirmed('Ответ на запрос установки не получен. Установка могла начаться. '
                                     'Не повторяйте запись сразу: проверьте запуск и версию приложения.') from exc
        notify('accepted', 'Образ принят. Установка и запуск приложения ещё не подтверждены. '
                           'Проверьте версию на сервере после восстановления связи.')
