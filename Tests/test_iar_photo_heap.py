# Доработка: Кряжев А., 2026
"""Реальный basic allocator из свежего IAR ELF в Unicorn, без платы."""
import struct
from pathlib import Path
import sys
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'Release/photo_test_deps'))
from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS
from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_XPSR, UC_ARM_REG_PC, UC_ARM_REG_R0
root=Path(__file__).resolve().parents[1]
with (root/'Release/Exe/DomovoiNext.out').open('rb') as f:
    elf=ELFFile(f)
    sym={s.name:s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols()}
    state=None
    for cu in elf.get_dwarf_info().iter_CUs():
        for die in cu.iter_DIEs():
            if die.attributes.get('DW_AT_name') and die.attributes['DW_AT_name'].value==b'photo_s':
                state=die.get_DIE_from_attribute('DW_AT_type').attributes['DW_AT_byte_size'].value
assert state is not None
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
pointers=[];sizes=[]
# Пик новой связи: состояние фото, самый большой ответ по USB (32 записи журнала, 1040)
# и блок фото для USB; копий UART/URC старого GSM больше нет.
for n in [state,1040,534]:
    p=call('__basic_malloc',n);assert p
    chunk=struct.unpack('<I',u.mem_read(p-8,4))[0]
    assert n<=chunk<=n+16,(n,p,chunk)
    pointers.append(p);sizes.append(chunk)
assert sum(sizes)<=4096-1024 # запас не меньше 1 КБ
print(f'IAR heap: photo_s={state}, chunks={sizes}, peak={sum(sizes)}/4096, free={4096-sum(sizes)}')
# Неудачное выделение не повреждает живые объекты и следующий вызов.
for i,p in enumerate(pointers):u.mem_write(p,bytes([0xA0+i])*16)
assert call('__basic_malloc',4096-sum(sizes))==0
for i,p in enumerate(pointers):assert bytes(u.mem_read(p,16))==bytes([0xA0+i])*16
call('__basic_free',pointers[-1]);assert call('__basic_malloc',534)
print('IAR allocation failure and retry: OK')
