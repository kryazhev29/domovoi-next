# Доработка: Кряжев А., 2026
"""Замер стека (команда 111, поле 57): stack_paint и stack_free_bytes из свежего IAR ELF в Unicorn."""
import os, sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
deps=os.environ.get('DOMOVOI_TEST_DEPS') or str(root/'Release/photo_test_deps')
if not Path(deps).exists():
    deps=r'C:\Users\kryaz\Desktop\мк\СофтЭксперт\Для новой системы\Domovoi Next_IAR95\Domovoi Next_IAR95\Release\photo_test_deps'
sys.path.insert(0,deps)
from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_XPSR, UC_ARM_REG_R0
with (root/'Release/Exe/DomovoiNext.out').open('rb') as f:
    sym={s.name:s['st_value'] for s in ELFFile(f).get_section_by_name('.symtab').iter_symbols()}
u=Uc(UC_ARCH_ARM,UC_MODE_THUMB|UC_MODE_MCLASS)
u.mem_map(0x08000000,0x20000);u.mem_write(0x08008200,(root/'Release/Exe/DomovoiNext.bin').read_bytes())
u.mem_map(0x20000000,0x5000)
stop=0x0801FFF0;base=sym['CSTACK$$Base'];lim=sym['CSTACK$$Limit'];size=lim-base
def call(n):
    u.reg_write(UC_ARM_REG_SP,lim-8);u.reg_write(UC_ARM_REG_XPSR,0x01000000);u.reg_write(UC_ARM_REG_LR,stop|1)
    u.emu_start(sym[n]|1,stop,count=100000);return u.reg_read(UC_ARM_REG_R0)
call('stack_paint');free=call('stack_free_bytes')
assert size-128<=free<=size-64,(size,free)
u.mem_write(base+1000,b'\0\0\0\0');assert call('stack_free_bytes')==1000
u.mem_write(base,b'\0\0\0\0');assert call('stack_free_bytes')==0
print(f'stack mark: стек {size}, после заполнения свободно {free}, запись на +1000 -> 1000, на дне -> 0: OK')
