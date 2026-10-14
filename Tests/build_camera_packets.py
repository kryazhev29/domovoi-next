"""Три ReleaseCustomer и сравнение Flash/пакетов 14; исходную константу восстанавливаем."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
from build_iar import build, ROOT


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-root', required=True, type=Path)
    parser.add_argument('--sizes', nargs='*', type=int, choices=(64,256,512), default=(64,256,512))
    args = parser.parse_args()
    source = ROOT / 'User/photo_context.h'
    original = source.read_bytes()
    rows = []
    try:
        for size in args.sizes:
            source.write_bytes(re.sub(rb'#define PHOTO_PACKET \d+u',
                                     f'#define PHOTO_PACKET {size}u'.encode(), original))
            base = args.build_root / str(size)
            folder = build(base, 'ReleaseCustomer')
            env = dict(os.environ, DOMOVOI_COMPARE_BUILD=str(base),
                       DOMOVOI_COMPARE_CONFIG='ReleaseCustomer')
            with (base / 'camera-packet.log').open('wb') as log:
                subprocess.run([sys.executable, str(ROOT/'Tests/asm/test_camera_packet.py')],
                               cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT,
                               check=True)
        for size in (64,256,512):
            base = args.build_root / str(size)
            folder = base / 'ReleaseCustomer'
            evidence = json.loads((base/'camera-packet.json').read_text())
            binary = folder/'ExePath/DomovoiNext.bin'
            maptext = (folder/'ListPath/DomovoiNext.map').read_text(encoding='cp1251')
            def memory(kind):
                return int(re.search(r"([\d']+) bytes of\s+" + kind.replace('  ', r'\s+') + r'\s+memory', maptext)[1].replace("'", ''))
            stack_limit = int(re.search(r"CSTACK\$\$Limit\s+0x([\da-f']+)", maptext)[1].replace("'", ''), 16)
            rows.append(dict(packet=size, bin=str(binary), bin_bytes=binary.stat().st_size,
                             sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                             rw_data=memory('readwrite data'), rw_code=memory('readwrite code'),
                             ro_code=memory('readonly  code'), ro_data=memory('readonly  data'),
                             cstack=4096, free_above_stack=0x20004fc0-stack_limit,
                             evidence=evidence))
        assert len({r['evidence']['flash_sha256'] for r in rows}) == 1
        assert len({r['evidence']['stream_sha256'] for r in rows}) == 1
        (args.build_root/'variants.json').write_text(json.dumps(rows, indent=2)+'\n')
        print('PASS 64/256/512: Flash and packet 14 byte-identical')
    finally:
        source.write_bytes(original)


if __name__ == '__main__':
    main()
