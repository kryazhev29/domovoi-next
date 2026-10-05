"""Runtime firmware continuation contract mutations in isolated copies."""
import pathlib, shutil, subprocess, sys, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
CASES=(
 ('early-cursor','fw_job.offset = 0; fw_job.waiting','device.FWblockIdx++; fw_job.offset = 0; fw_job.waiting'),
 ('busy-as-done','if (r == 1) return;\r\n    fw_job.waiting = 0;','if (r == 1) r = 2;\r\n    fw_job.waiting = 0;'),
 ('early-ack','if (code == FW_BLOCK_PENDING) fw_async_reply(uniqId);','if (code == FW_BLOCK_PENDING) { fw_async_reply(uniqId); user_send_statusUpdateFW(20, 0, cch, uniqId); }'),
 ('skip-crc','fw_blocks_done(fw_job.crc == fw_job.expected_crc)','fw_blocks_done(1)'),
 ('lose-disconnect','if (fw_async_active() && fw_job.channel == channel) fw_job.quiet = 1;','if (fw_async_active() && fw_job.channel != channel) fw_job.quiet = 1;'),
 ('free-owned-source','if (fw_job.waiting && w25_async_poll(now) == 1) return;','if (0 && fw_job.waiting && w25_async_poll(now) == 1) return;'),
 ('ignore-read-error','if (!mem_read(fw_job.position, fw_job.data, n)) goto read_error;','mem_read(fw_job.position, fw_job.data, n);'),
)
with tempfile.TemporaryDirectory() as d:
    dst=pathlib.Path(d)
    for name in ('User','Tests'):shutil.copytree(ROOT/name,dst/name,ignore=shutil.ignore_patterns('__pycache__'))
    shutil.copyfile(ROOT/'structures.h',dst/'structures.h')
    p=dst/'Tests/asm/reference/User/user_fun.c';original=p.read_bytes()
    for label,old,new in CASES:
        assert old.encode() in original,label
        p.write_bytes(original.replace(old.encode(),new.encode(),1))
        r=subprocess.run([sys.executable,str(dst/'Tests/test_fw_async.py')],cwd=dst,capture_output=True,timeout=60)
        p.write_bytes(original)
        assert r.returncode and b'FAIL' in r.stderr,(label,r.stderr[-800:])
        print(label,'detected',flush=True)
print('PASS FW mutations: 7/7')
