import pathlib, subprocess, sys, tempfile, re
ROOT=pathlib.Path(__file__).resolve().parents[1]
ZIG=r'C:\Users\kryaz\Documents\Codex\2026-09-14\c-users-kryaz-desktop-domovoi-next\work\toolchain\ziglang\zig.exe'
ref=sys.argv[1] if len(sys.argv)>1 else None
kind=sys.argv[2] if len(sys.argv)>2 else 'storage'
def source(name):
    return (subprocess.check_output(['git','show',ref+':'+name],cwd=ROOT) if ref else (ROOT/name).read_bytes()).decode('cp1251')
def function(text,name):
    m=re.search(r'(?m)^(?:static )?(?:void|uint8_t|uint16_t|uint32_t)\s+'+name+r'\s*\([^;]*?\)\s*\{',text)
    assert m,name
    depth=1;i=m.end()
    while depth:
        if text[i]=='{':depth+=1
        elif text[i]=='}':depth-=1
        i+=1
    return text[m.start():i]
with tempfile.TemporaryDirectory(prefix='domovoi-host-') as d:
    d=pathlib.Path(d)
    if kind=='storage':
        # Check physical erase-sector footprints, not just logical byte ranges.
        mapping=source('User/user_fun.c')
        ranges=[]
        for name,start,size in re.findall(r'mem\[(MB_\w+)\]\.start = (\d+);\s*mem\[\1\]\.size = (\d+);',mapping):
            start,size=int(start),int(size)
            if size:ranges.append((name,start&~4095,(start+size+4095)&~4095))
        for lo,hi in [(0x1f5000,0x1f7000),(0x1f7000,0x1f9000)]:
            assert lo>=0x41000 and hi<=0x200000
            for name,a,b in ranges:assert hi<=a or lo>=b,(name,hex(a),hex(b))
        print('Flash map: both slots outside all rounded mem[] ranges and 0x00000..0x40FFF',flush=True)
        code=source('User/memory.c').replace('#include "memory.h"','')
        header=source('User/memory.h')
        defines='\n'.join(x for x in header.splitlines() if x.startswith('#define MEM'))
        test=(ROOT/'Tests/test_storage.c').read_text()
        unit=test.replace('/* PRODUCTION */',defines+'\n'+code+'\n'+function(source('User/user_fun.c'),'user_writeConfigAndDevice'))
    else:
        raise ValueError(kind)
    (d/'test.c').write_text(unit)
    subprocess.run([ZIG,'cc','-std=c99','-O2','-Wall','-Wextra','-Werror',str(d/'test.c'),'-o',str(d/'test.exe')],check=True)
    result=subprocess.run([str(d/'test.exe')]+sys.argv[3:],timeout=300)
    sys.exit(result.returncode)
