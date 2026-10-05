"""Mutate isolated copies, require executable assertions to catch each fault."""
import os, pathlib, shutil, subprocess, sys, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
CASES=(
 ('refill-in-process','  server_rx_scan(); /* Also resume','  server_rx_work = SERVER_RX_WORK;\r\n  server_rx_scan(); /* Also resume'),
 ('lost-crc','server_rx_crc = crc16_calcInit(p + server_rx_pos, n, server_rx_crc);','server_rx_crc = crc16_calcInit(p + server_rx_pos, n, 0xffffu);'),
 ('unbounded-crc','if (n > server_rx_work) n = server_rx_work;','if (0) n = server_rx_work;'),
 ('stalled-input','  server_rx_scan(); /* Also resume','  if (0) server_rx_scan(); /* Also resume'),
 ('stale-reset','server_rx_ready = server_rx_offset = server_rx_total = server_rx_pos = 0;','server_rx_ready = server_rx_offset = 0;'),
 ('bad-crc-exec','if (server_rx_crc == ((uint16_t)p[server_rx_total - 2u] |','if (server_rx_crc != ((uint16_t)p[server_rx_total - 2u] |'),
 ('lost-partial','if (left < 6u) break;','if (left < 6u) { ++server_rx_offset; continue; }'),
)
with tempfile.TemporaryDirectory() as d:
    dst=pathlib.Path(d)
    shutil.copytree(ROOT/'User',dst/'User');shutil.copytree(ROOT/'Tests',dst/'Tests',ignore=shutil.ignore_patterns('__pycache__'))
    p=dst/'Tests/asm/reference/User/user_fun.c';original=p.read_bytes()
    for label,old,new in CASES:
        assert old.encode() in original,label
        p.write_bytes(original.replace(old.encode(),new.encode(),1))
        r=subprocess.run([sys.executable,str(dst/'Tests/run_host.py'),'work','serverrx'],capture_output=True,timeout=60,
                         env={**os.environ,'DOMOVOI_ROOT':str(dst)})
        p.write_bytes(original)
        assert r.returncode and b'FAIL' in r.stderr,(label,r.stderr[-1000:])
        print(label,'detected',flush=True)
print('PASS RX mutations: 7/7')
