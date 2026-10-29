"""Функции входов 1–7 (ТЗ «Домовой NEXT» п. 6.3.3.4) на настоящем ELF: SMS серверу, датчик транспорта,
снимок по тревоге, кнопка открытия, аппаратная разблокировка; датчик ворот на функции 3 — test_unblock."""
from test_unblock import UnblockMachine, parse
from test_access_reads import GROUP
from storage_machine import fields

HIGH, LOW = 3300, 0
PHONE = b'+79031234567' + bytes(1)


def machine(function=0, index=0, mask=2):
    m = UnblockMachine(mask)
    m.write(m.addr('config') + m.cfg['inputMode'] + index, function, 1)
    m.adc = [0, 0]
    m.stubs[m.addr('adc_startInjectedConvertion')] = lambda: m.ret(1)
    m.stubs[m.addr('adc_getinjectedvalue_mV')] = lambda: m.ret(m.adc[m.args()[0] - 8])
    m.texts = []; m.server_ok = 1
    def send():
        m.texts.append(bytes(m.u.mem_read(m.args()[0], 32)).split(bytes(1), 1)[0].decode())
        m.ret(m.server_ok)
    m.stubs[m.addr('user_send_smsToServer')] = send
    m.photos = []; m.photo_code = 0
    m.stubs[m.addr('photo_start')] = lambda: (m.photos.append(m.args()[0]), m.ret(m.photo_code))
    return m


def level(m, one, two=LOW):
    m.adc = [one, two]; m.checked('user_readinputs')


def known_id(m):
    m.put_group(1); m.put_id(1, b'0000123456')
    m.flash[GROUP + fields('cellGroup_s')['onPhone']] = 1


def sms():
    for f, text, event in ((1, 'Drive board failure', 42), (2, 'Case opened', 43), (3, 'Gate not closed', 44)):
        for index in (0, 1):
            m = machine(f, index)
            hi = (HIGH, LOW) if index == 0 else (LOW, HIGH)
            m.checked('input_sms_poll'); assert not m.texts
            level(m, *hi)
            assert m.system_events.count(event) == 1 and not m.texts, (f, index, m.system_events)
            m.server_ok = 0; m.checked('input_sms_poll'); assert m.texts == [text]
            m.checked('input_sms_poll'); assert m.texts == [text, text], 'отказ сервера снял ожидание'
            m.server_ok = 1; m.checked('input_sms_poll'); assert len(m.texts) == 3
            m.checked('input_sms_poll'); assert len(m.texts) == 3, 'повтор без нового фронта'
            level(m, *hi); level(m, LOW, LOW); m.checked('input_sms_poll')
            assert len(m.texts) == 3 and m.system_events.count(event) == 1, 'спад входа дал SMS'
            level(m, *hi); m.checked('input_sms_poll')
            assert len(m.texts) == 4 and m.system_events.count(event) == 2
    # Две функции сразу: по одному тексту за проход, порядок 1 -> 2; вход без функции молчит.
    m = machine(1); m.write(m.addr('config') + m.cfg['inputMode'] + 1, 2, 1)
    level(m, HIGH, HIGH)
    for _ in range(3): m.checked('input_sms_poll')
    assert m.texts == ['Drive board failure', 'Case opened'], m.texts
    m = machine(0); level(m, HIGH, HIGH); m.checked('input_sms_poll')
    assert not m.texts and 42 not in m.system_events
    return {'sms_functions': 3, 'inputs': 2, 'retry_and_edges': 'PASS'}


def transport():
    number = 0x21000600
    for index in (0, 1):
        m = machine(4, index, mask=0); known_id(m)  # без реле разблокировки: пересчёт в advance не трогает выходы
        opens = lambda: [x for x in m.loads if x[0] == 0]  # импульсы реле 1 (реле 2 щёлкает вместе с ним)
        m.u.mem_write(number, PHONE); m.put_id(2, b'9031234567')
        hi = (HIGH, LOW) if index == 0 else (LOW, HIGH)
        level(m, LOW, LOW)
        m.checked('WG_readedID', 0x123456); m.checked('user_incomingcall', number, 1)
        assert not m.loads and not m.events, 'без сигнала транспорта ID открыл'
        level(m, *hi)
        m.checked('WG_readedID', 0x123456); assert len(opens()) == 1 and m.events[-1][0] == 71, (m.loads, m.events)
        m.advance(5000); m.checked('user_incomingcall', number, 1)
        assert len(opens()) == 2 and m.events[-1][0] == 69, (m.loads, m.events)
        # Команда 8 от сервера датчик транспорта не смотрит.
        level(m, LOW, LOW); m.advance(5000)
        parse(m, 8, bytes([3, 1, 0]), channel=1)
        assert m.answers[-1] == (8, 0, 1, 37) and len(opens()) == 3, (m.answers, m.loads)
    # Без функции 4 вход на доступ не влияет.
    m = machine(0, mask=0); known_id(m); level(m, LOW, LOW)
    m.checked('WG_readedID', 0x123456); assert len([x for x in m.loads if x[0] == 0]) == 1
    return {'transport_gate': 'PASS', 'server_cmd8_bypass': 'PASS'}


def photo():
    m = machine(5); level(m, HIGH)
    assert not m.photos and 45 not in m.system_events, 'снимок без камеры'
    m = machine(5); c = m.addr('config')
    m.write(c + m.cfg['rs485Mode'], 1, 1)
    m.u.mem_write(c + m.cfg['rs485Periphery'] + 2, bytes([7, 9]))  # устройство 2: адрес 7, фотокамера JPEG
    level(m, HIGH); assert m.photos == [7] and m.system_events.count(45) == 1
    level(m, HIGH); level(m, LOW); assert m.photos == [7], 'снимок без фронта'
    m.photo_code = 2; level(m, HIGH)
    assert m.photos == [7, 7] and m.system_events.count(45) == 1, 'занятая камера дала событие'
    m.write(c + m.cfg['rs485Mode'], 0, 1); m.photo_code = 0; level(m, LOW); level(m, HIGH)
    assert m.photos == [7, 7], 'пассивный 485 снимает'
    return {'alarm_photo': 'PASS'}


def button():
    m = machine(6, mask=0)
    level(m, HIGH)
    assert [x[:2] for x in m.loads] == [(0, 3), (1, 3)], m.loads
    assert m.events[-1][0] == 80 and m.events[-1][2] == 255, m.events
    # Занятый импульс: повторное нажатие молчит; после паузы — снова.
    level(m, LOW); level(m, HIGH); assert len(m.loads) == 2 and len(m.events) == 1
    m.advance(5000); level(m, LOW); level(m, HIGH); assert len(m.loads) == 4 and len(m.events) == 2
    # Реле разблокировки кнопка не трогает; при разблокировке кнопка не действует.
    m = machine(6, mask=2); level(m, HIGH)
    assert [x[0] for x in m.loads] == [0] and m.events[-1][0] == 80
    m = machine(6, mask=2); m.set_trigger('handUnBlock', 1); m.checked('user_unblock_recheck', 1)
    n = len(m.loads); level(m, HIGH); assert len(m.loads) == n and not m.events
    # Уровень: кнопка переключает выход, как проход ID.
    m = machine(6, mask=0); m.set_relay(0, 'mode', 1); level(m, HIGH)
    assert m.loads[0][:2] == (0, 2), m.loads
    return {'button_pulse': 'PASS', 'button_level_toggle': 'PASS', 'button_blocked_by_unblock': 'PASS'}


def hw_unblock():
    for mask, held, idle in ((2, (1, 0), (0, 1)), (0, (1, 1), (0, 0)), (3, (0, 0), (1, 1))):
        m = machine(7, mask=mask); m.checked('user_unblock_recheck', 1)
        assert m.outputs() == idle and m.effective() == 0, (mask, m.outputs())
        level(m, HIGH)
        assert m.effective() == 3 and m.outputs() == held and 46 in m.system_events, (mask, m.outputs())
        known_id(m); n = len(m.loads)
        m.checked('WG_readedID', 0x123456); assert len(m.loads) == n, 'ID открыл при аппаратной разблокировке'
        m.advance(61000); assert m.effective() == 3 and m.outputs() == held, 'минутный пересчёт снял'
        level(m, LOW)
        assert m.effective() == 0 and m.outputs() == idle and 47 in m.system_events, (mask, m.outputs())
    # Ручная разблокировка остаётся после снятия аппаратной; реле разблокировки не щёлкает.
    m = machine(7, mask=2); m.set_trigger('handUnBlock', 1); m.checked('user_unblock_recheck', 0)
    assert m.effective() == 1
    level(m, HIGH); assert m.effective() == 3 and m.outputs() == (1, 0)
    level(m, LOW); assert m.effective() == 1 and m.outputs() == (0, 0)
    # Журнал: ручная выкл -> аппаратная вкл, затем аппаратная выкл -> ручная вкл.
    assert m.system_events[2:4] == [31, 46] and m.system_events[-3:] == [21, 47, 30], m.system_events
    # Второй вход с функцией 7 держит разблокировку, пока активен хотя бы один.
    m = machine(7, 0); m.write(m.addr('config') + m.cfg['inputMode'] + 1, 7, 1)
    level(m, HIGH, HIGH); level(m, LOW, HIGH); assert m.effective() == 3
    level(m, LOW, LOW); assert m.effective() == 0
    return {'hw_unblock_masks': 3, 'manual_restored': 'PASS', 'two_inputs': 'PASS'}


def gate():
    from test_unblock import gate_sensor
    return gate_sensor()


def mutations():
    from test_flash_mutations import run as mutate
    return mutate({'inputs-transport': ('transport', [
        ('User/c/user_access.c', 'input_assigned(IN_TRANSPORT) && !input_active(IN_TRANSPORT)', 'input_assigned(IN_TRANSPORT) && input_active(IN_TRANSPORT)'),
    ]), 'inputs-sms-retry': ('sms', [
        ('User/c/user_inputs.c', 'if (user_send_smsToServer((char *)sms_text[i])) sms_due', 'user_send_smsToServer((char *)sms_text[i]); sms_due'),
    ]), 'inputs-hw-relay': ('hw_unblock', [
        ('User/c/user_access.c', 'state = config.rele[i].useInBlock ? OUT_OFF : OUT_ON;', 'state = OUT_OFF;'),
    ]), 'inputs-gate-polarity': ('gate', [
        ('User/c/user_access.c', 'input_assigned(IN_SMS_GATE) && !input_active(IN_SMS_GATE)', 'input_active(IN_SMS_GATE)'),
    ]), 'inputs-photo-event': ('photo', [
        ('User/c/user_inputs.c', 'photo_start(config.rs485Periphery[i].address) == CMD_SUCCESS)', 'photo_start(config.rs485Periphery[i].address) != 255)'),
    ])}, 'test_inputs', 'inputs-mutations')
