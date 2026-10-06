"""Доработка: Кряжев А., 2026. Мутации восстановления связи: группа commserver обязана ловить каждую."""
import subprocess
import sys
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
    ('night-busy-ignored', b'  if (busy) return;\r\n', b''),
    ('night-twice', b'&& day != rc.night_day', b''),
    ('no-ladder-reset', b'rc.power_step = rc.soft_step = rc.soft_fails = rc.net_step = 0;', b'rc.soft_fails = 0;'),
    ('deact-error-fails', b' && !(action == MODEM_ANY && slm.attempt.result < COMM_AT_GOT_PROMPT)', b'', MODEM),
    ('query-error-reopens', b'  if (op == OP_QUERY) { query.cmd = NULL; return; }', b'  if (op == OP_QUERY) query.cmd = NULL;', CHANNEL),
    ('csq-not-parsed', b'info.v.csq = v[0] <= 99u ? (uint8_t)v[0] : 99u;', b'(void)v;', INFO),
    ('stale-info-offline', b'    info.v.csq = 99; info.v.creg', b'    info.v.csq = info.v.csq; info.v.creg', INFO),
    ('no-session-reset', b'rc.hooks->reset_session();', b''),
)


def main():
    originals = {path: path.read_bytes() for path in (SOURCE, MODEM, INFO, CHANNEL)}
    missed = []
    try:
        for name, old, new, *where in CASES:
            path = where[0] if where else SOURCE
            original = originals[path]
            assert original.count(old) == 1, name
            path.write_bytes(original.replace(old, new))
            run = subprocess.run([sys.executable, str(ROOT / 'Tests/run_host.py'), 'work', 'commserver'], capture_output=True, timeout=900)
            print(name, 'detected' if run.returncode else 'MISSED')
            if not run.returncode:
                missed.append(name)
            path.write_bytes(original)
    finally:
        for path, data in originals.items():
            path.write_bytes(data)
    print('recover mutations:', len(CASES) - len(missed), '/', len(CASES), 'detected')
    sys.exit(1 if missed else 0)


if __name__ == '__main__':
    main()
