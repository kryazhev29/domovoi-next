"""Фото/CSFB: реальные UART/AT/SLM/канал/сервер/звонок, управляемые ответы модема."""
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import subprocess
import tempfile

from comm_server_host import ROOT, ZIG, unit

# Потоки канала без окна TX (5d0cf6e): с 19.10 он не собирается вместе с мостом Wi-Fi, поэтому
# сняты на 71e5a70 (фото 19 384 Б за 16 766 мс, смешанный 1 107 Б) и сверяются по sha256.
BEFORE = {'after.stream': 'e2cf885babcb65f1cd38fb89946f68cb1e0102c8a519714a46a5212a3eff4b3a',
          'after-mixed.stream': '584e661a76bda193ce686a13208293d450b9155d3159db973f2a0d5d8fb94775'}


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
                        '    if (lc_payload()) return;\n    uint8_t response[32], args[5]={0}; uint16_t n=0;')
    code = replace_once(code, 'static void transport_tick(unsigned increment)\n{',
                        'static void transport_tick(unsigned increment)\n{\n  lc_tick();')
    code = replace_once(code, 'static void user_parse_comand(uint8_t *p, unsigned channel)\n{', 'static bool lc_request21(uint8_t *p);\nstatic void user_parse_comand(uint8_t *p, unsigned channel)\n{')
    code = replace_once(code, '++server_commands;', '++server_commands;\n  if (lc_request21(p)) return;')
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


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--mutations', action='store_true')
    args = parser.parse_args()
    out = Path(os.environ.get('DOMOVOI_COMPARE_BUILD', Path(tempfile.gettempdir()) / 'domovoi-agent-build')) / 'link-call'
    out.mkdir(parents=True, exist_ok=True)
    os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(out / 'zig-global')
    os.environ['ZIG_LOCAL_CACHE_DIR'] = str(out / 'zig-local')
    code = model()
    current = run(code, out, 'current', ('test', str(out / 'after.stream')))
    run(code, out, 'current-mixed', ('mixed', str(out / 'after-mixed.stream')))
    for name, digest in BEFORE.items():
        assert hashlib.sha256((out / name).read_bytes()).hexdigest() == digest, name
    timing = {'after_ms': int(re.search(r' ms=(\d+)', current)[1]), 'identical_stream_bytes': (out / 'after.stream').stat().st_size}
    timing['resume_cursor'] = int(re.search(r'resume: cursor=(\d+)', current)[1])
    if os.environ.get('DOMOVOI_ELF'):
        subprocess.run([__import__('sys').executable, str(ROOT / 'Tests/asm/test_tx_window_photo.py'), str(timing['resume_cursor'])], check=True)
    timing['windows'] = {}
    for limit in (534, 1068, 2048, 3072, 8192):
        log = run(replace_once(code, 'TX_WINDOW = 4096', f'TX_WINDOW = {limit}'), out, f'window-{limit}', ('measure',))
        timing['windows'][limit] = int(re.search(r' ms=(\d+)', log)[1])
    grouped = code.replace('#define LC_GROUP 1u', '#define LC_GROUP 2u').replace('frame[1040]', 'frame[1068]')
    log = run(grouped, out, 'two-frames', ('measure', str(out / 'grouped.stream')))
    assert (out / 'after.stream').read_bytes() == (out / 'grouped.stream').read_bytes()
    timing['grouped_ms'] = int(re.search(r' ms=(\d+)', log)[1])
    timing['faster_uart'] = {}
    for baud in (230400, 460800):
        log = run(replace_once(code, '#define LC_BAUD 115200u', f'#define LC_BAUD {baud}u'), out, f'baud-{baud}', ('measure',))
        timing['faster_uart'][baud] = int(re.search(r' ms=(\d+)', log)[1])
    (out / 'timing.json').write_text(json.dumps(timing, indent=2), encoding='utf-8')

    if args.mutations:
        faults = [
            ('first-timeout-reopen', '++ch.ack_timeouts < 3', '++ch.ack_timeouts < 1'),
            ('stop-and-wait', 'ch.sent - ch.acked + ch.frame_count <= TX_WINDOW', 'ch.sent == ch.acked'),
            ('acquire-renews-deadline', 'if (ch.sent == ch.acked) { ch.tx_since = ch.now; ch.diagnostic = 0; }',
             'ch.tx_since = ch.now; if (ch.sent == ch.acked) ch.diagnostic = 0;'),
            ('send-fail-reopens', 'op == OP_SEND && result == COMM_AT_SEND_FULL', 'op == OP_SEND && result == COMM_AT_SINK_ERROR'),
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
