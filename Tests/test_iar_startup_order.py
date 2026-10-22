"""Порядок старта в ELF: собственные векторы, уровни GPIO и отложенный PVD."""
import argparse
import re

from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.arm_const import UC_ARM_REG_PC, UC_ARM_REG_PRIMASK
from iar_irq_support import Machine
from test_iar_peripheral_config import Registers

VTOR = 0xE000ED08
PWR_CR = 0x40007000


def vectors():
    for boot, expected in ((False, 0x08008200), (True, 0x08000000)):
        for inherited in (0x08000000, 0x08008200):
            m = Machine(boot=boot)
            Registers(m)
            m.write(VTOR, inherited)

            def clock_write(u, access, address, size, value, data):
                assert m.read(VTOR) == expected, 'RCC starts with wrong vectors'

            m.u.hook_add(UC_HOOK_MEM_WRITE, clock_write, begin=0x40021000, end=0x40021027)
            m.call('SystemInit')
            assert m.read(VTOR) == expected == m.addr('__vector_table')
    print('startup vectors: app/boot, cold/inherited VTOR PASS')


def gpio():
    for seed in (0, 0xFFFF):
        m = Machine()
        Registers(m)
        ports = ((0x40010800, 0x600D, 0x600C),
                 (0x40010C00, 0x51C8, 0x1000),
                 (0x40011000, 0x6000, 0x4000))
        for base, mask, expected in ports:
            m.write(base, 0x44444444)
            m.write(base + 4, 0x44444444)
            m.write(base + 12, seed)
        modes = []

        def mode_write(u, access, address, size, value, data):
            if address not in (base + offset for base, _, _ in ports for offset in (0, 4)):
                return
            for base, mask, expected in ports:
                assert m.read(base + 12) & mask == expected, ('unsafe GPIO before mode', hex(base))
            modes.append(address)

        m.u.hook_add(UC_HOOK_MEM_WRITE, mode_write)
        m.call('board_init')
        assert len(modes) == 5
    print('startup GPIO: safe latches before modes, zero/set inherited ODR PASS')


def startup():
    for low in (False, True):
        m = Machine()
        Registers(m)
        m.write(VTOR, 0x08008200)
        seen = set()
        resets = []

        def reset():
            assert m.read(0x40006C04, 2) == 0x5056, 'PVD marker missing before reset'
            resets.append(1)
            m.ret()

        for value in re.findall(r"^NVIC_SystemReset\s+0x([0-9a-f']+)", m.maptext, re.M):
            m.stubs[int(value.replace("'", ''), 16) & ~1] = reset

        checkpoints = {m.addr(name): name for name in
                       ('__iar_program_start', 'stack_paint', 'board_init', 'timers_init')}

        def checkpoint(u, address, size, data):
            name = checkpoints.get(address)
            if name is None:
                return
            seen.add(name)
            assert m.read(VTOR) == 0x08008200, ('wrong startup vectors', name)
            if name in ('stack_paint', 'board_init'):
                assert u.reg_read(UC_ARM_REG_PRIMASK) == 1, ('IRQ open', name)
                assert m.read(0xE000ED24) & 0x70000 == 0x70000
            if name == 'timers_init':
                assert u.reg_read(UC_ARM_REG_PRIMASK) == 0, 'IRQ left closed'
                assert m.read(m.bottom) == 0x5A5AA5A5
                u.reg_write(UC_ARM_REG_PC, m.stop)

        def detector(u, access, address, size, value, data):
            if value & 0x10:
                assert u.reg_read(UC_ARM_REG_PRIMASK) == 1, 'PVD enabled with IRQ open'
                assert m.read(m.read(VTOR) + 17 * 4) & ~1 == m.addr('PVD_IRQHandler')
                if low:
                    m.write(PWR_CR + 4, 4)
                    m.write(0x40010414, 1 << 16)
                    m.pending = True

        m.u.hook_add(UC_HOOK_CODE, checkpoint)
        m.u.hook_add(UC_HOOK_MEM_WRITE, detector, begin=PWR_CR, end=PWR_CR)
        m.call('Reset_Handler', irq=('PVD_IRQHandler', ()))
        assert seen == set(checkpoints.values())
        assert len(resets) == int(low)
        for slot, name in ((3, 'HardFault_Handler'), (4, 'MemManage_Handler'),
                           (5, 'BusFault_Handler'), (6, 'UsageFault_Handler')):
            assert m.read(m.read(VTOR) + slot * 4) & ~1 == m.addr(name)
    print('startup reset to timers: masked paint/board, PVD marker on low power PASS')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--case', choices=('vectors', 'gpio', 'startup', 'all'), default='all')
    case = parser.parse_args().case
    for name, test in (('vectors', vectors), ('gpio', gpio), ('startup', startup)):
        if case in (name, 'all'):
            test()
