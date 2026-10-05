"""Collect final linked resources and ensure production never compiles oracles."""
import hashlib,json,re,xml.etree.ElementTree as ET
from storage_machine import *
from port_runtime import MODULES

project=ET.parse(ROOT/'DomovoiNext.ewp').getroot()
files=[f.findtext('name','').replace('\\','/') for f in project.findall('.//file')]
assert not any('Tests/' in f for f in files)
for module in MODULES:
    assert any(f.endswith('/User/asm/'+module+'_runtime.s') for f in files),module
    assert not any(f.endswith('/'+module+'.c') for f in files),module
m=StorageMachine();chunks=[];live=[]
for n in (180,1040,534,2016):
    p=m.checked('__basic_malloc',n);assert p;live.append(p)
    chunks.append(int.from_bytes(m.u.mem_read(p-8,4),'little'))
# IAR chunk size is independently cross-checked by adjacent live allocations.
assert [live[i+1]-live[i] for i in range(3)]==chunks[:3],(live,chunks)
assert sum(chunks)==3808,chunks
text=(ROOT/'Release/List/DomovoiNext.map').read_text(errors='replace')
values={}
for qualifier,kind in (('readonly','code'),('readonly','data'),('readwrite','code'),('readwrite','data')):
    value=re.search(r"([\d']+) bytes of "+qualifier+r'\s+'+kind+r' memory',text)
    values[qualifier+'_'+kind]=int(value[1].replace("'",''))
assert m.addr('__vector_table')==0x08008200
assert m.addr('HEAP$$Limit')<=m.addr('fault_record')==0x20004fc0
out={'units':'bytes; stack measurements exclude modeled peripheral calls',
     'elf_sha256':hashlib.sha256((ROOT/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest(),
     'bin_sha256':hashlib.sha256((ROOT/'Release/Exe/DomovoiNext.bin').read_bytes()).hexdigest(),
     'boot_elf_sha256':hashlib.sha256((ROOT/'Bootloader/build/Release/Exe/DomovoiBoot.out').read_bytes()).hexdigest(),
     'bin_bytes':(ROOT/'Release/Exe/DomovoiNext.bin').stat().st_size,'map':values,
     'heap':m.heap_bytes,'stack':m.top-m.bottom,'heap_chunks':chunks,'heap_peak':sum(chunks),
     'static_gap':m.addr('fault_record')-m.addr('HEAP$$Limit'),
     'stack_analysis':{'main_estimate':2360,'IRQ_estimate':256,'warning':'Ls016: not a proof of global maximum'},
     'prior_memory_model':json.loads((ROOT/'Release/audit-memory.json').read_text(encoding='utf-8'))}
(ROOT/'Tests/asm/runtime_final_resources.json').write_text(json.dumps(out,indent=2)+'\n')
print('PASS final ELF/resources:',out['bin_bytes'],values,'heap',chunks,'gap',out['static_gap'])
