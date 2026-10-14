"""Фото/CSFB: реальные UART/AT/SLM/канал/сервер/звонок, управляемые ответы модема."""
import argparse
import json
import os
import re
from pathlib import Path
import subprocess
import tempfile

from comm_server_host import ROOT, ZIG, source as project_source, unit


def replace_once(code, old, new):
    assert code.count(old) == 1, (code.count(old), old[:100])
    return code.replace(old, new)


def model():
    code = unit().replace('\r\n', '\n')
    fixture = (ROOT / 'Tests/test_link_call.c').read_text(encoding='utf-8')
    hooks, tests = fixture.split('/* СЦЕНАРИИ */')
    code = replace_once(code, 'static void transport_tx(void)', hooks + '\nstatic void transport_tx(void)')
    code = replace_once(code, '  snprintf(last_command,sizeof(last_command),"%s",cmd);',
                        '  snprintf(last_command,sizeof(last_command),"%s",cmd);\n  if (lc_command(cmd)) return;')
    code = replace_once(code, '    uint8_t response[32], args[5]={0}; uint16_t n=0;',
                        '    lc_payload();\n    uint8_t response[32], args[5]={0}; uint16_t n=0;')
    code = replace_once(code, 'static void transport_tick(unsigned increment)\n{',
                        'static void transport_tick(unsigned increment)\n{\n  lc_tick();')
    # Существующие регрессии исполняются отдельно; ссылка сохраняет их проверку компилятором.
    code = replace_once(code, 'int main(void)', 'static int previous_main(void)')
    return code + '\n' + tests


def run(code, out, name, args=(), fail=False):
    source, exe = out / (name + '.c'), out / (name + '.exe')
    source.write_text(code, encoding='utf-8')
    result = subprocess.run([ZIG, 'cc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                             str(source), '-o', str(exe)], capture_output=True)
    (out / (name + '-compile.log')).write_bytes(result.stdout + result.stderr)
    assert result.returncode == 0, (name, result.stderr.decode(errors='replace')[-1600:])
    result = subprocess.run([str(exe), *args], capture_output=True, timeout=60)
    log = (result.stdout + result.stderr).decode(errors='replace')
    (out / (name + '.log')).write_text(log, encoding='utf-8')
    assert (result.returncode == 1 and 'FAIL' in log) if fail else result.returncode == 0, (name, log[-2000:])
    print(name + ': ' + ('detected' if fail else log.strip()), flush=True)
    return log


def pipeline_estimate():
    """Оптимистичная модель окна 1068 Б: без ошибок/вызова/Flash, ACK виден лишь через опрос.
    Это оценка выгоды, не реализация автомата и не доказательство безопасности окна.
    """
    sizes = [min(512, 18570 - offset) + 22 for offset in range(0, 18570, 512)]
    now = sent = known = actual = peak = polls = 0
    pending = []
    for size in sizes + [0]:
        while (sent - known + size > 1068) or (size == 0 and sent != known):
            now += 100
            actual += sum(count for due, count in pending if due <= now)
            pending = [(due, count) for due, count in pending if due > now]
            known = actual
            now += 20
            polls += 1
        if size:
            wire = (size * 10000 + 115199) // 115200
            pending.append((now + 20 + wire + 50, size))
            now += 20 + wire + 6
            sent += size
            peak = max(peak, sent - known)
    assert sent == known == 19384 and peak <= 1068
    return {'ms': now, 'limit': 1068, 'peak': peak, 'polls': polls, 'acked': known}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--mutations', action='store_true')
    args = parser.parse_args()
    out = Path(os.environ.get('DOMOVOI_COMPARE_BUILD', Path(tempfile.gettempdir()) / 'domovoi-agent-build')) / 'link-call'
    out.mkdir(parents=True, exist_ok=True)
    os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(out / 'zig-global')
    os.environ['ZIG_LOCAL_CACHE_DIR'] = str(out / 'zig-local')
    code = model()
    current = run(code, out, 'current')
    # Та же модель, настоящие два исходника из указанной пользователем базы.
    baseline = code
    for name in ('Modules/comm_channel.c', 'User/comm_timeouts.c'):
        old = project_source(name).replace('\r\n', '\n')
        original = subprocess.check_output(['git', 'show', '2e42207:' + name], cwd=ROOT).decode('utf-8').replace('\r\n', '\n')
        original = re.sub(r'^\s*#include [\"<][^\n]*domovoi_diag\.h[\">]\s*$', '', original, flags=re.M)
        if name.endswith('comm_channel.c'):
            old = '\n'.join(line for line in old.splitlines() if not line.startswith('#include'))
            original = '\n'.join(line for line in original.splitlines() if not line.startswith('#include'))
        else:
            old = '\n'.join(line for line in old.splitlines() if not line.startswith('#include "'))
            original = '\n'.join(line for line in original.splitlines() if not line.startswith('#include "'))
        baseline = replace_once(baseline, old, original)
    before = run(baseline, out, 'baseline', ('measure',))
    run(baseline, out, 'baseline-call', ('call-baseline',))
    timing = {'before_ms': int(re.search(r' ms=(\d+)', before)[1]),
              'after_ms': int(re.search(r' ms=(\d+)', current)[1]), 'pipeline_estimate': pipeline_estimate()}
    (out / 'timing.json').write_text(json.dumps(timing, indent=2), encoding='utf-8')
    print('pipeline estimate: ' + json.dumps(timing['pipeline_estimate']), flush=True)
    if args.mutations:
        faults = [
            ('first-timeout-reopen', '++ch.ack_timeouts < 3', '++ch.ack_timeouts < 1'),
            ('poll-1000ms', '100, /* Опрос подтверждённых байтов */', '1000, /* Опрос подтверждённых байтов */'),
            ('no-timeout-limit', '++ch.ack_timeouts < 3', '++ch.ack_timeouts < 255'),
            ('counter-not-reset', '    ch.ack_timeouts = 0;', '    /* мутация: успех не очищает счётчик */'),
            ('no-new-session-reset', 'ch.state = CH_OPEN; ch.ack_timeouts = 0;', 'ch.state = CH_OPEN;'),
            ('late-OK-finishes-sync', 'state == SYNC_WAIT && sync_seen && code == COMM_AT_OK', 'state == SYNC_WAIT && code == COMM_AT_OK'),
            ('unacked-deadline', 'now_ms - ch.tx_since > comm_timeouts[COMM_T_UNACKED]', 'now_ms - ch.tx_since > comm_timeouts[COMM_T_STABLE]'),
        ]
        for name, old, new in faults:
            run(replace_once(code, old, new), out, name, fail=True)
        (out / 'mutations.json').write_text(json.dumps({'detected': len(faults), 'total': len(faults), 'names': [f[0] for f in faults]}, indent=2), encoding='utf-8')


if __name__ == '__main__':
    main()
