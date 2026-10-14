"""Доработка: Кряжев А., 2026. Мутации восстановления связи: группа commserver обязана ловить каждую."""
import subprocess
import sys
import os
import tempfile
from temp_cleanup import TemporaryDirectory
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'Modules/comm_recover.c'
MODEM = ROOT / 'Modules/comm_modem.c'
INFO = ROOT / 'Modules/comm_info.c'
SERVER = ROOT / 'Modules/comm_server.c'
CHANNEL = ROOT / 'Modules/comm_channel.c'
CASES = (
    ('antenna-never-expires', b'if (info.antenna && (int32_t)(now_ms - info.antenna_until) >= 0) info.antenna = 0;', b'/* lease never expires */', INFO),
    ('antenna-not-renewed', b'info.antenna = 1; info.antenna_until = now_ms + 60000u;', b'if (!info.antenna) { info.antenna = 1; info.antenna_until = now_ms + 60000u; }', INFO),
    ('lte-reset-on-enqueue', 'info.qeng_ok = 0; /* значения не сбрасываем'.encode('utf-8'), b'info.qeng_ok = 0; info.v.rsrp = 255; /* x', INFO),
    ('lte-empty-ok-kept', b'if (n || !info.qeng_ok)', b'if (n)', INFO),
    ('lte-wrong-field', b'for (uint8_t i = 0; i < 10u; ++i) {', b'for (uint8_t i = 0; i < 9u; ++i) {', INFO),
    ('server-pair-order', b'if (!ch.call.expected) ch.address = !ch.flags.registered_session && !ch.address && comm_modem_reserve();',
     b'if (!ch.call.expected) ch.address = comm_modem_reserve() && (ch.flags.registered_session || !ch.address);', CHANNEL),
    ('server-ping-between-addresses', b'sv.state != SV_ONLINE && !sv.address && comm_modem_reserve()', b'false', SERVER),
    ('server-probe-starved-by-queue', b'&& (!ch.probe || ch.probe_active)', b'', CHANNEL),
    ('server-closed-cancels-probe', '    if (ch.state == CH_PROBE || ch.state == CH_OPENING) return; /* До успешного QIOPEN поздний closed не меняет попытку/пробу. */'.encode('utf-8'), b'', CHANNEL),
    ('server-late-closed-cancels-reserve', b'ch.state == CH_PROBE || ch.state == CH_OPENING', b'ch.state == CH_PROBE', CHANNEL),
    ('server-dns-fence-as-failure', 'return; // Старое окно DNS ещё не означает отсутствие интернета.'.encode('utf-8'), b'ch.probe = 0; channel_emit(COMM_CH_PROBE_DONE, 0);', CHANNEL),
    ('server-retry-cancels-open', 'sv.state = SV_OFF; // Канал сообщит результат; таймер не прерывает идущий QIOPEN.'.encode('utf-8'), b'/* timer still active */', SERVER),
    ('server-tcp-counted-early', b'  if (step < SLM_TCP || (step == SLM_TCP && reason == SLM_ERR_TIMEOUT)) return;', b'  if (reason == SLM_ERR_TCP) { rc.now = now_ms; rc_power_ladder(0); return; }\r\n  if (step < SLM_TCP || (step == SLM_TCP && reason == SLM_ERR_TIMEOUT)) return;'),
    ('server-qio-timeout-counted-early', b'  if (step < SLM_TCP || (step == SLM_TCP && reason == SLM_ERR_TIMEOUT)) return;', b'  if (step < SLM_TCP) return;'),

    ('server-probe-needs-socket', b'(ch.state != CH_OPEN && ch.state != CH_PROBE) || ch.probe', b'!comm_channel_ready() || ch.probe', CHANNEL),
    ('server-open-fails-no-probe', b'ch.state = CH_PROBE; channel_emit(COMM_CH_OPEN_FAILED, ch.address);', b'channel_emit(COMM_CH_OPEN_FAILED, ch.address); comm_channel_reopen(0);', CHANNEL),
    ('server-repeat-event13', b'if (!sv.flags.traffic_event) sv.flags.traffic_event = sv.handlers.no_traffic();', b'sv.flags.traffic_event = sv.handlers.no_traffic();', SERVER),

    ('power-pause-skip', b'rc_power_ladder(rc_pause(COMM_T_POWER_1, rc.power_step));\r\n    return;\r\n  case SLM_ERR_TIMEOUT:', b'rc_power_ladder(0);\r\n    return;\r\n  case SLM_ERR_TIMEOUT:'),
    ('soft-forever', b'if (++rc.soft_fails >= 3u)', b'if (++rc.soft_fails >= 30u)'),
    ('no-sim-soft', b'case SLM_ERR_NO_SIM: case SLM_ERR_PIN:', b'case 250: case SLM_ERR_PIN:'),
    ('repeat-power-no-pause', b'return rc.power_step ? rc_pause(COMM_T_POWER_1, (uint8_t)(rc.power_step - 1u)) : 0;', b'return 0;'),
    ('no-tcp-first', b'if (rc.net_step == 0u) comm_channel_reopen(0);', b'if (0) comm_channel_reopen(0);'),
    ('no-soft-step', b'else if (rc.net_step == 1u) rc_schedule(false, 0);', b'else if (rc.net_step == 1u) rc_schedule(true, 0);'),
    ('early-counts-startup', b'if (step < SLM_TCP || (step == SLM_TCP && reason == SLM_ERR_TIMEOUT)) return;', b'if (step > 200u) return;'),
    ('early-window', b'>= comm_timeouts[window]', b'>= comm_timeouts[window] * 10u'),
    ('night-busy-ignored', b'  if (busy) return;\r\n', b'  if (busy && false) return;\r\n'),
    ('night-twice', b'&& day != rc.night_day', b''),
    ('no-ladder-reset', b'rc.power_step = rc.soft_step = rc.soft_fails = rc.net_step = 0;', b'rc.soft_fails = 0;'),
    ('deact-error-fails', b'action == MODEM_ANY || action == MODEM_APN_QUERY', b'action == MODEM_APN_QUERY', MODEM),
    ('query-error-reopens', b'    query.cmd = NULL; return;', b'    query.cmd = NULL;', CHANNEL),
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
    changes = []
    for name, old, new, *where in CASES:
        old, new = (data.decode('utf-8').replace('\r\n', '\n') for data in (old, new))
        assert production.count(old) == 1, name
        changes.append((name, production.replace(old,new)))
    # Вернуть прежнюю копию pending: отмена пробы поздним closed оставит её навсегда.
    stale = production
    for old,new in (
        ('stable:1, first_status:1', 'stable:1, check_pending:1, first_status:1'),
        ('if (value) sv.state = SV_SILENT;', 'sv.flags.check_pending = 0; if (value) sv.state = SV_SILENT;'),
        ('sv.flags.status_wait = sv.flags.status_due = 0;', 'sv.flags.status_wait = sv.flags.status_due = sv.flags.check_pending = 0;'),
        ('if (sv.state == SV_PING || sv.state == SV_DNS)\n    comm_channel_probe(',
         'if ((sv.state == SV_PING || sv.state == SV_DNS) && !sv.flags.check_pending)\n    sv.flags.check_pending = comm_channel_probe('),
    ):
        assert stale.count(old) == 1, old
        stale = stale.replace(old,new)
    changes.append(('server-cancelled-probe-pending', stale))
    # Вернуть старую смену адреса после трёх отказов вместе с её счётчиком.
    old='if (!ch.call.expected) ch.address = !ch.flags.registered_session && !ch.address && comm_modem_reserve();'
    three=production.replace('uint8_t address, connections, retry_step, probe;',
                             'uint8_t address, address_failures, connections, retry_step, probe;')
    three=three.replace('ch.connections = 0; ch.flags.registered_session = 1;',
                        'ch.connections = ch.address_failures = 0; ch.flags.registered_session = 1;')
    assert three.count(old)==1
    three=three.replace(old,'if (!ch.call.expected && !ch.flags.registered_session && ++ch.address_failures >= 3) { ch.address_failures=0; if (comm_modem_reserve()) ch.address ^= 1u; }')
    changes.append(('server-address-after-three',three))
    for name,mutant in changes:
        with TemporaryDirectory(dir=base) as directory:
            folder=Path(directory);source=folder/'test.c';executable=folder/'test.exe'
            source.write_text(mutant,encoding='utf-8')
            compiler=subprocess.run([ZIG,'cc','-std=c99','-O2','-Wall','-Wextra','-Werror',str(source),'-o',str(executable)],capture_output=True)
            (base/(name+'-compile.log')).write_bytes(compiler.stdout+compiler.stderr)
            compiler.check_returncode()
            result=subprocess.run([str(executable)],capture_output=True,timeout=120)
            (base/(name+'.log')).write_bytes(result.stdout+result.stderr)
            assert result.returncode and b'FAIL' in result.stderr, ('undetected',name)
        print(name,'detected',flush=True)
    print('recover mutations:',len(changes),'/',len(changes),'detected')


if __name__ == '__main__':
    main()
