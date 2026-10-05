"""Доработка: Кряжев А., 2026. Регрессии восстановления и адаптера настроек."""
from pathlib import Path
import os
import subprocess
import tempfile
from comm_server_host import ROOT, ZIG, unit, source


def review_unit():
    code = unit().replace('int main(void)', 'static int original_regressions(void)')
    code = code.replace('uint8_t protocolVersion, hardwareVersion;',
                        'uint8_t protocolVersion, hardwareVersion, moduleType;')
    user = source('Tests/asm/reference/User/user_fun.c')
    code += '\nstatic struct { char netOperator[32], server1[32], server2[32]; uint8_t netType, useServer1, useServer2; } config;\n'
    begin = 'static char comm_host[2][32]' if 'static char comm_host[2][32]' in user else 'char comm_host[2][32]'
    end = 'static const slm_settings_t comm_settings' if 'static const slm_settings_t comm_settings' in user else 'const slm_settings_t comm_settings'
    code += user[user.index(begin):user.index(end)]
    code += (ROOT / 'Tests/test_comm_review.c').read_bytes().decode('cp1251')
    return code.replace('\r\n', '\n')


def run():
    out = Path(tempfile.gettempdir()) / 'domovoi-comm-review'
    out.mkdir(parents=True, exist_ok=True)
    os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(out / 'zig-global')
    os.environ['ZIG_LOCAL_CACHE_DIR'] = str(out / 'zig-local')
    with tempfile.TemporaryDirectory(dir=out) as directory:
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
