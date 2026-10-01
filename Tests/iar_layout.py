"""Смещения полей из DWARF текущего IAR, без ручной копии структуры."""
from iar_irq_support import ROOT, ELFFile, DWARFExprParser

def fields(name, boot=False):
    image=ROOT/('Bootloader/build/Release/Exe/DomovoiBoot.out' if boot else 'Release/Exe/DomovoiNext.out')
    with image.open('rb') as f:
        dwarf=ELFFile(f).get_dwarf_info()
        for cu in dwarf.iter_CUs():
            for die in cu.iter_DIEs():
                a=die.attributes.get('DW_AT_name')
                if die.tag!='DW_TAG_typedef' or not a or a.value!=name.encode():continue
                out={}
                for member in die.get_DIE_from_attribute('DW_AT_type').iter_children():
                    if 'DW_AT_name' not in member.attributes:continue
                    off=member.attributes['DW_AT_data_member_location'].value
                    if not isinstance(off,int):off=DWARFExprParser(cu.structs).parse_expr(off)[0].args[0]
                    out[member.attributes['DW_AT_name'].value.decode()]=off
                return out
    raise AssertionError(name)
