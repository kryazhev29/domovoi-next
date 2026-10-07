# Доработка: Кряжев А., 2026
"""Установка скачанной FW ждёт свободную флеш, а не объявляет сбой.

Найдено на плате 07.10.2026: образ скачан целиком (200 с), но чтение образа
попало на занятую флеш, и установка закончилась событием 133 с причиной 11.
"""
from storage_machine import fields
from test_access_reads import AccessMachine


def main():
    m = AccessMachine()
    calls = []
    for name in ('app_boot_request', 'user_savereset', 'fw_stop'):
        m.stubs[m.addr(name)] = (lambda n: lambda: (calls.append(n), m.ret(1)))(name)
    # Проверка сроков ID здесь не участвует: в модели она стартовала бы сразу.
    m.stubs[m.addr('expiry_runtime_begin')] = lambda: m.ret(0)
    reboot = m.device + fields('device_s')['rebootToUpdate']
    m.write(reboot, 1, 1)

    def second():
        m.checked('timers_set', 3, 1000); m.checked('user_loop')

    # Флеш занята другим владельцем: установка откладывается, сбоя нет.
    m.write(m.addr('flash_owner'), 1, 1)
    second()
    assert calls == [], calls
    assert m.u.mem_read(reboot, 1)[0] == 1, 'запрос установки потерян'

    # Флеш свободна: образ передаётся загрузчику и плата уходит в перезагрузку.
    # Фоновая работа, начатая за эти секунды, тоже дожидается конца — без отказа.
    m.write(m.addr('flash_owner'), 0, 1)
    for _ in range(20):
        second()
        if calls: break
    assert calls == ['app_boot_request', 'user_savereset'], calls
    assert m.u.mem_read(reboot, 1)[0] == 0
    print('PASS установка FW ждёт свободную флеш')


if __name__ == '__main__':
    main()
