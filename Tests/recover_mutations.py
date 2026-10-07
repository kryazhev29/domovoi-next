"""Доработка: Кряжев А., 2026. Мутации восстановления связи: группа commserver обязана ловить каждую."""
import subprocess
import sys
import os
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'Modules/comm_recover.c'
MODEM = ROOT / 'Modules/comm_modem.c'
INFO = ROOT / 'Modules/comm_info.c'
CHANNEL = ROOT / 'Modules/comm_channel.c'
CASES = (
    ('power-pause-skip', b'rc_power_ladder(rc_pause(COMM_T_POWER_1, rc.power_step));\r\n    return;\r\n  case SLM_ERR_TIMEOUT:', b'rc_power_ladder(0);\r\n    return;\r\n  case SLM_ERR_TIMEOUT:'),
    ('soft-forever', b'if (++rc.soft_fails >= 3u)', b'if (++rc.soft_fails >= 30u)'),
    ('no-sim-soft', b'case SLM_ERR_NO_SIM: case SLM_ERR_PIN:', b'case 250: case SLM_ERR_PIN:'),
    ('repeat-power-no-pause', b'return rc.power_step ? rc_pause(COMM_T_POWER_1, (uint8_t)(rc.power_step - 1u)) : 0;', b'return 0;'),
    ('no-tcp-first', b'if (rc.net_step == 0u) comm_channel_reopen(0);', b'if (0) comm_channel_reopen(0);'),
    ('no-soft-step', b'else if (rc.net_step == 1u) rc_schedule(false, 0);', b'else if (rc.net_step == 1u) rc_schedule(true, 0);'),
    ('early-counts-startup', b'if (step < SLM_TCP) return;', b'if (step > 200u) return;'),
    ('early-window', b'>= comm_timeouts[window]', b'>= comm_timeouts[window] * 10u'),
    ('night-busy-ignored', b'  if (busy) return;\r\n', b'  if (busy && false) return;\r\n'),
    ('night-twice', b'&& day != rc.night_day', b''),
    ('no-ladder-reset', b'rc.power_step = rc.soft_step = rc.soft_fails = rc.net_step = 0;', b'rc.soft_fails = 0;'),
    ('deact-error-fails', b' && !(action == MODEM_ANY && slm.attempt.result < COMM_AT_GOT_PROMPT)', b'', MODEM),
    ('query-error-reopens', b'  if (op == OP_QUERY) { query.cmd = NULL; return; }', b'  if (op == OP_QUERY) query.cmd = NULL;', CHANNEL),
    ('csq-not-parsed', b'info.v.csq = v[0] <= 99u ? (uint8_t)v[0] : 99u;', b'(void)v;', INFO),
    ('stale-info-offline', b'    info.v.csq = 99; info.v.creg', b'    info.v.csq = info.v.csq; info.v.creg', INFO),
    ('no-session-reset', b'rc.hooks->reset_session();', b''),
)


def main():
    from comm_server_host import unit
    from comm_at_host import ZIG
    production = unit().replace('\r\n', '\n')
    base = Path(os.environ.get('DOMOVOI_COMPARE_BUILD', tempfile.gettempdir()))/'recover-mutations'
    base.mkdir(parents=True, exist_ok=True)
    for name, old, new, *where in CASES:
        old, new = (data.decode('utf-8').replace('\r\n', '\n') for data in (old, new))
        assert production.count(old) == 1, name
        with tempfile.TemporaryDirectory(dir=base) as directory:
            folder=Path(directory);source=folder/'test.c';executable=folder/'test.exe'
            source.write_text(production.replace(old,new),encoding='utf-8')
            compiler=subprocess.run([ZIG,'cc','-std=c99','-O2','-Wall','-Wextra','-Werror',str(source),'-o',str(executable)],capture_output=True)
            (base/(name+'-compile.log')).write_bytes(compiler.stdout+compiler.stderr)
            compiler.check_returncode()
            result=subprocess.run([str(executable)],capture_output=True,timeout=120)
            (base/(name+'.log')).write_bytes(result.stdout+result.stderr)
            assert result.returncode and b'FAIL' in result.stderr, ('undetected',name)
        print(name,'detected',flush=True)
    print('recover mutations:',len(CASES),'/',len(CASES),'detected')


if __name__ == '__main__':
    main()
