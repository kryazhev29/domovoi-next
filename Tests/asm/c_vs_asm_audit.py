"""Read-only ELF, object PUBLIC, map size and runtime-layout audit."""
import hashlib
import json
import os
from pathlib import Path
import re
import tempfile
from c_vs_asm_suite import ROOT, select_image
from c_vs_asm_compare import public_inventory
from elftools.elf.elffile import ELFFile
from iar_layout import fields

BASE=Path(os.environ.get('DOMOVOI_COMPARE_BUILD',Path(tempfile.gettempdir())/'domovoi-c-asm'))

def digest(data):return hashlib.sha256(data).hexdigest()

def audit():
    inventory=public_inventory();out={};symbolsets={}
    for variant in ('baseline','ASM','C','C_SPEED'):
        folder=BASE/'baseline/Release' if variant=='baseline' else select_image(variant)
        path=folder/'ExePath/DomovoiNext.out';mapping=(folder/'ListPath/DomovoiNext.map').read_text()
        with path.open('rb') as f:
            elf=ELFFile(f);syms={s.name:dict(s.entry) for s in elf.get_section_by_name('.symtab').iter_symbols()}
            segments=[{'address':s['p_paddr'],'bytes':s['p_filesz'],'sha256':digest(s.data())} for s in elf.iter_segments() if s['p_type']=='PT_LOAD']
            sections={s.name:digest(s.data()) for s in elf.iter_sections()}
            symbolsets[variant]=syms
        moduletext=mapping.split('*** MODULE SUMMARY',1)[1].split('*** ENTRY LIST',1)[0]
        head=next(l for l in moduletext.splitlines() if 'Module ' in l and 'ro code' in l)
        columns=[head.index(x) for x in ('ro code','rw code','ro data','rw data')]
        modules={}
        for line in moduletext.splitlines():
            m=re.match(r'\s+(\S+\.o)\s',line)
            if not m:continue
            values=[]
            for start in columns:
                cell=line[start:start+7].strip();values.append(int(cell.replace("'",'')) if cell else 0)
            modules[m[1]]=dict(zip(('ro_code','rw_code','ro_data','rw_data'),values))
        total={k:int(re.search(r"([\d']+) bytes of "+label,mapping)[1].replace("'",'')) for k,label in
               (('ro_code','readonly  code'),('rw_code','readwrite code'),('ro_data','readonly  data'),('rw_data','readwrite data'))}
        object_names=set();local_publics=[]
        if variant in ('ASM','C'):
            for stem in set(inventory.values()):
                paths=list((folder/'ObjPath').rglob(stem+'.o'));assert len(paths)==1,(stem,paths)
                with paths[0].open('rb') as f:
                    obj=ELFFile(f)
                    for s in obj.get_section_by_name('.symtab').iter_symbols():
                        if s['st_shndx']=='SHN_UNDEF':continue
                        object_names.add(s.name)
                        if inventory.get(s.name)==stem and s['st_info']['bind']=='STB_LOCAL':local_publics.append(s.name)
        layout={}
        if variant!='baseline':
            for typ in ('device_s','config_s','photo_s','cellID_s','cellGroup_s','cellShedule_s','cellHoliday_s','photoParam_s','bootFWparam_s','sj_runtime_state_s','photo_runtime_s'):
                layout[typ]=fields(typ)
        out[variant]={'elf_sha256':digest(path.read_bytes()),'load_segments':segments,'sections':sections,
            'map_totals':total,'modules':modules,'flash_payload_bytes':sum(s['bytes'] for s in segments),
            'flash_span_bytes':max(s['address']+s['bytes'] for s in segments)-0x08008200,
            'heap_top':syms['HEAP$$Limit']['st_value'],'fault_record':syms['fault_record']['st_value'],
            'ram_gap':syms['fault_record']['st_value']-syms['HEAP$$Limit']['st_value'],
            'layout':layout,'missing_object_publics':sorted(set(inventory)-object_names) if object_names else [],'local_publics':local_publics,
            'linked_publics':{n:{'address':s['st_value'],'size':s['st_size'],'type':s['st_info']['type']} for n,s in syms.items() if n in inventory}}
    out['checks']={
        'asm_load_equal':out['baseline']['load_segments']==out['ASM']['load_segments'],
        'asm_full_elf_equal':out['baseline']['elf_sha256']==out['ASM']['elf_sha256'],
        'asm_different_sections':[n for n,h in out['baseline']['sections'].items() if out['ASM']['sections'].get(n)!=h],
        'missing_linked_c_publics':sorted(set(out['ASM']['linked_publics'])-set(out['C']['linked_publics'])),
        'layout_equal':out['ASM']['layout']==out['C']['layout']==out['C_SPEED']['layout'],
        'ram_public_addresses_equal':all(s['address']==out['C']['linked_publics'][n]['address'] for n,s in out['ASM']['linked_publics'].items() if 0x20000000<=s['address']<0x20005000),
    }
    final={}
    for variant,config in (('C','Release'),('C_SPEED','ReleaseSpeed')):
        p=BASE/'final-validation'/('C-'+config)/'ExePath/DomovoiNext.out'
        if p.exists():
            with p.open('rb') as f:
                elf=ELFFile(f)
                segments=[{'address':s['p_paddr'],'bytes':s['p_filesz'],'sha256':digest(s.data())} for s in elf.iter_segments() if s['p_type']=='PT_LOAD']
            final[variant]={'elf_sha256':digest(p.read_bytes()),'same_load_segments':segments==out[variant]['load_segments']}
    out['final_source_rebuild']=final
    return out

if __name__=='__main__':
    result=audit();(BASE/'audit.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result['checks'],indent=2))
    for v in ('ASM','C','C_SPEED'):
        d=result[v];print(v,d['map_totals'],'Flash payload',d['flash_payload_bytes'],'gap',d['ram_gap'],'missing object PUBLIC',d['missing_object_publics'])
