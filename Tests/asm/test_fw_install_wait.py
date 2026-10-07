from test_flash_executor import hold_clear, release_clear
# Доработка: Кряжев А., 2026
"""Установка FW и подтверждение пробы ждут свободную флеш, а не теряются.

Найдено на плате 07.10.2026: образ скачан целиком (200 с), но чтение образа
попало на занятую флеш, и установка закончилась событием 133 с причиной 11.
Подтверждение пробы писалось один раз при регистрации: при занятой флеш
через 30 мин загрузчик откатил бы годную прошивку.
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
    hold_clear(m)
    second()
    assert calls == [], calls
    assert m.u.mem_read(reboot, 1)[0] == 1, 'запрос установки потерян'

    # Флеш свободна: образ передаётся загрузчику и плата уходит в перезагрузку.
    # Фоновая работа, начатая за эти секунды, тоже дожидается конца — без отказа.
    release_clear(m)
    for _ in range(20):
        second()
        if calls: break
    assert calls == ['app_boot_request', 'user_savereset'], calls
    assert m.u.mem_read(reboot, 1)[0] == 0

    # Проба: регистрация есть, флеш занята - подтверждения нет; освободилась - записано.
    m = AccessMachine(); calls = []; state = {'trial': 1}
    m.stubs[m.addr('expiry_runtime_begin')] = lambda: m.ret(0)
    m.stubs[m.addr('app_trial_active')] = lambda: m.ret(state['trial'])
    m.stubs[m.addr('app_trial_expired')] = lambda: m.ret(0)
    m.stubs[m.addr('app_trial_confirm')] = lambda: (calls.append('confirm'), state.update(trial=0), m.ret())
    m.stubs[m.addr('fw_event')] = lambda: (calls.append(m.args()[0]), m.ret())
    m.write(m.device + fields('device_s')['registredOnServer'], 1, 1)
    hold_clear(m)
    second()
    assert calls == [], calls
    release_clear(m)
    for _ in range(20):
        second()
        if calls: break
    assert calls == ['confirm', 136], calls
    print('PASS установка FW и подтверждение пробы ждут свободную флеш')


if __name__ == '__main__':
    main()
