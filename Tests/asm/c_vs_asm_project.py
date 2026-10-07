"""Доработка: Кряжев А., 2026.
Полный прогон выбранной прошивки; исходники стендов и результаты хранятся в TEMP.
Исторические синхронные стенды дополняются проверками реальных автоматов на ELF.
Смена проекта и профиля выбирает соответствующий образ и его смещения DWARF.
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

from contract_mapping import REPLACEMENTS


def mirror(destination):
    names=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','-z'],cwd=ROOT).decode('utf-8').split('\0')
    for name in names:
        source=ROOT/name
        if not name or not source.is_file():continue
        target=destination/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
    return destination


def main():
    sys.stdout.reconfigure(encoding='utf-8',errors='replace')
    p=argparse.ArgumentParser();p.add_argument('--project',choices=('C','HYBRID'),required=True)
    p.add_argument('--fast',action='store_true');p.add_argument('--output',type=Path)
    p.add_argument('--config',choices=('Release','ReleaseCustomer'),default='Release')
    args=p.parse_args()
    os.environ['DOMOVOI_COMPARE_CONFIG']=args.config
    os.environ['DOMOVOI_TEST_DIAG']='0' if args.config=='ReleaseCustomer' else '1'
    select_image(args.project)
    base=Path(os.environ.get('DOMOVOI_COMPARE_BUILD',Path(tempfile.gettempdir())/'domovoi-c-asm'))
    destination=Path(tempfile.mkdtemp(prefix=f'host-{args.project}-{args.config}-',dir=base));mirror(destination)
    env=os.environ.copy()
    env['DOMOVOI_TEST_PROJECT']=args.project
    env['PYTHONIOENCODING']='utf-8';env['PYTHONDONTWRITEBYTECODE']='1'
    env['PYTHONPATH']=os.pathsep.join((str(Path(tempfile.gettempdir())/'domovoi-c-asm-deps'),str(ROOT/'Tests'),env.get('PYTHONPATH','')))
    env['GIT_DIR']=subprocess.check_output(['git','rev-parse','--absolute-git-dir'],cwd=ROOT).decode('utf-8').strip()
    env['GIT_WORK_TREE']=str(destination)
    from build_iar import build
    boot=base/'BOOT-Release'
    if not (boot/'ExePath/DomovoiBoot.out').exists(): build('BOOT',base,project_path=ROOT/'Bootloader/DomovoiBoot.ewp')
    env.update(DOMOVOI_BOOT_ELF=str(boot/'ExePath/DomovoiBoot.out'), DOMOVOI_BOOT_BIN=str(boot/'ExePath/DomovoiBoot.bin'), DOMOVOI_BOOT_MAP=str(boot/'ListPath/DomovoiBoot.map'))
    results={}
    def run(name,command):
        start=time.monotonic();r=subprocess.run([sys.executable,*command],cwd=destination,env=env,capture_output=True)
        (base/(args.project+'-'+args.config+'-'+name.replace(' ','_')+'.log')).write_bytes(r.stdout+r.stderr)
        ok=r.returncode==0 and b'FAIL' not in r.stdout
        results[name]={'status':'PASS' if ok else 'FAIL','seconds':round(time.monotonic()-start,3)}
        print(results[name]['status'],name,flush=True)
        if not ok:print((r.stdout+r.stderr)[-1600:].decode('utf-8',errors='replace'),flush=True)
    run('sources',['Tests/check_comm_sources.py'])
    run('project sources',['Tests/test_hybrid_hot.py'])
    run('format oracle',['Tests/test_user_format.py'])
    for group in run_all.GROUPS:
        if group not in REPLACEMENTS:run(group,['Tests/run_host.py','work',group])
    for scenario in run_all.TX:run('tx '+scenario,['Tests/run_host.py','work','tx',scenario])
    run('WG parity off',['Tests/run_host.py','work','wiegandirq','0'])
    run('comm_review',['Tests/comm_review_host.py'])
    run('uart arm',['Tests/build_comm_uart.py'])
    for script in ('test_fault_capture','test_iar_boot_tx','test_iar_boot_usb_wait','test_iar_ram_stage7',
                   'test_iar_comm_uart','test_iar_peripheral_waits','test_iar_pma_bounds','test_iar_store_async',
                   'test_iar_timers_irq','test_iar_uart_clock','test_iar_wiegand_irq','test_iar_stack_mark','test_iar_pvd'):
        run(script,['Tests/'+script+'.py'])
    run('log extractor',['-m','unittest','Tests.test_modem_sim'])
    run('CRC differential',['Tests/asm/test_asm_crc.py','--firmware',os.environ['DOMOVOI_ELF'],'--json',str(base/f'crc-{args.project}-{args.config}.json')])
    run('kernel differential',['Tests/asm/test_fast_kernels.py','--firmware',os.environ['DOMOVOI_ELF'],'--mutations','--json',str(base/f'kernels-{args.project}-{args.config}.json')])
    # Стенды читают типы и смещения полей из выбранного ELF.
    if args.project in ('C', 'HYBRID'):
        for name, script in [('settings ELF', 'test_comm_settings_elf.py'), ('app update ELF', 'test_app_update_elf.py')]:
            run(name, ['Tests/' + script, '--build-root', str(base), '--variant', args.project, '--config', args.config])
    if args.project in ('C', 'HYBRID'):
        run('logic ELF', ['Tests/test_logic_cleanup.py', '--build-root', str(base), '--variant', args.project,
                          '--config', args.config, '--output', str(base / ('logic-' + args.project + '-' + args.config + '.json'))])
    cases=list(CORE)+['event_append','fw_blocks','fw_bridge','usb_queue']
    if not args.fast:cases+=list(POWER)
    results.update(run_cases(cases))
    if not args.fast:
        run('comm mutations',['Tests/comm_mutations.py'])
        run('recover mutations',['Tests/recover_mutations.py'])
        run('RX mutations',['Tests/rx_budget_mutations.py'])
        run('call mutations',['Tests/call_mutations.py'])
        run('paired mutations',['Tests/asm/c_vs_asm_mutations.py','--variant',args.project,'--output',str(base/f'mutations-{args.project}-{args.config}.json')])
    result={'variant':args.project,'configuration':args.config,'source_root':str(destination),'diagnostic_skips':(['USB command 111','USB command 112','DWT packet 113','111: USB counters/register snapshots','112: modem timings/step history and UART cumulative counters'] if args.config=='ReleaseCustomer' else []),'cases':results,'legacy_contract_mapping':REPLACEMENTS,
            'elf_sha256':__import__('hashlib').sha256(__import__('iar_image').image_paths()[0].read_bytes()).hexdigest()}
    output=args.output or base/f'run-all-{args.project}-{args.config}.json'
    output.write_text(json.dumps(result,indent=2,default=str)+'\n',encoding='utf-8')
    passed=sum(v['status']=='PASS' for v in results.values())
    print(f'TOTAL {passed}/{len(results)} passed; project {args.project}',flush=True)
    return 0 if passed==len(results) else 1

if __name__=='__main__':sys.exit(main())
