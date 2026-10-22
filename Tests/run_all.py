"""Доработка: Кряжев А., 2026.
Полный прогон единственной прошивки; исходники стендов и результаты хранятся в TEMP.
Исторические синхронные стенды дополняются проверками реальных автоматов на ELF.
Профиль выбирает соответствующий образ и его смещения DWARF.
"""
import argparse
import concurrent.futures
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

sys.path.insert(0,str(Path(__file__).resolve().parent/'asm'))
from firmware_suite import ROOT, CORE, POWER, select_image
sys.path.insert(0,str(ROOT/'Tests'))
GROUPS = ('access', 'bootjournal', 'cfgstrings', 'cmdlengths', 'commands', 'commat', 'commserver', 'commslm', 'commuart', 'countends', 'crc', 'crcblocks', 'crcff', 'flashcmds', 'fwblockbounds', 'fwstart', 'idexpiry', 'idindex', 'idsync', 'idtext', 'log', 'logcommit', 'logupload', 'outs', 'persistence', 'photo', 'recovery', 'rs485cfg', 'rules', 'rx', 'sectorjournal', 'serverrx', 'sms', 'storage', 'sync', 'tempidmeta', 'timersirq', 'uarttimer', 'uploadretry', 'usbdesc', 'usbresetwait', 'usbresume', 'usbsetupbounds', 'utcday', 'w25', 'wiegandirq')
TX = ('idle', 'reset', 'esof', 'busy', 'large')


from contract_mapping import REPLACEMENTS
from temp_cleanup import TemporaryDirectory
import tools_path


def mirror(destination):
    names=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','-z'],cwd=ROOT).decode('utf-8').split('\0')
    for name in names:
        source=ROOT/name
        if not name or not source.is_file():continue
        target=destination/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
    return destination


def default_jobs():
    return int(os.environ.get('DOMOVOI_TEST_JOBS', min(12, max(1, (os.cpu_count() or 2)//2))))


def execute_group(job, destination, base, config, env):
    name, command, elf = job
    start = time.monotonic()
    scope = base/'groups'/name.replace(' ', '_')
    scope.mkdir(parents=True, exist_ok=True)
    try:
        # select_image() и result_path() внутри стендов получают отдельные каталоги.
        for folder in (config, 'BOOT-Release'):
            for sub in ('ExePath', 'ListPath'):
                source = base/folder/sub
                if source.exists():
                    shutil.copytree(source, scope/folder/sub, dirs_exist_ok=True)
        with TemporaryDirectory(prefix='temp-', dir=scope) as temporary:
            # Python выбирает TMPDIR первым. TEMP/TMP оставляем компилятору:
            # установленный zig.exe строит общий безопасный кэш относительно TEMP.
            local_env = dict(env, DOMOVOI_COMPARE_BUILD=str(scope), TMPDIR=temporary)
            command = list(command)
            if elf:
                output = scope/'case.json'
                # Старый JSON не может замаскировать аварийное завершение нового запуска.
                output.unlink(missing_ok=True)
                command = ['Tests/asm/firmware_suite.py', '--case', name, '--output', str(output)]
            if '--build-root' in command:
                command[command.index('--build-root')+1] = str(scope)
            if name == 'uart arm':
                command += ['--out', str(scope/'uart-arm')]
            r = subprocess.run([sys.executable, 'Tests/temp_cleanup.py', *command],
                               cwd=destination, env=local_env, capture_output=True)
            (base/('DomovoiNext-'+config+'-'+name.replace(' ', '_')+'.log')).write_bytes(r.stdout+r.stderr)
            ok = r.returncode == 0 and b'FAIL' not in r.stdout
            if elf:
                result = json.loads(output.read_text(encoding='utf-8'))['cases'][name]
                if not ok:
                    result['status'] = 'FAIL'
                display = r.stdout.decode('utf-8', errors='replace').replace('\r\n', '\n')
                if not ok:
                    display += r.stderr[-1600:].decode('utf-8', errors='replace')
            else:
                result = {'status':'PASS' if ok else 'FAIL', 'seconds':round(time.monotonic()-start, 3)}
                display = result['status']+' '+name+'\n'
                if not ok:
                    display += (r.stdout+r.stderr)[-1600:].decode('utf-8', errors='replace')+'\n'
        return result, display
    except Exception as exc:
        import traceback
        trace = traceback.format_exc()
        result = {'status':'FAIL', 'seconds':round(time.monotonic()-start, 3)}
        if elf:
            result.update(error=str(exc), traceback=trace)
        return result, 'FAIL '+name+': '+trace+'\n'


def run_groups(jobs, destination, base, config, env, workers, previous=None):
    # Долгие группы стартуют первыми; печать и JSON сохраняют прежний порядок.
    previous = previous or {}
    def priority(job):
        name = job[0]
        estimate = 60 if name.endswith('_power') else 30 if name in (
            'protocol contract', 'kernel benefit', 'comm mutations', 'ELF mutations') else 0
        return previous.get(name, {}).get('seconds', estimate)
    finished = {}
    results = {}
    next_result = 0
    with concurrent.futures.ProcessPoolExecutor(max_workers=workers) as pool:
        pending = {pool.submit(execute_group, job, destination, base, config, env):job[0]
                   for job in sorted(jobs, key=priority, reverse=True)}
        for future in concurrent.futures.as_completed(pending):
            finished[pending[future]] = future.result()
            while next_result < len(jobs) and jobs[next_result][0] in finished:
                name = jobs[next_result][0]
                results[name], display = finished.pop(name)
                print(display, end='', flush=True)
                next_result += 1
    return results


def main():
    sys.stdout.reconfigure(encoding='utf-8',errors='replace')
    p=argparse.ArgumentParser();p.add_argument('--quick',action='store_true')
    p.add_argument('--fast',action='store_true');p.add_argument('--output',type=Path)
    p.add_argument('--config',choices=('Release','ReleaseCustomer'),default='Release')
    p.add_argument('--jobs',type=int,default=default_jobs(),help='Число независимых групп на профиль (DOMOVOI_TEST_JOBS).')
    args=p.parse_args()
    if args.jobs < 1:p.error('--jobs должен быть положительным')
    project="DomovoiNext"
    os.environ['DOMOVOI_COMPARE_CONFIG']=args.config
    os.environ['DOMOVOI_TEST_DIAG']='0' if args.config=='ReleaseCustomer' else '1'
    select_image()
    base=Path(os.environ.get('DOMOVOI_COMPARE_BUILD',Path(tempfile.gettempdir())/'domovoi-agent-build'))
    destination=Path(tempfile.mkdtemp(prefix=f'host-{project}-{args.config}-',dir=base));mirror(destination)
    env=os.environ.copy()
    env['PYTHONIOENCODING']='utf-8';env['PYTHONDONTWRITEBYTECODE']='1'
    # Копия Tests лежит в TEMP, поэтому пути инструментов передаются через окружение.
    env['DOMOVOI_ZIG']=tools_path.zig()
    env['PYTHONPATH']=os.pathsep.join((str(destination/'Tests'),env.get('PYTHONPATH') or tools_path.python_deps()))
    env['GIT_DIR']=subprocess.check_output(['git','rev-parse','--absolute-git-dir'],cwd=ROOT).decode('utf-8').strip()
    env['GIT_WORK_TREE']=str(destination)
    from build_iar import build
    boot=base/'BOOT-Release'
    if not args.quick and not (boot/'ExePath/DomovoiBoot.out').exists(): build(base,boot=True)
    env.update(DOMOVOI_BOOT_ELF=str(boot/'ExePath/DomovoiBoot.out'), DOMOVOI_BOOT_BIN=str(boot/'ExePath/DomovoiBoot.bin'), DOMOVOI_BOOT_MAP=str(boot/'ListPath/DomovoiBoot.map'))
    jobs=[]
    def run(name,command):
        jobs.append((name,command,False))
    run('sources',['Tests/check_comm_sources.py'])
    run('project sources',['Tests/test_project_sources.py'])
    run('format oracle',['Tests/test_user_format.py'])
    run('USB log ELF',['Tests/test_usb_log_elf.py'])
    for group in GROUPS:
        if group not in REPLACEMENTS and (not args.quick or group in ('commat','commserver','commslm','commuart','sms')):run(group,['Tests/run_host.py','work',group])
    for scenario in (() if args.quick else TX):run('tx '+scenario,['Tests/run_host.py','work','tx',scenario])
    if not args.quick:
        run('WG parity off',['Tests/run_host.py','work','wiegandirq','0'])
        run('comm_review',['Tests/comm_review_host.py'])
        run('fast start',['Tests/test_fast_start.py'])
        run('startup prefix',['Tests/measure_startup_prefix.py'])
        run('uart arm',['Tests/build_comm_uart.py'])
        for script in ('test_fault_capture','test_iar_boot_tx','test_iar_boot_usb_wait','test_iar_ram_stage7',
                       'test_iar_comm_uart','test_iar_peripheral_waits','test_iar_pma_bounds','test_iar_store_async',
                       'test_iar_timers_irq','test_iar_uart_clock','test_iar_wiegand_irq','test_iar_stack_mark','test_iar_pvd',
                       'test_iar_modem_startup','test_iar_startup_order','test_iar_io_review','test_iar_boot_review'):
            run(script,['Tests/'+script+'.py'])
        run('log extractor',['-m','unittest','Tests.test_modem_sim'])
        run('CRC differential',['Tests/asm/test_asm_crc.py','--firmware',os.environ['DOMOVOI_ELF'],'--json',str(base/f'crc-{project}-{args.config}.json')])
        run('C kernels',['Tests/test_c_kernels.py','--output',str(base/f'c-kernels-{args.config}.json')])
        # Стенды читают типы и смещения полей из выбранного ELF.
    if not args.quick:
        for name, script in [('settings ELF', 'test_comm_settings_elf.py'), ('app update ELF', 'test_app_update_elf.py')]:
            run(name, ['Tests/' + script, '--build-root', str(base), '--config', args.config])
        run('logic ELF', ['Tests/test_logic_cleanup.py', '--build-root', str(base), '--config', args.config, '--output', str(base / ('logic-' + project + '-' + args.config + '.json'))])
    cases=list(CORE)+['event_append','fw_blocks','fw_bridge','usb_queue']
    if not args.fast:cases+=list(POWER)
    if args.quick:cases=['quick protocol','quick storage','quick power']
    for case in dict.fromkeys(cases):jobs.append((case,[],True))
    if not args.quick:
        run('protocol contract',['Tests/asm/protocol_contract.py','--output',str(base/f'protocol-{args.config}.json')])
        run('kernel benefit',['Tests/benchmark_kernels.py','--output',str(base/f'kernel-benefit-{args.config}.json')])
    if not args.fast and not args.quick:
        run('comm mutations',['Tests/comm_mutations.py'])
        run('recover mutations',['Tests/recover_mutations.py'])
        run('comm review mutations',['Tests/test_comm_review_mutations.py'])
        run('RX mutations',['Tests/rx_budget_mutations.py'])
        run('call mutations',['Tests/call_mutations.py'])
        run('link call',['Tests/link_call_host.py','--mutations'])
        run('SMS mutations',['Tests/sms_mutations.py'])
        run('ELF mutations',['Tests/asm/elf_mutations.py','--output',str(base/f'mutations-{project}-{args.config}.json')])
    output=args.output or base/f'run-all-{project}-{args.config}.json'
    try:
        previous=json.loads(output.read_text(encoding='utf-8'))['cases']
    except (OSError, ValueError, KeyError):
        previous={}  # История влияет только на очередь, не на результат проверок.
    results=run_groups(jobs,destination,base,args.config,env,args.jobs,previous)
    result={'project':project,'configuration':args.config,'source_root':str(destination),'diagnostic_skips':(['USB command 111','DWT packet 113','111: USB/UART counters/register snapshots and internal states'] if args.config=='ReleaseCustomer' else []),'cases':results,'legacy_contract_mapping':REPLACEMENTS,
            'elf_sha256':__import__('hashlib').sha256(__import__('iar_image').image_paths()[0].read_bytes()).hexdigest()}
    output.write_text(json.dumps(result,indent=2,default=str)+'\n',encoding='utf-8')
    passed=sum(v['status']=='PASS' for v in results.values())
    print(f'TOTAL {passed}/{len(results)} passed; project {project}',flush=True)
    return 0 if passed==len(results) else 1

if __name__=='__main__':sys.exit(main())
