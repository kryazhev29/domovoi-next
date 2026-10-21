"""USSD: новые сценарии должны проходить сейчас и падать с автоматом баланса 2cad088.

Старая реализация берётся git show, без правки рабочей копии. Адаптер только
добавляет неиспользуемый bool idle к прежней сигнатуре и подставляет старые
1800000 мс вместо переименованного индекса таблицы. Метка BAL_CLOSE нужна
только новому тесту: старый автомат в это состояние не переходит.
"""
import json
import os
from pathlib import Path
import subprocess
import argparse
from comm_server_host import unit, ROOT, ZIG
from temp_cleanup import TemporaryDirectory


def c_module(text):
    return '\n'.join(line for line in text.splitlines() if not line.startswith('#include'))


def run():
    out = ROOT / 'Release/comm-stage4'
    out.mkdir(parents=True, exist_ok=True)
    os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(out / 'zig-global')
    os.environ['ZIG_LOCAL_CACHE_DIR'] = str(out / 'zig-local')
    code = unit().replace('\r\n', '\n').replace('int main(void)', 'static int all_tests(void)')
    code += '''
int main(int argc, char **argv) {
  CHECK(argc==2); unsigned n=(unsigned)atoi(argv[1]);
  if(n==0) balance_startup_test();
  else if(n==1) balance_reopen_test();
  else if(n==2) balance_daily_test();
  else if(n==18) { server_reset(); ussd_code="*100#"; advance(299000u-(sim_now-sv.registration_since)); CHECK(!cusd_count); }
  else if(n==19) balance_cancel_busy_test();
  else balance_busy_test(n-3,1);
  return 0;
}
'''
    current = c_module((ROOT / 'Modules/comm_balance.c').read_text(encoding='utf-8'))
    old = subprocess.check_output(['git', 'show', '2cad088:Modules/comm_balance.c'], cwd=ROOT).decode('utf-8')
    old = old.replace('comm_timeouts[COMM_T_USSD_RETRY]', '1800000u')
    old = old.replace('void comm_balance_poll(uint32_t now_ms)', 'void comm_balance_poll(uint32_t now_ms, bool idle)')
    old = old.replace('  uint8_t r;\n', '  (void)idle;\n  uint8_t r;\n')
    old += '\nenum { BAL_CLOSE = 254 };\n'
    assert code.count(current) == 1
    variants = {'current': code, '2cad088': code.replace(current, c_module(old))}
    names = ['first-registration-Log2-12-model', 'reopen-restarts-5min', 'daily-failure-menu-timeout',
             'QIOPEN', 'queued-TX', 'unacked', 'pending-RX', 'active-RX', 'priority', 'call',
             'AT-operation', 'probe', 'background-query', 'SMS-send', 'SMS-deliver', 'application-busy',
             'status-ACK', 'missed-status', 'first-5min', 'menu-cancel-busy']
    result = {}
    with TemporaryDirectory(dir=out) as tmp:
        tmp = Path(tmp)
        for variant, source in variants.items():
            c = tmp / (variant + '.c'); exe = c.with_suffix('.exe')
            c.write_text(source, encoding='utf-8')
            subprocess.run([ZIG, 'cc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                            '-Wno-unused-function', '-fsanitize=undefined', '-fno-sanitize-recover=all',
                            str(c), '-o', str(exe)], check=True)
            result[variant] = {}
            for n, name in enumerate(names):
                r = subprocess.run([str(exe), str(n)], capture_output=True, timeout=60)
                valid = r.returncode == 0 if variant == 'current' else r.returncode == 1 and b'FAIL ' in r.stderr
                assert valid, (variant, name, r.returncode, r.stderr.decode(errors='replace'))
                result[variant][name] = 'PASS' if variant == 'current' else 'FAIL_EXPECTED'
    print('PASS USSD: 20 current / 20 expected failures on 2cad088 (interface adapter only)')
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(); parser.add_argument('--output', type=Path)
    args = parser.parse_args(); result = run()
    if args.output:
        args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
