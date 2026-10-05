"""Re-run saved pre-change 3450335 ELF, never rebuild/replace the current image."""
import shutil,json,hashlib
from test_photo_runtime import *
import iar_irq_support as support

saved=ROOT/'Release/task-asm-storage'
image=saved/'baseline-root'
for folder,ext in (('Exe','out'),('Exe','bin'),('List','map')):
    target=image/'Release'/folder/f'DomovoiNext.{ext}'
    target.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(saved/f'DomovoiNext.{ext}',target)
support.ROOT=image
m=PhotoMachine(prefix_flash());assert m.checked('photo_start',5,budget=1000000)==0
start=m.steps;assert m.checked('photo_prepare',129)==1;prepare=m.steps
steps=[]
while m.pget('stage')!=6:
    m.checked('port_check_data',budget=2000000);steps.append(m.steps)
    assert len(steps)<300 and m.pget('stage'),m.pget('stage')
out={'units':'Unicorn instructions; modeled NOR; NOT cycles/time','baseline':'3450335',
     'elf_sha256':hashlib.sha256((saved/'DomovoiNext.out').read_bytes()).hexdigest(),
     'start':start,'prepare':prepare,'port_poll_max':max(steps),'port_poll_passes':len(steps),
     'port_poll_total':sum(steps),'stack':m.stack_peak}
support.ROOT=ROOT
m=PhotoMachine(prefix_flash());m.boot_media();assert m.checked('photo_start',5)==0;m.drain()
assert m.checked('photo_prepare',129)==1
steps=[]
while m.pget('stage')!=6:
    before=len(m.costs);m.port();m.tick();steps.append(sum(m.costs[before:]))
    assert len(steps)<1500 and not m.rget('fault')
out['after']={'port_and_storage_max':max(steps),'passes':len(steps),
              'total_instructions':sum(steps),'stack':m.stack_peak,
              'elf_sha256':hashlib.sha256((ROOT/'Release/Exe/DomovoiNext.out').read_bytes()).hexdigest()}
(ROOT/'Tests/asm/photo_baseline_results.json').write_text(json.dumps(out,indent=2)+'\n')
print(out)
