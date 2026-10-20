"""Доработка: Кряжев А., 2026. Регрессии восстановления и адаптера настроек."""
from pathlib import Path
import os
import subprocess
import tempfile
from temp_cleanup import TemporaryDirectory
from comm_server_host import ROOT, ZIG, unit, source, function


def review_unit():
    code = unit().replace('int main(void)', 'static int original_regressions(void)')
    code = code.replace('uint8_t protocolVersion, hardwareVersion;',
                        'uint8_t protocolVersion, hardwareVersion, moduleType;')
    user = source('User/c/user_modem.c')
    code += ('\nstatic struct { char netOperator[32], server1[32], server2[32]; uint8_t netType, useServer1, useServer2, rs485Mode;'
             ' struct { uint8_t address, type; } rs485Periphery[8]; } config;\n')
    code += user[user.index('static char comm_host'):user.index('// Обработчики')]
    code += function(source('User/c/photo_runtime.c'), 'rs485_wifi_enabled')
    code += '\n'.join(function(user, name) for name in ('comm_split', 'comm_endpoints', 'comm_text', 'comm_number'))
    code += (ROOT / 'Tests/test_comm_review.c').read_bytes().decode('utf-8')
    return code.replace('\r\n', '\n')


def run():
    out = Path(tempfile.gettempdir()) / 'domovoi-comm-review'
    out.mkdir(parents=True, exist_ok=True)
    os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(out / 'zig-global')
    os.environ['ZIG_LOCAL_CACHE_DIR'] = str(out / 'zig-local')
    with TemporaryDirectory(dir=out) as directory:
        directory = Path(directory)
        source = directory / 'review.c'
        source.write_text(review_unit(), encoding='utf-8')
        executable = directory / 'review.exe'
        subprocess.run([ZIG, 'cc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=undefined', '-fno-sanitize-recover=all',
                        str(source), '-o', str(executable)], check=True)
        subprocess.run([str(executable)], check=True, timeout=120)


if __name__ == '__main__':
    run()
