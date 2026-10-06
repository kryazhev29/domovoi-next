"""Доработка: Кряжев А., 2026. Изолированная модель SLM336 и оценка ARM."""
from pathlib import Path
import argparse
import os
import struct
import subprocess
import tempfile
from comm_at_host import ROOT, ZIG
from source_select import diagnostic_header

MODULES = ('User/comm_timeouts.c', 'Modules/comm_power_v5.c', 'Modules/comm_modem.c', 'Modules/modem_slm336.c')
HEADERS = ('Modules/comm_uart.h', 'Modules/comm_at.h', 'User/comm_timeouts.h',
           'Modules/comm_power_v5.h', 'Modules/modem_slm336.h', 'Modules/comm_modem.h')


def unit(root=ROOT):
    def source(name):
        return (root / name).read_bytes().decode('cp1251')
    model = source('Tests/test_comm_uart.c').split('/* PRODUCTION */', 1)[0]
    model = model.replace('static uint8_t at_command[128], frame[1040];', '')
    model = model.replace('/* HEADER */', source('Modules/comm_uart.h'))
    headers = '\n'.join(line for n in HEADERS[1:] for line in source(n).splitlines()
                        if not line.startswith('#include "'))
    sources = '\n'.join(line for n in ('Modules/comm_uart.c', 'Modules/comm_at.c', *MODULES)
                        for line in source(n).splitlines() if not line.startswith('#include'))
    return diagnostic_header(root) + model + headers + sources + '\n' + source('Tests/test_comm_slm.c')


def memory(obj):
    data = obj.read_bytes()
    h = struct.unpack_from('<16sHHIIIIIHHHHHH', data)
    sections = [struct.unpack_from('<IIIIIIIIII', data, h[6] + i * h[11]) for i in range(h[12])]
    ram = sum(s[5] for s in sections if s[2] & 3 == 3)
    flash = sum(s[5] for s in sections if s[2] & 2 and s[1] != 8)
    return ram, flash


def arm(out):
    rows = []; stage_ram = 0
    with tempfile.TemporaryDirectory(dir=out) as tmp:
        tmp = Path(tmp)
        for name in HEADERS + MODULES + ('Modules/comm_at.c','User/domovoi_diag.h'):
            (tmp / Path(name).name).write_text((ROOT / name).read_bytes().decode('cp1251').replace('\r\n', '\n'), encoding='utf-8', newline='\n')
        (tmp / 'string.h').write_text('#include <stddef.h>\nvoid *memcpy(void*,const void*,size_t);\n'
                                    'void *memset(void*,int,size_t);\nint memcmp(const void*,const void*,size_t);\n'
                                    'size_t strlen(const char*);\n', encoding='ascii')
        (tmp / 'stdio.h').write_text('#include <stddef.h>\nint snprintf(char*,size_t,const char*,...);\n', encoding='ascii')
        for name in (*MODULES, 'Modules/comm_at.c'):
            stem = Path(name).stem
            obj = out / (stem + '.o')
            subprocess.run([ZIG, 'cc', '-target', 'thumb-freestanding-eabi', '-mcpu=cortex_m3',
                            '-mthumb', '-Oz', '-std=c99', '-Wall', '-Wextra', '-Werror',
                            '-fstack-usage', '-I' + str(tmp), '-c', str(tmp / (stem + '.c')),
                            '-o', str(obj)], check=True)
            ram, flash = memory(obj)
            if name in MODULES: stage_ram += ram
            rows.append(f'{stem}: RAM={ram} Flash={flash} heap=0')
    (out / 'memory.txt').write_text('\n'.join(rows) + '\n', encoding='utf-8')
    print('\n'.join(rows))
    assert stage_ram <= 250, ('stage 3 RAM', stage_ram, 250)


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--arm', action='store_true')
    p.add_argument('--sanitize', action='store_true')
    a = p.parse_args()
    out = ROOT / 'Release/comm-stage3'
    out.mkdir(parents=True, exist_ok=True)
    os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(out / 'zig-global')
    os.environ['ZIG_LOCAL_CACHE_DIR'] = str(out / 'zig-local')
    if a.arm:
        arm(out)
    else:
        with tempfile.TemporaryDirectory(dir=out) as tmp:
            tmp = Path(tmp)
            (tmp / 'test.c').write_text(unit().replace('\r\n', '\n'), encoding='utf-8', newline='\n')
            subprocess.run([ZIG, 'cc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                            *(['-fsanitize=undefined', '-fno-sanitize-recover=all'] if a.sanitize else []),
                            str(tmp / 'test.c'), '-o', str(tmp / 'test.exe')], check=True)
            subprocess.run([str(tmp / 'test.exe')], check=True, timeout=120)
