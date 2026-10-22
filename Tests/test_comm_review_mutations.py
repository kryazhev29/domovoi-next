"""Регрессии отчёта по связи: возврат каждого старого дефекта обязан сломать модель."""
import os
import subprocess
import tempfile
from pathlib import Path
from comm_server_host import unit, ZIG
from temp_cleanup import TemporaryDirectory


def main():
    code = unit().replace('\r\n', '\n')
    cases = (
        ('registration-cancels-power',
         'rc.power <= RC_POWER && s->state == SLM_FAILED\n      && s->phase == SLM_PHASE_NETWORK && !s->at_fault',
         '(s != NULL)'),
        ('sms-result-waits-flash',
         'comm_sms_poll(sim_now, sms_access_ready);',
         'if (sms_access_ready) comm_sms_poll(sim_now, sms_access_ready);'),
        ('lte-reset-on-any-line',
         'uint8_t rsrp = 255, rsrq = 255, sinr = 255, band = 255, bandwidth = 255;',
         'uint8_t rsrp = 255, rsrq = 255, sinr = 255, band = 255, bandwidth = 255; info.v.rsrp = 255;'),
        ('gsm-escape-discarded',
         "if (s == 0x1B) { sms_char('?'); msg.half = 1; return; }",
         'if (s == 0x1B) return;'),
        ('empty-international-sender',
         " || (msg.nlen == 1u && msg.number[0] == '+')", ''),
        ('sms-encoding-past-limit',
         'while (len < SMS_UD * 8u / 7u && text[len])',
         'while (text[len] && len < 255u)'),
        ('null-low-signal-handler', 'info_low && info.v.csq < 15u', 'info.v.csq < 15u'),
    )
    base = Path(os.environ.get('DOMOVOI_COMPARE_BUILD', tempfile.gettempdir())) / 'comm-review-mutations'
    base.mkdir(parents=True, exist_ok=True)
    for name, old, new in cases:
        assert code.count(old) == 1, name
        with TemporaryDirectory(dir=base) as folder:
            source = Path(folder) / 'test.c'; exe = Path(folder) / 'test.exe'
            source.write_text(code.replace(old, new), encoding='utf-8')
            build = subprocess.run([ZIG, 'cc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                                    str(source), '-o', str(exe)], capture_output=True)
            (base / (name + '-compile.log')).write_bytes(build.stdout + build.stderr)
            build.check_returncode()
            result = subprocess.run([str(exe)], capture_output=True, timeout=120)
            (base / (name + '.log')).write_bytes(result.stdout + result.stderr)
            assert result.returncode != 0, ('undetected', name)
        print(name, 'detected', flush=True)
    print('comm review mutations: 7/7 detected')


if __name__ == '__main__':
    main()
