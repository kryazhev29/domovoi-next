"""Защитное слово стека и остановка FLT1/STAK без стека в собранном образе."""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from iar_irq_support import Machine, UC_ARM_REG_PRIMASK, UC_ARM_REG_PC, UC_ARM_REG_SP, UC_ARM_REG_LR
from unicorn import UC_HOOK_MEM_WRITE


def run():
    m = Machine(); m.init(); m.call('stack_paint')
    before = bytes(m.u.mem_read(m.addr('fault_record'), 48))
    m.call('stack_canary_check')
    healthy_steps = m.steps
    assert m.minimum == m.top and m.u.reg_read(UC_ARM_REG_PC) == m.stop
    assert bytes(m.u.mem_read(m.addr('fault_record'), 48)) == before
    for damage in (0, 0xffffffff, 0x5a5aa5a4):
        m = Machine(); m.init(); m.write(m.bottom, damage)
        writes = []
        m.u.hook_add(UC_HOOK_MEM_WRITE,
                     lambda u, access, p, n, value, _: writes.append((p, n)))
        m.minimum = m.top
        m.u.reg_write(UC_ARM_REG_SP, m.top); m.u.reg_write(UC_ARM_REG_LR, m.stop | 1)
        m.u.emu_start(m.addr('stack_canary_check') | 1, m.stop, count=200)
        record = m.addr('fault_record')
        assert m.read(record) == 0x31544c46 and m.read(record + 4) == 0x5354414b
        assert m.read(record + 12) == m.top and not m.read(record + 44)
        assert m.u.reg_read(UC_ARM_REG_PRIMASK) == 1 and m.minimum == m.top
        assert all(record <= p < p + n <= record + 48 for p, n in writes)
        pc = m.u.reg_read(UC_ARM_REG_PC)
        assert bytes(m.u.mem_read(pc, 2)) == b'\xfe\xe7', 'failure did not stop'
        assert m.u.reg_read(UC_ARM_REG_SP) == m.top
    return {'healthy_instructions': healthy_steps, 'failure_cases': 3,
            'stack_bytes_used': 0, 'reset': 'existing IWDG stop; no board timing claim'}


if __name__ == '__main__':
    print('PASS stack canary', run())
