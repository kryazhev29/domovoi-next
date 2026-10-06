"""Доработка: Кряжев А., 2026.
Мутации писателя событий: каждый внесённый отказ должен обнаруживаться исполняемым тестом.
Источники данных и отказы задаются моделью; результат проверяется утверждениями.
"""
import pathlib, shutil, subprocess, sys, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
CASES=(
 ('early-cursor','log_job.phase = LA_BODY; break;','device.newAddressLog = log_job.address + sizeof(actual); log_job.phase = LA_BODY; break;'),
 ('busy-is-success','if (r == 1) return 1;','if (r == 1) return 2;'),
 ('pop-on-error','if (log_async_step(now) != 2) return;','if (log_async_step(now) == 1) return;'),
 ('skip-body','log_job.phase = LA_BODY; break;','log_job.phase = LA_MARK; break;'),
 ('duplicate-late-commit','if (!memcmp(&actual, &log_job.cell, sizeof(actual)) && log_valid(&actual)) {','if (log_job.result != 3 && !memcmp(&actual, &log_job.cell, sizeof(actual)) && log_valid(&actual)) {'),
 ('change-inflight-time','log_async_active() ? 1u : 0u','0u'),
)
with tempfile.TemporaryDirectory() as d:
    dst=pathlib.Path(d)
    for name in ('User','Tests'):shutil.copytree(ROOT/name,dst/name,ignore=shutil.ignore_patterns('__pycache__'))
    shutil.copyfile(ROOT/'structures.h',dst/'structures.h')
    p=dst/'Tests/asm/reference/User/user_fun.c';original=p.read_bytes()
    for label,old,new in CASES:
        assert old.encode() in original,label
        p.write_bytes(original.replace(old.encode(),new.encode(),1))
        r=subprocess.run([sys.executable,str(dst/'Tests/test_event_async.py')],cwd=dst,capture_output=True,timeout=60)
        p.write_bytes(original)
        assert r.returncode and b'FAIL' in r.stderr,(label,r.stderr[-800:])
        print(label,'detected',flush=True)
print('PASS event mutations: 6/6')
