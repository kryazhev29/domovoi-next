"""Доработка: Кряжев А., 2026. Мутации ответа на вызов и возврата данных, без платы."""
from pathlib import Path
import os
import subprocess
import tempfile

from comm_server_host import unit, ZIG


def main():
    out = Path(os.environ.get('DOMOVOI_COMPARE_BUILD', Path(tempfile.gettempdir()) / 'domovoi-call-build')) / 'call-mutations'
    out.mkdir(parents=True, exist_ok=True)
    os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(out / 'zig-global')
    os.environ['ZIG_LOCAL_CACHE_DIR'] = str(out / 'zig-local')
    code = unit().replace('\r\n', '\n')
    faults = (
        ('hold-timer', 'now_ms - call.answered >= comm_timeouts[COMM_T_CALL_HOLD]', 'now_ms - call.answered >= comm_timeouts[COMM_T_STABLE]'),
        ('call-queue-delay', 'slm_side_issue(); return true;', 'return true;'),
        ('startup-preempts-call', 'if (slm.call_phase == 1u && slm_side_allowed()) goto end;', ''),
        ('absolute-limit', 'now_ms - call.since >= comm_timeouts[COMM_T_CALL_LIMIT]', 'now_ms - call.since >= comm_timeouts[COMM_T_STABLE]'),
        ('ATA-timeout-before-resync', 'if (mode == COMM_AT_CALL) { comm_at_call_abort(now_ms); goto end; }', 'if (0) { comm_at_call_abort(now_ms); goto end; }'),
        ('repeat-RING', 'if (call.state == CALL_IDLE) call_begin();', 'if (call.state == CALL_IDLE) call_begin(); else if (call.state == CALL_DONE) call.state = CALL_ANSWER;'),
        ('prompt-guard', 'comm_at_busy() || comm_at_prompt()) return;', 'comm_at_busy()) return;'),
        ('fast-reopen', 'ch.operation_since = ch.now; ch.reopen_pause = 0;', 'ch.operation_since = ch.now; ch.reopen_pause = 30000;'),
        ('expected-loss-count', 'if (ch.call_stage && ch.call_stage != DATA_REGISTER) { channel_call_loss(); return; }', 'if (0) { channel_call_loss(); return; }'),
        ('PDP-context', 'v[0] == 1 && v[1] == 1 && v[2] >= 1', 'v[0] > 0 && v[1] == 1 && v[2] >= 1'),
        ('TCP-context', 'v[2] == 2 && v[3] == 1;', 'v[2] == 2 && v[3] > 0;'),
        ('registration-URC-leaks-answer', 'if (!urc) ch.answer = 1;', 'ch.answer = 1;'),
        ('network-URC', 'if (ch.call_stage && kind == COMM_AT_URC) channel_call_net(p,n,true);', 'if (0) channel_call_net(p,n,true);'),
    )
    changed = []
    for name, old, new in faults:
        assert old in code, (name, old)
        changed.append((name, code.replace(old, new)))
    old = 'if (call.caller) call.caller(call.number);'
    assert code.count(old) == 1
    mutant = code.replace(old, '')
    old = 'if (r == COMM_AT_OK) { call.answered = now_ms;'
    assert old in mutant
    changed.append(('answer-before-action', mutant.replace(old, 'if (r == COMM_AT_OK) { if (call.caller) call.caller(call.number); call.answered = now_ms;', 1)))
    with tempfile.TemporaryDirectory(dir=out) as folder:
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
