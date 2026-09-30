# Доработка: Кряжев А., 2026
"""Команда 13 (синхронизация ID) и heap: реальный basic allocator IAR в Unicorn, без платы.

Порядок выделений как на плате: gsm_check_data держит calloc(count + 8) с копией UART, пока
разбирается кадр; команда 13 внутри разбора просит malloc(MEMSECTORSIZE = 4096) под сектор.
Кадр с N ID по 32 байта: тело N*32 + 8, кадр тело + 8; в UART перед кадром заголовок модема
(+QIURC: "recv",0,<len> и перевод строки), берётся с запасом 32 байта.
Тест требует, чтобы сектор выделялся для любого кадра, который принимает server_rx_buffer
(тело до 2040 байт = 63 ID). На master 89ac32c ожидается отказ."""
import os, struct, sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
deps=os.environ.get('DOMOVOI_TEST_DEPS') or str(root/'Release/photo_test_deps')
if not Path(deps).exists():
    deps=r'C:\Users\kryaz\Desktop\мк\СофтЭксперт\Для новой системы\Domovoi Next_IAR95\Domovoi Next_IAR95\Release\photo_test_deps'
sys.path.insert(0,deps)
from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_XPSR, UC_ARM_REG_PC, UC_ARM_REG_R0
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
worst=None
for ids in range(1,64):
    frame=ids*32+16
    copy=call('__basic_malloc',frame+MODEM_HEADER+8);assert copy,ids
    sector=call('__basic_malloc',4096)
    if sector:call('__basic_free',sector)
    call('__basic_free',copy)
    if not sector and worst is None:worst=ids
print(f'IAR heap {heap} байт: команда 13 получает сектор 4096 до {(worst or 64)-1} ID в кадре'
      + (f', с {worst} ID (кадр {worst*32+16} байт) - отказ RUNERROR' if worst else ''))
assert worst is None,f'команда 13 с {worst}+ ID не получает память под сектор (heap {heap})'
print('cmd13 heap: сектор выделяется для любого кадра до 63 ID: OK')
