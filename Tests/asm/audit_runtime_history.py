"""Inventory the C functions changed in the requested optimization/storage history."""
import json,re,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
COMMITS=['ddab825','233244a','cbcf73f','73d0855','4b5f943','3cc3149','174d940',
         '63440f8','43f7053','13f18b3','3450335']
def git(*args):return subprocess.check_output(['git',*args],cwd=ROOT).decode('cp1251')
inventory=json.loads((ROOT/'Tests/asm/runtime_inventory.json').read_text())
out=[]
for commit in COMMITS:
    files=git('diff-tree','--no-commit-id','--name-only','-r',commit).splitlines()
    entry={'commit':git('rev-parse',commit).strip(),'changed_functions':{}}
    for path in files:
        if not path.endswith('.c') or path.startswith('Tests/'):continue
        source=git('show',commit+':'+path)
        clean=re.sub(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\x27(?:\\.|[^\x27\\])*\x27',
                     lambda m:re.sub(r'[^\n]',' ',m[0]),source)
        spans=[]
        pattern=r'(?m)^[ \t]*(?:(?:[A-Za-z_]\w*|\*)[ \t]+)+([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{'
        for m in re.finditer(pattern,clean):
            if m[1] in ('if','for','while','switch'):continue
            pos=m.end();depth=1
            while depth and pos<len(clean):
                depth+=(clean[pos]=='{')-(clean[pos]=='}');pos+=1
            spans.append((source.count('\n',0,m.start())+1,source.count('\n',0,pos)+1,m[1]))
        diff=git('diff',commit+'^',commit,'--unified=0','--',path)
        ranges=[(int(a),int(n or 1)) for a,n in re.findall(r'@@ -[^+]+\+(\d+)(?:,(\d+))? @@',diff)]
        names=sorted({name for lo,hi,name in spans for a,n in ranges if n and a<=hi and a+n>lo})
        if not names:continue
        module=Path(path).stem
        assert module in inventory or path=='main.c',(commit,path,'unported changed functions',names)
        entry['changed_functions'][path]={'functions':names,'production':
          'User/asm/'+module+'_runtime.s' if module in inventory else 'existing C entry; only the new call is an ASM bridge'}
    out.append(entry)
result={'scope':'C kernels/wrappers and bounded runtime commits through 3450335; source before the ASM migration',
        'history':out,'current_assembly_inventory':'runtime_inventory.json',
        'remaining_changed_C':['main: existing loop; only added user_storage_poll() call, implemented in ASM'],
        'crc32_update':'ASM_CRC resolves directly to User/asm/crc_asm.s; the C fallback is used by the unchanged bootloader only',
        'unchanged_C':'bootloader (including User/sector_journal.c), STM32 libraries and unrelated communication/peripheral modules'}
(ROOT/'Tests/asm/runtime_history.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS history audit:',sum(len(v['functions']) for e in out for v in e['changed_functions'].values()),
      'function changes;',sorted({p for e in out for p in e['changed_functions']}))
