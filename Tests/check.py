"""Сборка и проверки одной командой; логи и результаты остаются в TEMP."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import contextlib
import io
import concurrent.futures
import shutil
from build_iar import build

ROOT=Path(__file__).resolve().parents[1]

def main():
    p=argparse.ArgumentParser();mode=p.add_mutually_exclusive_group(required=True)
    mode.add_argument('--quick',action='store_true');mode.add_argument('--full',action='store_true')
    a=p.parse_args();start=time.monotonic()
    base=Path(os.environ.get('DOMOVOI_COMPARE_BUILD',Path(tempfile.gettempdir())/'domovoi-agent-build'))
    base.mkdir(parents=True,exist_ok=True)
    env=os.environ.copy();env['DOMOVOI_COMPARE_BUILD']=str(base);env['PYTHONIOENCODING']='utf-8'
    results={}
    try:
        for config in (('Release',) if a.quick else ('Release','ReleaseCustomer')):
            with contextlib.redirect_stdout(io.StringIO()):build(base,config)
        if a.full:
            with contextlib.redirect_stdout(io.StringIO()):build(base,boot=True)
        def run_profile(config):
            profile_env=env.copy()
            if a.full:
                # Изолировать ELF и файлы результатов: два профиля можно проверить параллельно.
                scope=base/'runs'/config
                for folder in (config,'BOOT-Release'):
                    for sub in ('ExePath','ListPath'):
                        shutil.copytree(base/folder/sub,scope/folder/sub,dirs_exist_ok=True)
                profile_env['DOMOVOI_COMPARE_BUILD']=str(scope)
            cmd=[sys.executable,str(ROOT/'Tests/run_all.py'),'--config',config,'--output',str(base/f'run-all-{config}.json')]
            if a.quick:cmd.append('--quick')
            log_path=base/f'check-{config}.log'
            with log_path.open('wb') as log:r=subprocess.run(cmd,cwd=ROOT,env=profile_env,stdout=log,stderr=subprocess.STDOUT)
            if r.returncode:print(log_path.read_bytes()[-2400:].decode('utf-8',errors='replace'))
            return config,r.returncode==0
        if a.quick:results.update([run_profile('Release')])
        else:
            with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
                results.update(pool.map(run_profile,('Release','ReleaseCustomer')))
        if a.full:
            r=subprocess.run([sys.executable,str(ROOT/'Tests/test_dead_code.py')],cwd=ROOT,env=env,capture_output=True)
            (base/'test-dead-code.log').write_bytes(r.stdout+r.stderr);results['dead_code_regression']=r.returncode==0
            r=subprocess.run([sys.executable,str(ROOT/'Tests/check_dead_code.py'),'--build-root',str(base)],cwd=ROOT,env=env,capture_output=True)
            (base/'check-dead-code.log').write_bytes(r.stdout+r.stderr);results['dead_code']=r.returncode==0
            if r.returncode:print(r.stdout.decode('utf-8',errors='replace'))
    except Exception as e:
        results['build']=False;print(str(e))
    elapsed=round(time.monotonic()-start,2)
    if a.quick:results['under_5_minutes']=elapsed<=300
    passed=all(results.values()) and bool(results)
    (base/'check-summary.json').write_text(json.dumps({'mode':'quick' if a.quick else 'full','passed':passed,'seconds':elapsed,'results':results},indent=2)+'\n',encoding='utf-8')
    print(f'{"PASS" if passed else "FAIL"} {"QUICK" if a.quick else "FULL"}: '+', '.join(f'{k}={"PASS" if v else "FAIL"}' for k,v in results.items())+f'; {elapsed:.2f}s')
    return 0 if passed else 1

if __name__=='__main__':raise SystemExit(main())
