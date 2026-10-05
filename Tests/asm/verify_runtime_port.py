"""Check every instruction of exported assembly against independent IAR objects."""
import hashlib,json,re
from pathlib import Path
from elftools.elf.elffile import ELFFile
from port_runtime import ROOT,MODULES

def sections(path):
    with path.open('rb') as f:
        e=ELFFile(f)
        return [(s.name,s.data()) for s in e.iter_sections()
                if s.name in ('.text','.rodata','.data','.bss')]

out={}
for n in MODULES:
    p=ROOT/'Release/task-asm-storage/export'
    a,b=sections(p/(n+'.o')),sections(p/(n+'_asm.o'))
    assert a==b, (n,'assembly differs from IAR oracle instructions/data')
    asm=(ROOT/'User/asm'/f'{n}_runtime.s').read_text()
    functions=sorted(set(re.findall(r'^\s*CFI Function\s+(\w+)',asm,re.M)))
    out[n]={'functions':functions,'instruction_data_bytes':sum(len(x[1]) for x in a),
            'assembly_sha256':hashlib.sha256(asm.encode()).hexdigest()}
(ROOT/'Tests/asm/runtime_inventory.json').write_text(json.dumps(out,indent=2)+'\n')
print('PASS all',sum(len(x['functions']) for x in out.values()),'functions:',
      'object instructions and data byte-identical; CFI retained')
