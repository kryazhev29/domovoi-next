"""Доработка: Кряжев А., 2026. Мутации отбоя без ответа и восстановления данных, без платы."""
from pathlib import Path
import os
import subprocess
import tempfile
import sys
from temp_cleanup import TemporaryDirectory

from comm_server_host import unit, ZIG


def main():
    out = Path(os.environ.get('DOMOVOI_COMPARE_BUILD', Path(tempfile.gettempdir()) / 'domovoi-call-build')) / 'call-mutations'
    out.mkdir(parents=True, exist_ok=True)
    os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(out / 'zig-global')
    os.environ['ZIG_LOCAL_CACHE_DIR'] = str(out / 'zig-local')
    code = unit().replace('\r\n', '\n')
    faults = (
        ('answer-forbidden', '{"ATH", NULL, NULL, 0, COMM_AT_CALL, COMM_T_CALL_HANGUP}', '{"ATA", NULL, NULL, 0, COMM_AT_CALL, COMM_T_CALL_HANGUP}'),
        ('access-skipped', 'call.caller(call.number);', '(void)call.number;'),
        ('foreign-number', 'call.caller(call.number);', 'call.caller("+79161234567");'),
        ('number-order', 'if (call.state == CALL_IDLE) call_begin();\n  if (call.state != CALL_WAIT_NUMBER)', 'if (call.state != CALL_WAIT_NUMBER)'),
        ('number-deadline', 'now_ms - call.since < comm_timeouts[COMM_T_CALL_NUMBER]', 'now_ms - call.since < comm_timeouts[COMM_T_STABLE]'),
        ('missing-number-action', 'case CALL_WAIT_NUMBER:', 'case CALL_WAIT_NUMBER: call.caller("+79161234567");'),
        ('repeat-RING', 'if (call.state == CALL_IDLE) call_begin();', 'if (call.state == CALL_IDLE || call.state == CALL_DONE) call_begin();'),
        ('repeat-CLIP', 'if (call.state != CALL_WAIT_NUMBER) return;', 'if (call.state != CALL_WAIT_NUMBER && call.state != CALL_DONE) return;'),
        ('late-CLIP', 'CALL_DONE && call.number[0] && strcmp(number, call.number)', 'CALL_DONE && strcmp(number, call.number)'),
        ('access-not-ready', 'if (!access_ready) break;', 'if (0 && !access_ready) break;'),
        ('FW-blocks-call', 'comm_call_poll(sim_now, call_access_ready);', 'if (!(device.status & (1u << 11))) comm_call_poll(sim_now, call_access_ready);'),
        ('call-queue-delay', 'slm_side_issue(); return true;', 'return true;'),
        ('startup-preempts-call', 'if (slm.call_active && slm_side_allowed()) return;', ''),
        ('ATH-timeout-before-resync', 'if (mode == COMM_AT_CALL) { comm_at_call_abort(now_ms); return; }', 'if (0) { comm_at_call_abort(now_ms); return; }'),
        ('ATH-error-ignored', 'else if (r != UINT8_MAX) call.state = CALL_ABORT;', 'else if (r != UINT8_MAX) call_finished();'),
        ('ATH-CME-ignored', 'else if (r != UINT8_MAX) call.state = CALL_ABORT;', 'else if (r != UINT8_MAX && r != COMM_AT_CME) call.state = CALL_ABORT;'),
        ('prompt-guard', 'comm_at_busy() || comm_at_prompt()) return;', 'comm_at_busy()) return;'),
        ('fast-reopen', 'if (ch.call.expected) ch.reopen_pause = 0;', 'if (ch.call.expected) ch.reopen_pause = 30000;'),
        ('first-open-counted', 'if (!ch.call.expected) ++ch.connections;', '++ch.connections;'),
        ('live-TCP-reopened', 'if (ch.call.lost) comm_channel_reopen(0);', 'comm_channel_reopen(0);'),
        ('expected-PDP-pause', 'if (comm_channel_expected() && comm_modem_status()->reason == SLM_ERR_PDP) rc_schedule(false, 0);', 'if (0) rc_schedule(false, 0);'),
        ('network-URC-ignored', 'if ((comm_at_parse(p,n,"+CGREG:\\005",values)', 'if (0 && (comm_at_parse(p,n,"+CGREG:\\005",values)'),
        ('TCP-attempt-failure-free', 'if (!ch.call.expected) ++ch.connections;\n    ch.call.expected = ch.call.lost = 0;', 'if (!ch.call.expected) ++ch.connections;'),
    )
    changed = []
    for name, old, new in faults:
        assert old in code, (name, old)
        changed.append((name, code.replace(old, new)))
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
    sys.path.insert(0, str(Path(__file__).resolve().parent / 'asm'))
    from firmware_suite import select_image
    select_image()
    from test_call_pause import run
    run()
    try:
        run(ignore_pause=True)
    except AssertionError as error:
        assert str(error) == 'second call during output pause', error
        print('output-pause detected (IAR ELF)', flush=True)
    else:
        raise AssertionError('undetected output-pause')
    print('call mutations: %d/%d detected' % (len(changed) + 1, len(changed) + 1))


if __name__ == '__main__':
    main()
