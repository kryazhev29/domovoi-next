# Доработка: Кряжев А., 2026
"""Команда 13 (синхронизация ID) и heap: реальный basic allocator IAR в Unicorn, без платы.

gsm_check_data копирует UART в calloc(count + 8). Команда 13 просит malloc(MEMSECTORSIZE = 4096)
под сектор, а её ответ уходит AT-командой, во время которой URC откладываются в heap (до 2056).
Порядок берётся из исходника: выполняется ли команда после free копии (server_rx_process после
free(buf)) и освобождается ли сектор до ответа. Кадр с N ID по 32 байта: тело N*32 + 8, кадр
тело + 8, перед ним заголовок модема (+QIURC: "recv",0,<len>) - с запасом 32 байта.
Требование: сектор и отложенный URC получают память для любого кадра, который принимает
server_rx_buffer (тело до 2040 байт = 63 ID). На master 89ac32c - отказ с 30 ID."""
import os, re, sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
deps=os.environ.get('DOMOVOI_TEST_DEPS') or str(root/'Release/photo_test_deps')
if not Path(deps).exists():
    deps=r'C:\Users\kryaz\Desktop\мк\СофтЭксперт\Для новой системы\Domovoi Next_IAR95\Domovoi Next_IAR95\Release\photo_test_deps'
sys.path.insert(0,deps)
from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_XPSR, UC_ARM_REG_PC, UC_ARM_REG_R0

user=(root/'Tests/asm/reference/User/user_fun.c').read_bytes().decode('cp1251')
if 'static bool sync_write_cells(' in user:
    import runpy
    runpy.run_path(str(root/'Tests/test_iar_sync_pages.py'))
    sys.exit(0)
check=user[user.index('void gsm_check_data'):]; check=check[:check.index('\n}')]
has_copy='calloc(' in check
has_stash='gsm_urcbuf' in (root/'Modules/gsm.c').read_bytes().decode('cp1251')
copy_freed_first=not has_copy or (re.search(r'free\(buf\);\s*server_rx_process\(\);',check) is not None and 'server_rx_defer = 1;' in check)
case13=user[user.index('\n    case 13:'):user.index('\n    case 14:')]
sector_freed_first=case13.index('free(membfr);')<case13.index('user_send_timeshtamp(')

with (root/'Release/Exe/DomovoiNext.out').open('rb') as f:
    sym={s.name:s['st_value'] for s in ELFFile(f).get_section_by_name('.symtab').iter_symbols()}
heap=sym['HEAP$$Limit']-sym['HEAP$$Base']
u=Uc(UC_ARCH_ARM,UC_MODE_THUMB|UC_MODE_MCLASS)
u.mem_map(0x08000000,0x20000);u.mem_write(0x08008200,(root/'Release/Exe/DomovoiNext.bin').read_bytes())
u.mem_map(0x20000000,0x5000)
stop=0x0801FFF0
u.reg_write(UC_ARM_REG_SP,sym['CSTACK$$Limit']);u.reg_write(UC_ARM_REG_XPSR,0x01000000)
def call(name,arg=0):
    u.reg_write(UC_ARM_REG_R0,arg);u.reg_write(UC_ARM_REG_LR,stop|1)
    u.emu_start(sym[name]|1,stop,count=2000000)
    assert u.reg_read(UC_ARM_REG_PC)==stop
    return u.reg_read(UC_ARM_REG_R0)
call('__iar_data_init3')
MODEM_HEADER=32
fail=None
for ids in range(1,64):
    frame=ids*32+16
    live=[]
    if has_copy:
        copy=call('__basic_malloc',frame+MODEM_HEADER+8);assert copy,ids
        if copy_freed_first:call('__basic_free',copy)
        else:live.append(copy)
    sector=call('__basic_malloc',4096)
    if not sector:fail=(ids,'сектор 4096')
    elif sector_freed_first:call('__basic_free',sector)
    else:live.append(sector)
    if not fail and has_stash:
        urc=call('__basic_malloc',2056)  # ответ уходит AT-командой, URC откладывается в heap
        if not urc:fail=(ids,'отложенный URC во время ответа')
        else:live.append(urc)
    for p in live:call('__basic_free',p)
    if fail:break
order=('команда после free копии' if copy_freed_first else 'команда при живой копии UART')+\
      (', сектор освобождён до ответа' if sector_freed_first else ', ответ при живом секторе')
print(f'IAR heap {heap} байт; {order}')
assert fail is None,f'команда 13 с {fail[0]}+ ID: не хватает heap на {fail[1]}'
print('cmd13 heap: сектор и отложенный URC получают память для любого кадра до 63 ID: OK')
