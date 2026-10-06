"""Project-aware run_all entry point, with every generated host file under TEMP.

Historical synchronous source-extraction fixtures cannot test the current ASM
state machines. Those contracts are exercised by the existing linked-ELF tests
listed explicitly below. Common driver/protocol tests still compile their inputs.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

from c_vs_asm_suite import ROOT, CORE, POWER, run_cases, select_image
sys.path.insert(0,str(ROOT/'Tests'))
import run_all

REPLACEMENTS={
    'access':'access','bootjournal':'config_power','crcblocks':'service',
    'flashcmds':'clear','fwstart':'firmware','idindex':'index',
    'idsync':'table','log':'upload','logupload':'upload','photo':'photo',
    'rs485cfg':'photo_edges',
    'sectorjournal':'table_power','serverrx':'commserver','storage':'storage_boundary',
    'utcday':'access','uploadretry':'upload',
    'async config':'config','async callers':'owners',
    'async events':'event_append','async FW':'fw_blocks',
}


def mirror(destination):
    names=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','-z'],cwd=ROOT).decode('utf-8').split('\0')
    for name in names:
        source=ROOT/name
        if not name or not source.is_file():continue
        target=destination/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
    return destination


def main():
    sys.stdout.reconfigure(encoding='utf-8',errors='replace')
    p=argparse.ArgumentParser();p.add_argument('--project',choices=('ASM','C','HYBRID'),required=True)
    p.add_argument('--fast',action='store_true');p.add_argument('--output',type=Path)
    args=p.parse_args();select_image(args.project)
    base=Path(os.environ.get('DOMOVOI_COMPARE_BUILD',Path(tempfile.gettempdir())/'domovoi-c-asm'))
    destination=base/('host-'+args.project);destination.mkdir(parents=True,exist_ok=True);mirror(destination)
    env=os.environ.copy();env['DOMOVOI_SOURCES']=args.project.lower()
    env['PYTHONIOENCODING']='utf-8';env['PYTHONDONTWRITEBYTECODE']='1'
    env['PYTHONPATH']=os.pathsep.join((str(Path(tempfile.gettempdir())/'domovoi-c-asm-deps'),str(ROOT/'Tests'),env.get('PYTHONPATH','')))
    env['GIT_DIR']=subprocess.check_output(['git','rev-parse','--absolute-git-dir'],cwd=ROOT).decode('utf-8').strip()
    env['GIT_WORK_TREE']=str(destination)
    results={}
    def run(name,command):
        start=time.monotonic();r=subprocess.run([sys.executable,*command],cwd=destination,env=env,capture_output=True)
        (base/(args.project+'-'+name.replace(' ','_')+'.log')).write_bytes(r.stdout+r.stderr)
        ok=r.returncode==0 and b'FAIL' not in r.stdout
        results[name]={'status':'PASS' if ok else 'FAIL','seconds':round(time.monotonic()-start,3)}
        print(results[name]['status'],name,flush=True)
        if not ok:print((r.stdout+r.stderr)[-1600:].decode('utf-8',errors='replace'),flush=True)
    run('sources',['Tests/check_comm_sources.py'])
    run('hybrid ASM split',['Tests/test_hybrid_hot.py'])
    run('format oracle',['Tests/test_user_format.py'])
    for group in run_all.GROUPS:
        if group not in REPLACEMENTS:run(group,['Tests/run_host.py','work',group])
    for scenario in run_all.TX:run('tx '+scenario,['Tests/run_host.py','work','tx',scenario])
    run('modem_sim',['-m','unittest','Tests/modem_sim.py'])
    run('comm_review',['Tests/comm_review_host.py'])
    run('uart arm',['Tests/build_comm_uart.py'])
    # Runtime fixtures import production types/offsets from the selected ELF.
    cases=list(CORE)+['event_append','fw_blocks','fw_bridge','usb_queue']
    if not args.fast:cases+=list(POWER)
    results.update(run_cases(cases))
    if not args.fast:
        run('paired mutations',['Tests/asm/c_vs_asm_mutations.py','--variant',args.project,'--output',str(base/f'mutations-{args.project}.json')])
    result={'variant':args.project,'cases':results,'legacy_contract_mapping':REPLACEMENTS,
            'elf_sha256':__import__('hashlib').sha256(__import__('iar_image').image_paths()[0].read_bytes()).hexdigest()}
    output=args.output or base/f'run-all-{args.project}.json'
    output.write_text(json.dumps(result,indent=2,default=str)+'\n',encoding='utf-8')
    passed=sum(v['status']=='PASS' for v in results.values())
    print(f'TOTAL {passed}/{len(results)} passed; project {args.project}',flush=True)
    return 0 if passed==len(results) else 1

if __name__=='__main__':sys.exit(main())
