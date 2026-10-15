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
    if args.output:
        args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print('PASS настройки связи ELF:', len(result['cases']), 'набора, команда/старт/исправление')


if __name__ == '__main__':
    main()
