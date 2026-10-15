"""Доработка: Кряжев А., 2026. Мутации ответа на вызов и возврата данных, без платы."""
from pathlib import Path
import os
import subprocess
import tempfile
from temp_cleanup import TemporaryDirectory

from comm_server_host import unit, ZIG


def main():
    out = Path(os.environ.get('DOMOVOI_COMPARE_BUILD', Path(tempfile.gettempdir()) / 'domovoi-call-build')) / 'call-mutations'
    out.mkdir(parents=True, exist_ok=True)
    os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(out / 'zig-global')
    os.environ['ZIG_LOCAL_CACHE_DIR'] = str(out / 'zig-local')
    code = unit().replace('\r\n', '\n')
    faults = (
        ('SLM336-ATH', '{"AT+CHUP", NULL, NULL, 0, COMM_AT_CALL', '{"ATH", NULL, NULL, 0, COMM_AT_CALL'),
        ('CHUP-OK-with-live-call', 'if (r == COMM_AT_OK && !call.seen) call_finished();', 'if (r == COMM_AT_OK) call_finished();'),
        ('call-ACK-deadline', 'if (!active && ch.call.active) ch.tx_since = ch.now;', ''),
        ('hold-timer', 'now_ms - call.answered >= comm_timeouts[COMM_T_CALL_HOLD]', 'now_ms - call.answered >= comm_timeouts[COMM_T_STABLE]'),
        ('call-queue-delay', 'slm_side_issue(); return true;', 'return true;'),
        ('startup-preempts-call', 'if (slm.call_active && slm_side_allowed()) return;', ''),
        ('absolute-limit', 'now_ms - call.since >= comm_timeouts[COMM_T_CALL_LIMIT]', 'now_ms - call.since >= comm_timeouts[COMM_T_STABLE]'),
        ('ATA-timeout-before-resync', 'if (mode == COMM_AT_CALL) { comm_at_call_abort(now_ms); return; }', 'if (0) { comm_at_call_abort(now_ms); return; }'),
        ('redial-suppressed', 'if (call.state == CALL_IDLE || call.state == CALL_DONE) call_begin();', 'if (call.state == CALL_IDLE) call_begin();'),
        ('pending-after-hangup', 'if (comm_at_equal(p,n,"NO CARRIER")) {\n    call_stop();', 'if (comm_at_equal(p,n,"NO CARRIER")) {'),
        ('pending-deadline', 'now_ms - call.born >= comm_timeouts[COMM_T_CALL_QUIET]', '0'),
        ('prompt-guard', 'comm_at_busy() || comm_at_prompt()) return;', 'comm_at_busy()) return;'),
        ('fast-reopen', 'if (ch.call.expected) ch.reopen_pause = 0;', 'if (ch.call.expected) ch.reopen_pause = 30000;'),
        ('first-open-counted', 'if (!ch.call.expected) ++ch.connections;', '++ch.connections;'),
        ('pre-registration-unlimited', 'else if (had_socket && ch.connections >= 3)', 'else if (0)'),
        ('live-TCP-reopened', 'if (ch.call.lost) comm_channel_reopen(0);', 'comm_channel_reopen(0);'),
        ('expected-PDP-pause', 'if (comm_channel_expected() && comm_modem_status()->reason == SLM_ERR_PDP) rc_schedule(false, 0);', 'if (0) rc_schedule(false, 0);'),
        ('network-URC-ignored', 'if (registered == 1 || registered == 5)', 'if (0 && (registered == 1 || registered == 5))'),
        ('TCP-attempt-failure-free', 'if (!ch.call.expected) ++ch.connections;\n    ch.call.expected = ch.call.lost = 0;', 'if (!ch.call.expected) ++ch.connections;'),
    )
    changed = []
    for name, old, new in faults:
        assert old in code, (name, old)
        changed.append((name, code.replace(old, new)))
    old = 'if (call.caller(call.number)) call.pending = 2;'
    assert code.count(old) == 1
    mutant = code.replace(old, 'call.pending = 2;')
    old = 'if (r == COMM_AT_OK) { call.answered = now_ms;'
    assert old in mutant
    changed.append(('answer-before-action', mutant.replace(old, 'if (r == COMM_AT_OK) { call.caller(call.number); call.answered = now_ms;', 1)))
    with TemporaryDirectory(dir=out) as folder:
        folder = Path(folder)
        for name, mutant in changed:
            source, executable = folder / 'test.c', folder / 'test.exe'
            source.write_text(mutant, encoding='utf-8')
            result = subprocess.run([ZIG, 'cc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror', str(source), '-o', str(executable)], capture_output=True)
            (out / (name + '-compile.log')).write_bytes(result.stdout + result.stderr)
            assert result.returncode == 0, (name, 'mutation must compile')
            result = subprocess.run([str(executable)], capture_output=True, timeout=300)
            (out / (name + '.log')).write_bytes(result.stdout + result.stderr)
            assert result.returncode == 1 and b'FAIL' in (result.stdout + result.stderr), ('undetected', name)
            print(name, 'detected', flush=True)
    print('call mutations: %d/%d detected' % (len(changed), len(changed)))


if __name__ == '__main__':
    main()
