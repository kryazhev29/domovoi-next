"""Доработка: Кряжев А., 2026. Замер модулей связи той же командой IAR."""
from pathlib import Path
import argparse
import json
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
IAR = Path(r'C:\Program Files\IAR Systems\Embedded Workbench 9.2\arm')
MODULES = ('Modules/comm_uart.c', 'Modules/comm_at.c', 'Modules/modem_slm336.c',
           'Modules/comm_power_v5.c', 'User/comm_timeouts.c',
           'Modules/comm_channel.c', 'Modules/comm_server.c', 'Modules/comm_recover.c', 'Modules/comm_info.c', 'Tests/asm/reference/User/user_comm.c')


def measure(label, limits=False):
    out = ROOT / 'Release/comm-stage4_5' / label
    out.mkdir(parents=True, exist_ok=True)
    rows = {}
    modules = list(MODULES)
    if (ROOT / 'Modules/comm_modem.c').exists():
        modules.insert(2, 'Modules/comm_modem.c')
    for name in modules:
        path = Path(name)
        for source in (ROOT / name, ROOT / path.with_suffix('.h')):
            if source.exists():
                target = out / 'sources' / source.relative_to(ROOT)
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(source, target)
        cmd = [str(IAR / 'bin/iccarm.exe'), name, '--cpu', 'Cortex-M3', '--thumb',
               '-Ohz', '-e', '--dlib_config', str(IAR / 'inc/c/DLib_Config_Normal.h'),
               '-DSTM32F10X_MD', '-DUSE_STDPERIPH_DRIVER',
               '-DDOMOVOI_APP_HW=139', '-I.', '-IModules', '-IUser',
               '-ICMSIS/Include', '-ICMSIS/STM32F10x/Include',
               '-ISTM32F10x_StdPeriph_Driver/inc', '-IUSB-FS-CDC_User/Inc',
               '-ISTM32_USB-FS-Device_Driver/inc', '-IUtilities',
               '-o', str(out / (path.stem + '.o'))]
        run = subprocess.run(cmd, cwd=ROOT, capture_output=True)
        (out / (path.stem + '.log')).write_bytes(run.stdout + run.stderr)
        if run.returncode:
            print((run.stdout + run.stderr).decode('cp1251', errors='replace'))
            raise SystemExit(run.returncode)
        sizes = {kind: int(re.sub(r'\D', '', size)) for size, kind in
                 re.findall(r"([\d ',\u00a0]+) bytes of (CODE|CONST|DATA)\s+memory", run.stdout.decode('cp1251'))}
        rows[path.stem] = dict(flash=sizes.get('CODE', 0) + sizes.get('CONST', 0),
                               ram=sizes.get('DATA', 0), code=sizes.get('CODE', 0),
                               const=sizes.get('CONST', 0))
        dump = subprocess.run([str(IAR / 'bin/ielfdumparm.exe'), '-s', '.symtab',
                               str(out / (path.stem + '.o'))], cwd=ROOT, capture_output=True, check=True)
        (out / (path.stem + '.symbols.txt')).write_bytes(dump.stdout)
        functions = {name: int(size, 16) for name, size in re.findall(
            r'^\s*\d+: ([A-Za-z_]\w*)\s+0x[0-9a-f]+\s+\d+ Code \w+\s+(0x[0-9a-f]+)',
            dump.stdout.decode('cp1251'), re.M)}
        (out / (path.stem + '.functions.json')).write_text(json.dumps(functions, indent=2), encoding='utf-8')
        print(path.stem, rows[path.stem])
    (out / 'memory.json').write_text(json.dumps(rows, indent=2), encoding='utf-8')
    print(label, 'flash=', sum(v['flash'] for v in rows.values()),
          'RAM=', sum(v['ram'] for v in rows.values()))
    if limits:
        groups = ((('comm_uart',), 820), (('comm_at',), 2150),
                  (('comm_modem',), 2500), (('modem_slm336',), 600),
                  (('comm_channel',), 1800), (('comm_server', 'user_comm'), 870),
                  (('comm_power_v5', 'comm_timeouts'), 260))
        failed = []
        for names, limit in groups:
            size = sum(rows[name]['flash'] for name in names)
            if size > limit:
                print(f'Ориентир превышен: {" + ".join(names)}: {size} > {limit}')
        if sum(v['flash'] for v in rows.values()) > 9000:
            failed.append('total > 9000')
        if sum(v['ram'] for v in rows.values()) > 2264:
            failed.append('RAM > 2264')
        if failed:
            print('Budget FAIL: ' + '; '.join(failed))
            raise SystemExit(1)
        print('Budget PASS')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('label')
    parser.add_argument('--limits', action='store_true')
    args = parser.parse_args()
    measure(args.label, args.limits)
