"""Доработка: Кряжев А., 2026. Host/ARM сборка AT отдельно от приложения."""
from pathlib import Path
import argparse
import os
import struct
import subprocess
import tempfile
from modem_sim import c_header

ROOT = Path(__file__).resolve().parents[1]
ZIG = r'C:\Users\kryaz\Documents\Codex\2026-09-14\c-users-kryaz-desktop-domovoi-next\work\toolchain\ziglang\zig.exe'


def unit(root=ROOT):
    def source(name):
        return (root / name).read_bytes().decode('cp1251')
    uart_test = source('Tests/test_comm_uart.c').split('/* PRODUCTION */', 1)[0]
    uart_test = uart_test.replace('static uint8_t at_command[128], frame[1040];', '')
    uart_test = uart_test.replace('/* HEADER */', source('Modules/comm_uart.h'))
    code = '\n'.join(line for name in ('Modules/comm_uart.c', 'Modules/comm_at.c')
                     for line in source(name).splitlines() if not line.startswith('#include'))
    return uart_test + source('Modules/comm_at.h') + code + c_header() + source('Tests/test_comm_at.c')


def arm(out):
    # Clang читает комментарии UTF-8; рабочие исходники остаются cp1251/CRLF.
    with tempfile.TemporaryDirectory(dir=out, prefix='arm-') as tmp:
        tmp = Path(tmp)
        for name in ('comm_at.c', 'comm_at.h', 'comm_uart.h'):
            (tmp / name).write_text((ROOT / 'Modules' / name).read_bytes().decode('cp1251').replace('\r\n', '\n'), encoding='utf-8', newline='\n')
        # Freestanding Zig не поставляет string.h для ARM; стандартные объявления,
        # реализации не подменяются, внешние memcpy/memcmp остаются в объекте.
        (tmp / 'string.h').write_text('#include <stddef.h>\nvoid *memcpy(void *, const void *, size_t);\n'
                                    'void *memset(void *, int, size_t);\nint memcmp(const void *, const void *, size_t);\n'
                                    'size_t strlen(const char *);\n', encoding='ascii')
        obj = out / 'comm_at.o'
        subprocess.run([ZIG, 'cc', '-target', 'thumb-freestanding-eabi', '-mcpu=cortex_m3',
                        '-mthumb', '-Oz', '-std=c99', '-Wall', '-Wextra', '-Werror',
                        '-fstack-usage', '-I' + str(tmp), '-c', str(tmp / 'comm_at.c'), '-o', str(obj)], check=True)
    data = obj.read_bytes()
    h = struct.unpack_from('<16sHHIIIIIHHHHHH', data)
    sections = [struct.unpack_from('<IIIIIIIIII', data, h[6] + i * h[11]) for i in range(h[12])]
    strings_section = sections[h[13]]
    strings = data[strings_section[4]:strings_section[4] + strings_section[5]]
    names = {strings[s[0]:].split(b'\0', 1)[0].decode(): s[5] for s in sections if s[2] & 2}
    ram = sum(s[5] for s in sections if s[2] & 3 == 3)
    flash = sum(s[5] for s in sections if s[2] & 2 and s[1] != 8)
    print('comm_at ARM sections:', names, 'static RAM:', ram, 'Flash object:', flash, 'heap: 0')
    (out / 'memory.txt').write_text(f'RAM={ram}\nFlash={flash}\nsections={names}\n', encoding='utf-8')


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--arm', action='store_true')
    a = p.parse_args()
    out = ROOT / 'Release/comm-stage2'
    out.mkdir(parents=True, exist_ok=True)
    os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(out / 'zig-global')
    os.environ['ZIG_LOCAL_CACHE_DIR'] = str(out / 'zig-local')
    if a.arm:
        arm(out)
    else:
        with tempfile.TemporaryDirectory(dir=out, prefix='host-') as tmp:
            tmp = Path(tmp)
            (tmp / 'test.c').write_text(unit().replace('\r\n', '\n'), encoding='utf-8', newline='\n')
            subprocess.run([ZIG, 'cc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                            str(tmp / 'test.c'), '-o', str(tmp / 'test.exe')], check=True)
            subprocess.run([str(tmp / 'test.exe')], check=True, timeout=120)
