"""Доработка: Кряжев А., 2026. Заполнение стека и диагностический замер выбранного ELF."""
import os
from iar_image import image_paths
from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_XPSR, UC_ARM_REG_R0

with image_paths()[0].open('rb') as f:
    sym={s.name:s['st_value'] for s in ELFFile(f).get_section_by_name('.symtab').iter_symbols()}
u=Uc(UC_ARCH_ARM,UC_MODE_THUMB|UC_MODE_MCLASS)
u.mem_map(0x08000000,0x20000);u.mem_write(0x08008200,image_paths()[1].read_bytes())
u.mem_map(0x20000000,0x5000)
stop=0x0801FFF0;base=sym['CSTACK$$Base'];lim=sym['CSTACK$$Limit'];size=lim-base


def call(n):
    u.reg_write(UC_ARM_REG_SP,lim-8);u.reg_write(UC_ARM_REG_XPSR,0x01000000);u.reg_write(UC_ARM_REG_LR,stop|1)
    u.emu_start(sym[n]|1,stop,count=100000);return u.reg_read(UC_ARM_REG_R0)


diagnostic = os.environ.get('DOMOVOI_COMPARE_CONFIG', 'Release') != 'ReleaseCustomer'
assert ('stack_free_bytes' in sym) == diagnostic
call('stack_paint')
painted = 0
while bytes(u.mem_read(base+painted,4)) == b'\xa5\xa5\x5a\x5a': painted += 4
assert size-128<=painted<=size-64,(size,painted)
if diagnostic:
    assert call('stack_free_bytes') == painted
    u.mem_write(base+1000,b'\0\0\0\0');assert call('stack_free_bytes')==1000
    u.mem_write(base,b'\0\0\0\0');assert call('stack_free_bytes')==0
print(f'stack mark: стек {size}, заполнено {painted}, диагностический замер {diagnostic}: OK')
