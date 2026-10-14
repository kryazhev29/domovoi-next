"""Доработка: Кряжев А., 2026. Питание из настоящего SysTick при занятом user_init."""
import json
import struct
from iar_irq_support import Machine
from unicorn import UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_R1
from iar_layout import fields
from iar_image import result_path


def startup(cold, heavy, mutation=False, origin=0):
    m = Machine(); m.init()
    m.u.mem_map(0x42000000, 0x600000)
    d, c = fields('device_s'), fields('config_s')
    device, config, millis = m.addr('device'), m.addr('config'), m.addr('milliSeconds')
    trace, sent, ready_at = [], [], []
    m.write(millis, origin)
    m.write(0x40021024, (1 << 27) if cold else (1 << 28))
    m.stubs[m.addr('w25_read_devID')] = lambda: m.ret(0x17c2)
    for name in ('mem_recover', 'user_unblock_recheck', 'app_boot_metadata',
                 'sched16_boot', 'rs485_init', 'uart_setRecTimeout',
                 'comm_uart_init', 'comm_uart_abort', 'usb_timer1ms', 'uart_timer1ms'):
        m.stubs[m.addr(name)] = lambda: m.ret(1)

    def read_config():
        m.u.mem_write(config + c['server1'], b'example.org:9000\0')
        m.write(config + c['useServer1'], 1, 1)
        m.ret()

    def saved():
        m.write(device + d['moduleType'], 2, 1)
        m.ret()

    def tick():
        m.irq = ('SysTick_Handler', ())
        m.invoke_irq()
        if m.read(m.addr('power_state'), 1) == 6 and not ready_at:
            ready_at.append((m.read(millis) - origin) & 0xffffffff)

    def numbers():
        for _ in range(heavy): tick()
        m.ret(3151)

    def packet():
        p, n = (m.u.reg_read(r) for r in (UC_ARM_REG_R0, UC_ARM_REG_R1))
        sent.append(bytes(m.u.mem_read(p, n + 6)))
        m.ret(1)

    gpio_address = m.addr('comm_gpio')
    def gpio(u, address, size, unused):
        if address == gpio_address:
            trace.append(((m.read(millis) - origin) & 0xffffffff,
                          u.reg_read(UC_ARM_REG_R0), bool(u.reg_read(UC_ARM_REG_R1))))

    m.u.hook_add(UC_HOOK_CODE, gpio)
    m.stubs[m.addr('user_readconfig')] = read_config
    m.stubs[m.addr('user_readsavedinformation')] = saved
    m.stubs[m.addr('user_getNumbersCount')] = numbers
    def restored_count(count,flag):
        m.write(device+d['status'],m.read(device+d['status'],2)|flag,2);m.ret(count)
    m.stubs[m.addr('log_getRecordsCount')] = lambda: restored_count(127,0x1000)
    m.stubs[m.addr('user_getTempIDRecordsCount')] = lambda: restored_count(3,0x4000)
    m.stubs[m.addr('packet_send')] = packet
    power = m.addr('comm_power_v5_poll')
    if mutation: m.stubs[power] = m.ret
    m.call('user_init')
    assert not sent, 'регистрация/статус ушли из user_init до готовности счётчиков'
    assert [m.read(device + d[n], 2) for n in ('numbersCount', 'logsCount', 'tempIDsCount')] == [3151, 127, 3]
    assert m.read(device + d['status'], 2) & 0x5000 == 0x5000, 'readyLogData/readyID не готовы'
    m.call('user_send_registrationData', 0)
    m.call('user_send_statusPack', 0)
    m.call('user_send_statusPackToConfigurator', 0)
    assert [p[8] for p in sent] == [1, 2, 101]
    assert sent[1][13:15] == m.u.mem_read(device + d['status'], 2)
    assert sent[2][37:39] == struct.pack('<H', 3151)

    def main_poll():
        if mutation:
            del m.stubs[power]
            m.call('comm_power_v5_poll', m.read(millis))
            m.stubs[power] = m.ret
        m.call('comm_modem_poll', m.read(millis))
        if m.read(m.addr('power_state'), 1) == 6 and not ready_at:
            ready_at.append((m.read(millis) - origin) & 0xffffffff)

    main_poll()
    # После user_init главный цикл снова работает. До конца загрузки AT запрещены.
    target = 5000 if cold else 7000
    for _ in range(max(target, heavy) + 7000 - heavy):
        if m.read(m.addr('power_state'), 1) == 6: break
        tick()
        main_poll()
    finished = ready_at[0]
    diag = m.call('comm_modem_diag')
    power_ms = m.read(diag + fields('slm_diag_t')['phases'])
    expected = [(0, 13, False), (0, 14, True),
                (100 if cold else 2100, 13, True),
                (1000 if cold else 3000, 14, False),
                (3000 if cold else 5000, 14, True)]
    # GPIO - по таблице независимо от инициализации; USB 112 - до готовности главного цикла.
    assert trace == expected and finished == target and power_ms == max(target, heavy), ('питание', cold, heavy, trace, finished, power_ms)
    return {'POR': cold, 'init_ms': heavy, 'power_ms': finished, 'GPIO': trace}


def races():
    # SysTick на каждой границе start/off: состояние публикуется после отметки,
    # а выключение запрещает переход ещё до первого GPIO.
    for name in ('comm_power_v5_start', 'comm_power_v5_off'):
        m = Machine(); m.init()
        for stub in ('usb_timer1ms', 'uart_timer1ms', 'comm_uart_tick'):
            m.stubs[m.addr(stub)] = m.ret
        args = (m.addr('comm_gpio') | 1, 2000) if name.endswith('start') else ()
        m.call('comm_power_v5_start', m.addr('comm_gpio') | 1, 0)
        m.write(m.addr('milliSeconds'), 1999)
        m.call(name, *args); steps = m.steps
        for at in range(steps):
            m.init(); m.call('comm_power_v5_start', m.addr('comm_gpio') | 1, 0)
            m.write(m.addr('milliSeconds'), 1999)
            m.call(name, *args, inject_at=at, irq=('SysTick_Handler', ()))
            assert m.read(m.addr('power_state'), 1) == (1 if args else 0), (name, at)
            if args: assert m.read(m.addr('power_started')) == 2000
            else:
                m.call('SysTick_Handler')
                assert m.read(m.addr('power_state'), 1) == 0


rows = [startup(cold, n) for cold in (False, True) for n in (0, 2000, 5000)]
rows += [startup(cold, 5000, origin=0xfffff000) for cold in (False, True)]
rows += [startup(cold, 8000) for cold in (False, True)]
for cold in (False, True):
    try: startup(cold, 5000, mutation=True)
    except AssertionError as error:
        assert error.args[0][0] == 'питание', 'мутация должна нарушать срок/порядок GPIO'
    else: raise AssertionError('мутация питания только в главном цикле выжила')
races()
result_path('modem-startup.json').write_text(json.dumps(rows, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print('PASS IAR modem startup: init 0/2000/5000 ms, POR 5000/reset 7000, GPIO, counters/packets, wrap, IRQ races; 2 main-loop mutations killed')
