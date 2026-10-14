"""Оценка ELF до запуска питания, не физический замер платы/загрузчика.

Выполняет main и user_init с моделью пустой NOR. Периферийные init, USB и
ожидание delay_ms заменены границами: известные 500 мс учитываются отдельно,
USB отдельно исполняется без заглушек. Число инструкций и NOR-чтений до power_start.
"""
import json
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent / 'asm'))
from storage_machine import StorageMachine
from protocol_contract import Profile
from iar_image import result_path
from iar_irq_support import Machine


class PrefixDone(Exception):
    pass


def measure(usb):
    machine = StorageMachine()
    profile = Profile(machine, {})
    reads, elapsed, usb_calls = [], [0], []

    def read():
        address, pointer, length, unused = machine.args()
        reads.append((address, length))
        assert length and address + length <= len(machine.flash)
        machine.u.mem_write(pointer, bytes(machine.flash[address:address + length]))
        machine.ret(1)

    def delay():
        elapsed[0] += machine.args()[0]
        machine.write(machine.addr('milliSeconds'), elapsed[0])
        machine.ret()

    def stop():
        raise PrefixDone()

    machine.stubs[machine.addr('w25_read_data')] = read
    machine.stubs[machine.addr('timers_pause')] = delay
    machine.stubs[machine.addr('comm_power_v5_start')] = stop
    machine.stubs[machine.addr('usb_init')] = lambda: (usb_calls.append(elapsed[0]), machine.ret())
    for name in ('stack_paint', 'board_init', 'timers_init', 'spi_init', 'adc_init', 'wdt_init', 'wdt_reload'):
        machine.stubs[machine.addr(name)] = machine.ret
    machine.write(0x40010c08, 0x8000 if usb else 0)
    machine.write(0x40021024, 1 << 27)
    try:
        profile.enabled = True
        machine.call('main', budget=30000000)
    except PrefixDone:
        pass
    else:
        raise AssertionError('Питание модема не начато')
    finally:
        profile.enabled = False
        profile.close()
    # Профиль считает инструкции и штраф выборки, а не такты реальной платы.
    payload = sum(n for _, n in reads)
    wire = (payload + 6 * len(reads)) * 8 / 18000  # SPI1 72 MHz / 4, миллисекунды.
    return {'USB_connected': usb, 'fixed_delay_ms': elapsed[0], 'USB_before_power': len(usb_calls),
            'instructions': profile.n, 'fetch_estimate': profile.n + profile.wait,
            'core_estimate_ms_72MHz': round((profile.n + profile.wait) / 72000, 3),
            'NOR_reads': len(reads), 'NOR_bytes': payload, 'NOR_wire_ms_18MHz': round(wire, 3),
            'limit': 'До вызова power_start; без времени загрузчика, периферии, USB, erase/program/BUSY NOR и стабилизации VBAT; физический PC13 high наступает позже по таблице питания.'}


rows = [measure(usb) for usb in (False, True)]
assert rows[0]['USB_before_power'] == 0 and rows[1]['USB_before_power'] == 1
# Полный синхронный usb_init: без заглушек, USB RESET/IRQ и хост не моделируются.
usb = Machine(); usb.init(); usb.u.mem_map(0x42000000, 0x600000)
usb.u.mem_map(0x1ffff000, 0x1000)  # Уникальный номер MCU, читаемый Get_SerialNum.
for address in (0x1ffff7e8, 0x1ffff7ec, 0x1ffff7f0): usb.write(address, 0x1234abcd)
usb.write(usb.addr('SystemCoreClock'), 72000000)
profile = Profile(usb, {})
profile.run(lambda: usb.call('usb_init', budget=100000))
usb_cost = dict(profile.last)
usb_cost['estimate_ms_72MHz'] = round(usb_cost['fetch_estimate'] / 72000, 6)
assert usb_cost['estimate_ms_72MHz'] < 2, usb_cost
result_path('usb-init-measure.json').write_text(json.dumps(usb_cost, indent=2) + '\n', encoding='utf-8')
result_path('startup-prefix.json').write_text(json.dumps(rows, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print(json.dumps(rows, ensure_ascii=False))
print('USB init:', usb_cost)
