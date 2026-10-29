"""Камера номеров Interceptor (тип 485 = 8) на настоящем ELF IAR: опрос номера FT+IDN, проход по номеру,
снимок потоком FT+IDC прямо во флеш, отказы потока и занятая линия. Протокол — camera.docx и ответы камеры
28.10.2026 (Логи/2026-10-28_камера_номеров_loop0). Линия 485 моделируется как драйвер USART: байты
дописываются в буфер владельца, uart_rebuffer сдвигает непрочитанное, лишнее при полном буфере теряется."""
import struct
import zlib
from test_photo_access import PassageMachine
from test_photo_runtime import fields, HEAD, FORMAT
from test_access_reads import GROUP, TODAY

P_ERASE, P_IDC = 7, 8
PLATE = b'A001XH197\x00'   # 9 символов латиницей и ноль — как отдаёт камера
PLATE2 = b'C020PM177\x00'
JPEG = bytes([0xff, 0xd8]) + bytes((i * 131 + (i >> 8) * 7) & 255 for i in range(42198)) + bytes([0xff, 0xd9])


def answer(number, address=b'0'):
    """Ответ на FT+IDN: ++FT<адрес>IDN<номер 10 Б>CRLF; номер из нулей — в кадре пусто."""
    return b'++FT' + address + b'IDN' + number + b'\r\n'


def stream(jpeg=JPEG, number=PLATE, crc=None, size=None):
    """Ответ на FT+IDC: заголовок, JPEG, CRC32 zlib и размер младшим байтом вперёд, CRLF."""
    crc = zlib.crc32(jpeg) if crc is None else crc
    return b'++FT0IDC' + number + b':' + jpeg + struct.pack('<II', crc, len(jpeg) if size is None else size) + b'\r\n'


class AnprMachine(PassageMachine):
    def __init__(self, flash=None):
        super().__init__(flash)
        self.u.mem_write(self.cfg + self.c['rs485Periphery'], bytes([48, 8]) + bytes(14))
        for i in range(2):
            self.write(self.cfg + self.c['rele'] + i * 11 + self.rele['onCamera'], 1, 1)
        self.flash[GROUP + fields('cellGroup_s')['onAuto']] = 1
        self.put_id(2, PLATE); self.put_id(3, PLATE2)
        self.line = (self.addr('wf_line'), 96); self.count = 0; self.lines = []; self.lost = 0
        for name, fn in [('uart_init', self.uart_init), ('uart_received', lambda: self.ret(self.count)),
                         ('uart_clear', self.uart_clear), ('uart_rebuffer', self.uart_rebuffer),
                         ('uart_send', self.uart_send), ('uart_getstate', lambda: self.ret(self.count != 0)),
                         ('uart_isReceiving', lambda: self.ret(0))]:
            self.stubs[self.addr(name)] = fn

    def uart_init(self):
        _, _, buffer, size = self.args(); self.line = (buffer, size); self.count = 0; self.ret(1)

    def uart_clear(self):
        self.count = 0; self.ret()

    def uart_rebuffer(self):
        buffer, size, used, _ = self.args(); start = self.line[0]
        left = min(max(self.count - used, 0), size) if used != 0xffff else 0
        data = bytes(self.u.mem_read(start + used, left)) if left else b''
        if data: self.u.mem_write(buffer, data)
        self.line = (buffer, size); self.count = left; self.ret()

    def uart_send(self):
        p, n, _, _ = self.args(); self.lines.append((bytes(self.u.mem_read(p, n)), self.now)); self.ret()

    def hear(self, data):
        """Байты с линии: дописать в буфер владельца, как прерывание USART; не влезшее теряется."""
        start, size = self.line
        taken = data[:max(size - self.count, 0)] if start else b''
        if taken: self.u.mem_write(start + self.count, taken); self.count += len(taken)
        self.lost += len(data) - len(taken)

    def line_poll(self):
        self.now += 1; self.checked('port_check_data')

    def step(self):
        """Проход главного цикла: линия 485, затем хранилище."""
        self.line_poll(); self.tick()

    def stage(self):
        return self.pget('stage') if self.photo_ptr() else 0

    def state(self):
        return bytes(self.u.mem_read(self.device + self.d['peripheryState'], 8))

    def last(self):
        return self.lines[-1][0] if self.lines else b''

    def count_photos(self):
        return int.from_bytes(self.u.mem_read(self.device + self.d['countPhoto'], 2), 'little')


def ready(flash=None):
    m = AnprMachine(flash); m.checked('log_getRecordsCount')
    m.write(m.addr('store_save_requested'), 0, 1)
    m.boot_media(); m.line_poll()
    return m


def ask(m, command=b'FT+IDN0\r\n'):
    """Довести опрос до следующего запроса номера (не чаще раза в секунду)."""
    before = len(m.lines)
    for _ in range(400):
        m.step()
        if len(m.lines) > before:
            assert m.last() == command, (m.last(), command)
            return m.lines[-1][1]  # время отправки запроса
        m.now += 10
    raise AssertionError('опрос не спросил номер')


def until(m, check, limit=5000, what='состояние'):
    for _ in range(limit):
        if check(): return
        m.step()
    raise AssertionError(what + ' не наступило')


def to_idc(m, number=PLATE):
    """Номер в кадре -> проход -> место под снимок стёрто -> запрос IDC."""
    ask(m); m.hear(answer(number)); m.line_poll()
    until(m, lambda: m.last() == b'FT+IDC0\r\n' and m.stage() == P_IDC, what='запрос снимка')
    return m.pget('address', 4)


def feed(m, data, chunk=200):
    """Поток камеры без пауз порциями; между порциями главный цикл успевает записать принятое.
    Съёмка, остановленная посреди потока, остаток не принимает."""
    for off in range(0, len(data), chunk):
        if not m.stage(): break
        m.hear(data[off:off + chunk])
        until(m, lambda: m.count <= 250, limit=200, what='запись принятого')
    assert not m.lost, ('потеря байтов линии', m.lost)


def finish(m, limit=300):
    until(m, lambda: m.stage() == 0, limit=limit, what='конец съёмки')


def plate_photo():
    m = ready()
    sent = ask(m); assert m.state() == bytes(8)
    # Ответ приходит с паузой посреди строки: проход только по полному ответу.
    m.hear(answer(PLATE)[:6]); m.line_poll(); assert not m.loads
    m.hear(answer(PLATE)[6:]); m.line_poll()
    assert len(m.loads) == 2 and m.state()[0] == 1, (m.loads, m.state())
    assert [row for row in m.logs() if row[0] == 73] == [(73, PLATE)], m.logs()
    # Снимок прохода: камера номеров с номером в кадре; место 12 секторов + сектор впереди.
    m.flash[0x200000:0x20e000] = bytes(0x0e000)  # грязь в кольце фото: снимок пишется только в стёртое
    assert m.active() and m.rget('address') == 48
    until(m, lambda: m.last() == b'FT+IDC0\r\n' and m.stage() == P_IDC, what='запрос снимка')
    address = m.pget('address', 4)
    assert address == 0x200000 and m.flash[0x200000:0x20d000] == b'\xff' * 0x0d000, 'место не стёрто'
    assert m.flash[0x20d000:0x20e000] == bytes(4096), 'стёрто лишнее'
    # Снимок по ответу камеры не ждёт окна запроса номера: номер уже получен.
    idc_at = m.lines[-1][1]; assert idc_at - sent < 3000, idc_at - sent
    feed(m, stream()); finish(m)
    assert m.flash[address:address + HEAD] == struct.pack('<IIIB', TODAY, len(JPEG), FORMAT, 255)
    assert m.flash[address + HEAD:address + HEAD + len(JPEG)] == JPEG
    end = (address + HEAD + len(JPEG) + 4095) & ~4095
    assert m.flash[end:end + 4096] == b'\xff' * 4096, 'следующий снимок без стёртого сектора'
    assert m.count_photos() == 1 and int.from_bytes(m.u.mem_read(m.device + m.d['status'], 2), 'little') & (1 << 13)
    assert not m.u.mem_read(m.device + m.d['getPhoto'], 1)[0]
    m.flush_logs()
    assert [row for row in m.logs() if row[0] == 129] == [(129, struct.pack('<I', TODAY) + bytes(6))], m.logs()
    # Линия снова за опросом; тот же номер в кадре второго прохода и снимка не даёт.
    ask(m); m.hear(answer(PLATE)); m.line_poll()
    assert len(m.loads) == 2 and not m.active() and m.stage() == 0
    # Второй номер: второй снимок встаёт сразу за первым.
    address2 = to_idc(m, PLATE2); assert address2 == end
    feed(m, stream(JPEG[:5000], PLATE2)); finish(m)
    assert m.count_photos() == 2 and m.flash[address2 + HEAD:address2 + HEAD + 5000] == JPEG[:5000]
    return {'jpeg': len(JPEG), 'record_sectors': (end - address) // 4096, 'idc_after_poll_ms': idc_at - sent,
            'max_storage_poll': max(m.poll_costs), 'stack': m.stack_peak}


def photo_faults():
    results = {}
    for case in ('crc', 'size', 'header', 'silence', 'overrun', 'oversize'):
        m = ready(); address = to_idc(m)
        if case == 'crc': feed(m, stream(crc=zlib.crc32(JPEG) ^ 1))
        elif case == 'size': feed(m, stream(size=len(JPEG) - 1))
        elif case == 'header': m.hear(answer(PLATE)); m.line_poll(); assert m.stage() == 0, 'чужой ответ принят'
        elif case == 'silence': pass
        elif case == 'overrun':  # главный цикл не успел: часть потока потеряна
            data = stream(); m.hear(data[:3000]); assert m.lost == 3000 - 512
            until(m, lambda: m.count <= 250, limit=200, what='разбор после переполнения')
            m.lost = 0; feed(m, data[3000:])
        else: feed(m, stream(bytes(49139 + 500)))
        finish(m, limit=4000)
        assert m.count_photos() == 0 and m.flash[address:address + 12] == b'\xff' * 12, case
        assert not m.u.mem_read(m.device + m.d['getPhoto'], 1)[0] and not m.active(), case
        assert 129 not in [row[0] for row in m.logs()], case
        # Линия вернулась опросу; следующий снимок встаёт на то же место поверх стёртого заново.
        m.now += 16000
        assert to_idc(m) == address, case
        feed(m, stream(JPEG[:3000])); finish(m)
        assert m.count_photos() == 1 and m.flash[address + HEAD:address + HEAD + 3000] == JPEG[:3000], case
        results[case] = 'no photo, line back, next photo on same place'
    return results


def poll_states():
    m = ready()
    # Пустой кадр: камера на связи, проходов нет.
    ask(m); m.hear(answer(bytes(10))); m.line_poll()
    assert m.state()[0] == 1 and not m.loads and not m.active()
    # Нет ответа 3 с: камера не на связи; следующий запрос — сразу после ожидания.
    sent = ask(m)
    until(m, lambda: m.state()[0] == 0, what='отметка «нет ответа»')
    assert m.lines[-1][1] - sent >= 3000 and m.last() == b'FT+IDN0\r\n' and len(m.lines) == 3
    # Команда 7 без номера в кадре: камера номеров снимать не берётся.
    m.packet(7, b'\x30'); assert m.answers[-1][1] == 2 and not m.active()
    # Номер не из базы: прохода нет, но снимок по команде 7 возможен 5 с.
    ask(m); m.hear(answer(b'X999XX99\x00\x00')); m.line_poll()
    assert not m.loads and [row for row in m.logs() if row[0] == 65] == [(65, b'X999XX99\x00\x00')]
    m.flush_logs()  # запись журнала важнее съёмки: пока она ждёт, команда 7 получает «повторить»
    m.packet(7, b'\x30'); assert m.answers[-1][1] == 0 and m.active(), m.answers[-1]
    until(m, lambda: m.last() == b'FT+IDC0\r\n', what='снимок по команде 7')
    feed(m, stream(JPEG[:1000], b'X999XX99\x00\x00')); finish(m); assert m.count_photos() == 1
    m.now += 5000; m.packet(7, b'\x30'); assert m.answers[-1][1] == 2, 'номер устарел, а снимок начат'
    # Две камеры номеров опрашиваются по очереди; ответ с чужим адресом не засчитывается.
    m = ready(); m.u.mem_write(m.cfg + m.c['rs485Periphery'], bytes([48, 8, 49, 8]) + bytes(12))
    ask(m, b'FT+IDN1\r\n'); m.hear(answer(bytes(10), b'1')); m.line_poll()
    ask(m); m.hear(answer(bytes(10))); m.line_poll()
    assert m.state()[:2] == b'\x01\x01'
    ask(m, b'FT+IDN1\r\n'); m.hear(answer(bytes(10))); m.line_poll()  # отвечает «0» на запрос камере «1»
    until(m, lambda: m.state()[1] == 0, what='отметка чужого адреса')
    assert m.state()[0] == 1 and m.last() == b'FT+IDN0\r\n'
    # Пассивный режим и выбранный Wi-Fi-модем: запросов номера нет.
    for name, value in (('rs485Mode', 0), ('wifiModem', 1)):
        m = ready(); base = m.cfg + m.c['rs485Mode'] if name == 'rs485Mode' else m.device + m.d['wifiModem']
        m.write(base, value, 1); before = len(m.lines)
        for _ in range(300): m.step(); m.now += 10
        assert len(m.lines) == before, name
    return {'empty_frame': 'alive', 'timeout_ms': 3000, 'command7': 'needs plate in frame', 'round_robin': 2}


def busy_line():
    """Снимок по звонку/карте, пока запрос номера ещё в линии: IDC — только после окна ответа 3 с."""
    m = ready()
    ask(m); m.hear(answer(b'X999XX99\x00\x00')); m.line_poll()
    sent = ask(m)
    m.checked('WG_readedID', 0x42)
    assert len(m.loads) == 2 and m.active()
    until(m, lambda: m.stage() == P_ERASE, what='стирание')
    for _ in range(150): m.step()  # 13 секторов стёрты, а окно ответа номера ещё идёт
    assert m.last() == b'FT+IDN0\r\n', 'IDC ушёл поверх ждущего ответа номера'
    m.hear(answer(b'X999XX99\x00\x00'))  # запоздавший ответ номера ляжет в приёмник снимка
    until(m, lambda: m.last() == b'FT+IDC0\r\n', what='запрос снимка')
    assert m.lines[-1][1] - sent >= 3000
    feed(m, stream(JPEG[:2000], b'X999XX99\x00\x00')); finish(m)
    assert m.count_photos() == 1
    return {'idc_after_ms': m.lines[-1][1] - sent}


def mutations():
    from test_flash_mutations import run as mutate
    return mutate({'anpr-tail-crc': ('photo_faults', [
        ('User/c/photo_runtime.c', 'rt_get32(photo->rx) == ~photo->crc &&', '1 &&'),
    ]), 'anpr-hold-back': ('plate_photo', [
        ('User/c/photo_runtime.c', 'if (photo->packet && !photo->used && n > 10u) {', 'if (photo->packet && !photo->used && n) {'),
        ('User/c/photo_runtime.c', 'uint16_t k = n - 10u;', 'uint16_t k = n;'),
    ]), 'anpr-wait-poll': ('busy_line', [
        ('User/c/photo_runtime.c', 'else if (!anpr.wait || now - anpr.sent >= ANPR_WAIT_MS) {', 'else {'),
    ]), 'anpr-empty-frame': ('poll_states', [
        ('User/c/photo_runtime.c', 'plate |= id[i] != 0;', 'plate = true;'),
    ])}, 'test_anpr', 'anpr-mutations')


if __name__ == '__main__':
    for name in ('plate_photo', 'photo_faults', 'poll_states', 'busy_line'):
        print(name, globals()[name]())
