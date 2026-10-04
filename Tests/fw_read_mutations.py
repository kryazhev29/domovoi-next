"""Доработка: Кряжев А., 2026. Ошибки чтения Flash при обновлении прошивки."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def run():
    source = (ROOT / 'User/user_fun.c').read_text(encoding='cp1251')
    cases = (
        ('crc-read', 'if (!mem_read (adr, bf, readed)) return false;', 'mem_read (adr, bf, readed);'),
        ('crc-empty', 'if (!size || (size & 3u))', 'if (size & 3u)'),
        ('crc-alignment', 'if (!size || (size & 3u))', 'if (!size)'),
        ('read-back', 'if (!mem_read(adr + off, b, sizeof(b))) return 0;', 'mem_read(adr + off, b, sizeof(b));'),
        ('final-header', 'if (!mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&fwParam, sizeof(bootFWparam_s))) {', 'if (mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&fwParam, sizeof(bootFWparam_s)) && 0) {'),
        ('same-download', 'if (!mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&h, sizeof(h))) return 0;', 'mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&h, sizeof(h));'),
        ('resume-header', 'if (!mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&h, sizeof(h)) || BOOT_VER', 'if ((mem_read(mem[MB_FIRMWARE].start, (uint8_t*)&h, sizeof(h)) && 0) || BOOT_VER'),
        ('resume-cursor', 'device.FWblockIdx < device.FWblockCount ? device.FWblockIdx : UINT16_MAX', 'device.FWblockIdx'),
    )
    out = ROOT / 'Release/fw-read-review'
    out.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='fw-read-', dir=out) as directory:
        copied = Path(directory)
        for name in ('User/user_fun.c', 'structures.h', 'Tests/test_fw_start.c'):
            target = copied / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(ROOT / name, target)
        env = dict(os.environ, DOMOVOI_ROOT=str(copied))
        for label, old, new in cases:
            assert source.count(old) == 1, label
            (copied / 'User/user_fun.c').write_bytes(source.replace(old, new).replace('\n', '\r\n').encode('cp1251'))
            result = subprocess.run([sys.executable, str(ROOT / 'Tests/run_host.py'), 'work', 'fwstart'],
                                    env=env, capture_output=True, timeout=300)
            output = result.stdout + result.stderr
            (out / (label + '.log')).write_bytes(output)
            assert result.returncode == 1 and b'FAIL ' in output and b'Traceback' not in output, (label, output.decode(errors='replace'))
            print(label, 'detected')
    print(f'firmware read mutations: {len(cases)}/{len(cases)} detected')


if __name__ == '__main__':
    run()
