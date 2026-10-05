"""Смещения полей из DWARF текущего IAR, без ручной копии структуры."""
from iar_irq_support import ROOT, ELFFile, DWARFExprParser
from iar_image import image_paths
from functools import lru_cache

def fields(name, boot=False):
    image=image_paths(boot)[0]
    return dict(_fields(str(image), image.stat().st_mtime_ns, name))

@lru_cache(None)
def _fields(path, version, name):
    from pathlib import Path
    image=Path(path)
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
