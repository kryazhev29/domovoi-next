"""Reproducible IAR Thumb-2 assembly export. References are NEVER production inputs.

Normal IAR builds assemble the checked-in .s files. This explicit developer tool
is the only operation that reads the independent C specifications. No hardware.
"""
import argparse, hashlib, json, re, subprocess
from pathlib import Path
from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parents[2]
IAR = Path(r'C:\Program Files\IAR Systems\Embedded Workbench 9.2\arm')
MODULES = ['user_fun', 'flash_w25', 'memory', 'photo', 'crc', 'idindex', 'user_comm', 'sector_runtime',
           'spi_stm32f10x', 'usb_stm32f10x', 'sector_journal']
INCLUDES = ['CMSIS/Include', 'CMSIS/STM32F10x/Include', 'Modules',
            'STM32F10x_StdPeriph_Driver/inc', 'User', '.',
            'STM32_USB-FS-Device_Driver/inc', 'USB-FS-CDC_User/Inc', 'Tests/asm/reference/User']
FLAGS = ['--no_cse', '--no_unroll', '--no_inline', '--no_code_motion',
         '--no_tbaa', '--no_clustering', '--no_scheduling', '--debug',
         '--endian=little', '--cpu=Cortex-M3', '-e', '--fpu=None', '-Ohz',
         '--dlib_config', str(IAR/'inc/c/DLib_Config_Normal.h')]
FLAGS += ['-D'+d for d in ('ASM_RUNTIME','ASM_CRC','ASM_ID_HASH','ASM_FAST_KERNELS',
                          'STM32F10X_MD','USE_STDPERIPH_DRIVER')]
FLAGS += [x for d in INCLUDES for x in ('-I',str(ROOT/d))]

def run(args):
    p = subprocess.run([str(x) for x in args], cwd=ROOT, capture_output=True)
    if p.returncode:
        raise RuntimeError(p.stdout.decode(errors='replace')+p.stderr.decode(errors='replace'))

def export(name):
    out = ROOT/'Release/task-asm-storage/export'; out.mkdir(parents=True,exist_ok=True)
    folder = 'Modules' if name in ('spi_stm32f10x','usb_stm32f10x') else 'User'
    ref = ROOT/'Tests/asm/reference'/folder/f'{name}.c'
    source = ref if ref.exists() else ROOT/folder/f'{name}.c'
    # Preserve include search and original relative include semantics.
    temp = ROOT/folder/f'_asm_export_{name}.c'
    obj, listing = out/f'{name}.o', out/f'{name}.s'
    try:
        temp.write_bytes(source.read_bytes())
        run([IAR/'bin/iccarm.exe', temp, *FLAGS, '-o',obj,'-la',listing])
    finally:
        temp.unlink(missing_ok=True)
    asm = listing.read_text(encoding='utf-8-sig')
    # IAR's C listing prints PUBLIC even for __weak definitions. Carry the
    # binding from its object so the real callback overrides the weak default.
    with obj.open('rb') as f:
        weak=[s.name for s in ELFFile(f).get_section_by_name('.symtab').iter_symbols()
              if s['st_info']['bind']=='STB_WEAK' and s['st_shndx']!='SHN_UNDEF']
    for symbol in weak:
        asm=re.sub(r'(?m)^(\s*)PUBLIC\s+'+re.escape(symbol)+r'\s*$',r'\1PUBWEAK '+symbol,asm)
    # Drop timestamp/source-path comments; keep instructions, data and all CFI.
    asm = '\n'.join(l.rstrip() for l in asm.splitlines() if not l.lstrip().startswith((';','//')))+'\n'
    # IAR's listing puts FunCall inside an IT block; its standalone assembler
    # rejects directives there. Same CFI state, immediately before the IT.
    asm = re.sub(r'(        IT[^\n]*\n)(\s+CFI (?:FunCall|TailCall)[^\n]*\n)',r'\2\1',asm)
    lines=asm.splitlines(); fixed=[]; i=0
    while i<len(lines):
        match=re.match(r'\s+IT([TE]*)\s+\w+\s*$',lines[i])
        if not match: fixed.append(lines[i]); i+=1; continue
        block=[lines[i]]; cfi=[]; left=1+len(match[1]); i+=1
        while left:
            line=lines[i]; i+=1
            if re.match(r'\s+CFI (FunCall|TailCall)\b',line): cfi.append(line)
            else:
                block.append(line)
                if line.strip(): left-=1
        fixed.extend(cfi+block)
    asm='\n'.join(fixed)+'\n'
    # Listing includes a caller argument which standalone IASM does not accept
    # on conditional IndirectCall. The enclosing CFI Function carries it.
    asm=re.sub(r'(CFI\s+\([^\n]+\)\s+IndirectCall)\s+\w+',r'\1',asm)
    # Preserve named state visibility for ELF ABI/ownership tests. IAR omits
    # local assembler data labels from the linked symbol table otherwise.
    functions=set(re.findall(r'^\s*CFI Function\s+(\w+)',asm,re.M))
    public=set(re.findall(r'^\s*PUBLIC\s+(\w+)',asm,re.M))
    labels=set(re.findall(r'^(\w+):',asm,re.M))
    data=sorted(labels-functions-public)
    asm=''.join('        PUBLIC '+n+'\n' for n in data)+asm
    asm = re.sub(r'\n{3,}', '\n\n', asm)
    target = ROOT/'User/asm'/f'{name}_runtime.s'
    target.write_text('; Cortex-M3 / IAR. Executable production module.\n'
                      '; Reference: Tests/asm/reference/'+folder+'/'+name+'.c\n'+asm,encoding='utf-8')
    # Keep the reference object (DWARF/type and independent machine-code oracle).
    run([IAR/'bin/iasmarm.exe',target,'--cpu','Cortex-M3','-o',out/f'{name}_asm.o'])
    return {'reference_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
            'assembly_sha256':hashlib.sha256(target.read_bytes()).hexdigest(),
            'functions':re.findall(r'^\s*CFI Function\s+(\S+)',asm,re.M)}

if __name__ == '__main__':
    p=argparse.ArgumentParser();p.add_argument('modules',nargs='*');a=p.parse_args()
    result={n:export(n) for n in (a.modules or MODULES)}
    (ROOT/'Release/task-asm-storage/export/manifest.json').write_text(json.dumps(result,indent=2)+'\n')
    print('Export/assemble PASS:', ', '.join(result))
