"""Причина старта: настоящий user_init и очередь журнала из IAR ELF."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from iar_irq_support import Machine
from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_READ

CSR, MARKER = 0x40021024, 0x40006C04
RMVF, PIN, POR, SFT, IWDG, WWDG, LPWR = (1 << n for n in (24, 26, 27, 28, 29, 30, 31))
PVD_MARK = 0x5056


def boot(flags, marker=0, full=False, clear_after_sample=False):
    m = Machine(); m.init()
    m.write(CSR, flags | 3)  # LSION/LSIRDY должны сохраниться при RMVF.
    m.write(MARKER, marker, 2)
    if full:
        m.write(m.addr('log_pendingCount'), 16, 1)
    reads, writes, attempts = [], [], []

    def watch(u, access, address, size, value, data):
        if address == CSR:
            assert size == 4
            (reads if access == UC_MEM_READ else writes).append(value)

    # Проверяем снимок до первого вызова восстановления и его независимость от RCC.
    def memory_id():
        assert len(reads) == 1, 'CSR не прочитан один раз до восстановления'
        if clear_after_sample:
            m.write(CSR, 3)
        m.ret()

    for name in ('mem_recover', 'user_readconfig', 'user_initvariables', 'sched16_boot',
                 'user_unblock_recheck', 'app_boot_metadata', 'rs485_init', 'uart_setRecTimeout'):
        m.stubs[m.addr(name)] = m.ret
    m.stubs[m.addr('w25_read_devID')] = memory_id
    def sync_write():
        raise AssertionError('стартовое событие должно попасть в очередь журнала')

    m.stubs[m.addr('log_addEvent')] = sync_write
    log_calls = {m.addr('log_addSystemEvent'), m.addr('log_addSystemValue')}
    m.u.hook_add(UC_HOOK_CODE, lambda u, a, n, data: attempts.append(a) if a in log_calls else None)
    # Общий MEM_WRITE ломает цикл IAR memset в Unicorn; следим только за CSR.
    m.u.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, watch, begin=CSR, end=CSR + 3)
    m.call('user_init')
    return m, reads, writes, attempts


def run():
    cases = ((0, None), (POR, None), (PIN, None), (POR | PIN, None),
             (IWDG, 40), (IWDG | POR | PIN, 40), (WWDG, 40), (SFT, 40),
             (LPWR, 40), (IWDG | WWDG | SFT | LPWR | POR | PIN, 40))
    for flags, event in cases:
        m, reads, writes, attempts = boot(flags, marker=0x1234)
        count = m.read(m.addr('log_pendingCount'), 1)
        assert count == int(event is not None), (hex(flags), count, 'лишнее/потерянное событие')
        assert len(attempts) == count, 'событие пробовали записать больше одного раза'
        assert writes == [flags | 3 | RMVF] and len(reads) == 2, 'RMVF не записан'
        assert m.read(MARKER, 2) == 0x1234, 'чужая BKP-метка изменена'
        if event:
            record = bytes(m.u.mem_read(m.addr('log_pending'), 32))
            assert record[8] == event
            assert record[9:19] == str(flags >> 24).encode().ljust(10, b'\0'), 'аргумент 1 неверен'
            assert record[19:] == bytes(13), 'остальные аргументы должны быть нулевыми'

    # PVD подавляет 40 даже при нескольких флагах; 39 снимает свою метку.
    for full in (False, True):
        m, reads, writes, attempts = boot(SFT | IWDG | PIN, PVD_MARK, full)
        assert len(attempts) == 1
        assert m.read(MARKER, 2) == (PVD_MARK if full else 0)
        assert len(reads) == (1 if full else 2)
        assert writes == ([] if full else [SFT | IWDG | PIN | 3 | RMVF])
        if not full:
            assert m.read(m.addr('log_pendingCount'), 1) == 1
            record = bytes(m.u.mem_read(m.addr('log_pending'), 32))
            assert record[8] == 39 and record[9:] == bytes(23), 'PVD должен писать только 39'

    m, reads, writes, attempts = boot(IWDG, full=True)
    assert len(attempts) == 1 and len(reads) == 1 and not writes, 'отказ журнала потерял причину'
    assert m.read(CSR) == IWDG | 3 and m.read(m.addr('log_pendingCount'), 1) == 16
    m, reads, writes, attempts = boot(IWDG | PIN, clear_after_sample=True)
    record = bytes(m.u.mem_read(m.addr('log_pending'), 32))
    assert record[8] == 40 and record[9:19] == b'36'.ljust(10, b'\0'), 'использован поздний CSR'
    assert writes == [3 | RMVF]
    return {'reset_cases': len(cases), 'pvd_cases': 2, 'failed_insertions': 2, 'early_snapshot': True}


def mutations():
    from test_flash_mutations import run as run_mutations
    check = ('else if (reset_csr & (RCC_CSR_IWDGRSTF | RCC_CSR_WWDGRSTF |\n'
             '                          RCC_CSR_SFTRSTF | RCC_CSR_LPWRRSTF))')
    return run_mutations({'always-log': ('run', [('User/c/user_main.c', check, 'else')])},
                         'test_resetcause', 'resetcause-mutations')


if __name__ == '__main__':
    print('PASS reset cause', run())
