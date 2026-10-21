# Доработка: Кряжев А., 2026
"""Настройки связи на ELF: настоящий старт, отказ до записи и исправление команды 12.
Моделируется только граница UART и NOR; валидатор, адаптер и цикл связи реальные.
Неверный сохранённый набор наблюдается дольше суток, включая сроки перезапуска.
"""
import argparse
import hashlib
import json
import os
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'Tests'), str(ROOT / 'Tests/asm')]
from firmware_suite import select_image
from iar_image import image_paths
from iar_layout import fields
from storage_machine import crc16, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_PC
from test_service_runtime import ServiceMachine


# Сформировать полный проверенный кадр команды 12 с фиксированным уникальным номером.
def command(machine, config):
    body = struct.pack('<HBBB', len(config) + 3, 0, 73, 12) + config
    machine.u.mem_write(0x21000000, body + struct.pack('<H', crc16(body)))
    machine.receive_command(0x21000000, 0)


# Подготовить внешний набор настроек, сохраняя реальные размеры полей из DWARF.
def configuration():
    offsets = fields('config_s')
    config = bytearray(offsets['crc'] + 2)
    for name, value in [('server1', b'example.org:9000'), ('countryCode', b'7'),
                        ('netOperator', b'internet,,')]:
        at = offsets[name]
        config[at:at + len(value)] = value
    config[offsets['useServer1']] = 1
    struct.pack_into('<I', config, offsets['datetime'], 1791331200)
    return config


# Запустить реальный адаптер, подменяя только недоступное периферийное оборудование.
def machine(config, modem_type=2):
    m = ServiceMachine()
    m.sent.clear()
    m.stubs.pop(m.addr('user_send_answer_comand'), None)
    m.u.mem_write(m.addr('config'), bytes(config))
    m.write(m.device + m.d['moduleType'], modem_type, 1)
    m.uart_starts = 0
    # Запомнить физический запуск UART, не касаясь регистров периферии.
    def uart_start():
        m.uart_starts += 1
        m.ret()
    m.stubs[m.addr('comm_uart_init')] = uart_start
    m.stubs[m.addr('comm_uart_abort')] = m.ret
    m.checked('user_comm_boot')
    return m


# Проверить байты ответа 11, отсутствие записи и полное сохранение прежнего config/device.
def rejected(m, candidate):
    before = (bytes(m.u.mem_read(m.addr('config'), 512)),
              bytes(m.u.mem_read(m.device, 512)), bytes(m.flash), m.counts['start'])
    command(m, candidate)
    payload = struct.pack('<HBBBBIH', 10, 255, 73, 11, 1, 0x78563412, 2048)
    assert m.sent, 'неверная команда 12 должна получить немедленный отказ'
    assert m.sent[-1] == b'SLDN' + payload + struct.pack('<H', crc16(payload)), m.sent[-1].hex()
    after = (bytes(m.u.mem_read(m.addr('config'), 512)),
             bytes(m.u.mem_read(m.device, 512)), bytes(m.flash), m.counts['start'])
    assert after == before, 'отказ изменил настройки или записал флеш'


# Дождаться реального атомарного сохранения и применения исправленного набора.
def accepted(m, config):
    m.sent.clear()
    command(m, config)
    for _ in range(5000):
        m.now += 1
        m.checked('user_storage_poll')
        if m.sent:
            break
    else:
        raise AssertionError('верная команда 12 не закончилась')
    assert m.sent[-1][8:10] == bytes([12, 4]), m.sent[-1].hex()
    assert bytes(m.u.mem_read(m.addr('config'), 510)) == bytes(config[:510])
    assert m.counts['start'] > 0, 'верная команда не записана'


# Покрыть повреждённую команду и повреждённый долговечный набор одним настоящим стартом.
def run():
    good = configuration()
    offsets = fields('config_s')
    cases = []
    for name, field, value, modem_type in [
        ('символ адреса', 'server1', b'bad;host:9000', 2),
        ('порт 0', 'server1', b'example.org:0', 2),
        ('резервный порт 0', 'server2', b'backup.local:0', 2),
        ('символ резервного адреса', 'server2', b'bad;backup:9000', 2),
        ('переполнение порта', 'server1', b'example.org:65537', 2),
        ('знак порта', 'server1', b'example.org:-1', 2),
        ('хвост порта', 'server1', b'example.org:9000junk', 2),
        ('кавычка APN', 'netOperator', b'inte"rnet,,', 2),
        ('другой модем', None, b'', 1),
    ]:
        bad = bytearray(good)
        if field:
            at = offsets[field]
            bad[at:at + 32] = bytes(32)
            bad[at:at + len(value)] = value
            if field == 'server2':
                bad[offsets['useServer2']] = 1
        m = machine(good)
        assert m.uart_starts == 1
        m.write(m.device + m.d['moduleType'], modem_type, 1)
        rejected(m, bad)
        # Тип задаёт команда 105, а не 12; восстановить его перед верной командой 12.
        m.write(m.device + m.d['moduleType'], 2, 1)
        accepted(m, good)

        m = machine(bad, modem_type)
        assert m.checked('comm_modem_state') == 0 and not m.uart_starts
        starts = m.counts['start']
        for now in (0, 1, 300, 10000, 60000, 600000, 3600000, 86400000, 90000000):
            m.now = now
            m.checked('user_comm_poll', now)
            assert m.checked('comm_modem_state') == 0 and not m.uart_starts
        assert m.counts['start'] == starts, 'цикл записал флеш при отвергнутом старте'
        rejected(m, bad)
        m.write(m.device + m.d['moduleType'], 2, 1)
        accepted(m, good)
        assert m.uart_starts == 1, 'исправление не запустило связь'
        cases.append(name)
    m = machine(good)
    m.stubs[m.addr('user_writeConfigAndDevice')] = m.ret
    m.checked('user_setdefault')
    assert bytes(m.u.mem_read(m.addr('config') + offsets['netOperator'], 32)) == bytes(32), 'заводской APN не автоматический'
    cases.append('заводской APN автоматический')
    return cases


# Снимать готовность флеша после каждого обработчика, способного сменить его владельца.
def dispatcher_ownership():
    for server_busy, call_busy, expected_call, expected_sms in [
            (True, False, False, 0), (False, True, True, 0), (False, False, True, 1)]:
        m = machine(configuration())
        calls, sms = [], []
        # Смоделировать захват флеша выполненной командой сервера.
        def begin_store():
            # Хвостовой вызов настоящего begin внутри исполняемого user_comm_poll.
            m.u.reg_write(UC_ARM_REG_R0, 12)
            m.u.mem_write(0x21000400, struct.pack('<IB3x',0,30))
            m.u.reg_write(UC_ARM_REG_R1, 0x21000400)
            m.u.reg_write(UC_ARM_REG_PC, m.addr('flash_exec_begin') | 1)
        def server_poll():
            if server_busy:begin_store()
            else:m.ret()
        # Запомнить допуск вызова и смоделировать захват его обработчиком.
        def call_poll():
            calls.append(bool(m.args()[1]))
            if call_busy:
                begin_store()
            else:m.ret()
        # Запомнить допуск обработчика SMS.
        def sms_poll():
            sms.append(1)
            m.ret()
        for name in ('comm_recover_poll', 'comm_info_poll', 'comm_balance_poll'):
            m.stubs[m.addr(name)] = m.ret
        m.stubs[m.addr('comm_server_poll')] = server_poll
        m.stubs[m.addr('comm_call_poll')] = call_poll
        m.stubs[m.addr('comm_sms_poll')] = sms_poll
        m.checked('user_comm_poll', 100)
        assert calls == [expected_call] and len(sms) == expected_sms, (server_busy, call_busy, calls, sms)
        assert m.exec_state() == ((12,30,1) if server_busy or call_busy else (0,0,0))
        assert not m.io_budget
    # Единственный выбранный сервер 2 не должен резервировать сам себя.
    config = configuration()
    offsets = fields('config_s')
    config[offsets['useServer1']] = 0
    config[offsets['useServer2']] = 1
    at = offsets['server2']
    config[at:at + 32] = bytes(32)
    config[at:at + 17] = b'backup.local:9000'
    m = machine(config)
    assert m.uart_starts == 1 and not m.checked('comm_modem_reserve')
    return 4


def balance_ownership():
    # Проверяется исполняемый адаптер user_comm_poll, а не копия его выражения.
    names = ['idle', 'call', 'firmware', 'photo', 'photo-ready', 'upload', 'update-ready', 'update-reboot']
    for name in names:
        m = machine(configuration()); allowed = []
        for field in ('status', 'FWinProgress', 'rebootToUpdate'):
            m.write(m.device + m.d[field], 0, 2 if field == 'status' else 1)
        for fn in ('comm_server_poll', 'comm_recover_poll', 'comm_info_poll', 'comm_call_poll', 'comm_sms_poll'):
            m.stubs[m.addr(fn)] = m.ret
        m.stubs[m.addr('comm_call_active')] = lambda: m.ret(name == 'call')
        m.stubs[m.addr('photo_busy')] = lambda: m.ret(name == 'photo')
        m.stubs[m.addr('upload_runtime_busy')] = lambda: m.ret(name == 'upload')
        if name == 'firmware': m.write(m.device + m.d['FWinProgress'], 1, 1)
        if name == 'photo-ready': m.write(m.device + m.d['status'], 1 << 13, 2)
        if name == 'update-ready': m.write(m.device + m.d['status'], 1 << 11, 2)
        if name == 'update-reboot': m.write(m.device + m.d['rebootToUpdate'], 1, 1)
        def balance():
            allowed.append(bool(m.args()[1])); m.ret()
        m.stubs[m.addr('comm_balance_poll')] = balance
        m.checked('user_comm_poll', 400000)
        assert allowed == [name == 'idle'], (name, allowed)
    # Журнал: неподтверждённый пакет любого канала запрещает USSD даже без активной записи Flash.
    m = machine(configuration())
    assert not m.checked('upload_runtime_busy')
    for channel in (0, 1):
        p = m.addr('log_pending32') + 4 * channel
        m.write(p, 1); assert m.checked('upload_runtime_busy')
        m.write(p, 0); assert not m.checked('upload_runtime_busy')
    assert m.checked('upload_runtime_begin', 9, 0, 73)
    assert m.checked('upload_runtime_busy')
    return len(names) + 3


def fallback_config():
    """Временный GSM на настоящем ELF не меняет config, ответы 12/106 и запись NOR."""
    good = configuration()
    struct.pack_into('<H', good, 510, crc16(good[:510]))
    m = machine(good)
    m.checked('comm_recover_init', 0, 123456)
    def send_config():
        p, count = m.args()[:2]
        assert 0 < count <= 1040
        m.sent.append(bytes(m.u.mem_read(p, count)))
        m.ret(1)
    m.stubs[m.addr('usb_send')] = send_config
    # Радиосеть и PDP - внешняя граница; адаптер config и автомат восстановления настоящие.
    m.stubs[m.addr('comm_modem_lte')] = lambda: m.ret(1)
    m.stubs[m.addr('comm_modem_state')] = lambda: m.ret(2)
    m.stubs[m.addr('comm_channel_bridge')] = lambda: m.ret(0)
    lines = []
    def log():
        kind, body, count = m.args()[:3]
        lines.append((kind, bytes(m.u.mem_read(body, count))))
        m.ret()
    m.stubs[m.addr('usb_log')] = log
    # 106 до и после: кадр сравнивается целиком, включая CRC.
    m.sent.clear()
    m.checked('user_send_configToConfigurator', 0)
    before = m.sent[-1]
    for _ in range(2):
        m.checked('comm_recover_data', 0)
        m.checked('comm_recover_data', 1)
    assert m.checked('comm_number', 6) == 1
    assert m.checked('comm_number', 11) == 1  # сохранённое авто
    m.sent.clear()
    m.checked('user_send_configToConfigurator', 0)
    assert m.sent[-1] == before, 'временный GSM попал в 106'
    assert lines == [(3, b'4G->2G')], lines
    accepted(m, good)
    assert m.checked('comm_number', 6) == 1, 'синхронизация 12 отменила сутки GSM'
    # Ответ 12 содержит код результата, а не config; accepted уже сверил все 510 сохраняемых байт.
    m.sent.clear()
    m.checked('user_send_configToConfigurator', 0)
    assert m.sent[-1] == before, 'синхронизация изменила выдаваемый netType'
    expected = [0, 1, 0, 5, 3]
    for raw in range(1, 5):
        m.write(m.addr('config') + fields('config_s')['netType'], raw, 1)
        assert m.checked('comm_number', 6) == expected[raw]
        assert m.checked('comm_number', 11) == 0
    m.write(m.addr('config') + fields('config_s')['netType'], 0, 1)
    assert m.checked('comm_number', 6) == 1
    m.stubs[m.addr('comm_modem_restart')] = lambda: m.ret(0)
    m.checked('comm_recover_poll', 123456 + 86400000 - 1)
    assert m.checked('comm_number', 6) == 1 and len(lines) == 1
    m.checked('comm_recover_poll', 123456 + 86400000)
    assert m.checked('comm_number', 6) == 0 and lines[-1] == (3, b'->AUTO')
    m.checked('comm_recover_data', 0)
    m.checked('comm_recover_data', 1)
    assert m.checked('comm_number', 6) == 1  # одна неудача пробного авто
    m.checked('comm_recover_init', 0, 0)
    assert m.checked('comm_number', 6) == 0
    return '12/106 unchanged; sync keeps GSM; explicit modes; boot auto; USB text'


# Выбрать только единственный проект указанного профиля и сохранить привязку результата к ELF.
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-root', type=Path, required=True)
    parser.add_argument('--config', choices=('Release', 'ReleaseCustomer'), default='Release')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    os.environ['DOMOVOI_COMPARE_CONFIG'] = args.config
    select_image(args.build_root)
    result = {'cases': run(), 'elf_sha256': hashlib.sha256(image_paths()[0].read_bytes()).hexdigest()}
    result['ownership_checks'] = dispatcher_ownership()
    result['balance_ownership_checks'] = balance_ownership()
    result['fallback_config'] = fallback_config()
    if args.output:
        args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print('PASS настройки связи ELF:', len(result['cases']), 'набора, команда/старт/исправление')


if __name__ == '__main__':
    main()
