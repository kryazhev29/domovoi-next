"""Фото/CSFB: реальные UART/AT/SLM/канал/сервер/звонок, управляемые ответы модема."""
import argparse
import json
import os
import re
from pathlib import Path
import subprocess
import tempfile

from comm_server_host import ROOT, ZIG, function, source as project_source, unit


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
    # Те же модель UART/ACK и кадры; автомат канала из пользовательской базы.
    old = project_source('Modules/comm_channel.c').replace('\r\n', '\n')
    old = '\n'.join(line for line in old.splitlines() if not line.startswith('#include'))
    original = subprocess.check_output(['git', 'show', '5d0cf6e:Modules/comm_channel.c'], cwd=ROOT).decode('utf-8')
    original = '\n'.join(line for line in original.splitlines() if not line.startswith('#include'))
    baseline = replace_once(code, old, original)
    # База 5d0cf6e знает только GSM: современный Wi-Fi не принадлежит её автомату.
    # Убираем его реализацию и только Wi-Fi-сценарии из baseline, не из current.
    wifi = project_source('Modules/comm_wifi.inc').replace('\r\n', '\n')
    wifi = '\n'.join(line for line in wifi.splitlines() if not line.startswith('#include'))
    baseline = replace_once(baseline, wifi, '')
    fixture = project_source('Tests/test_comm_channel.c').replace('\r\n', '\n')
    begin = fixture.index('/* Стенд моста DR164')
    end = fixture.index('/* Опрос сигнала:', begin)
    baseline = replace_once(baseline, fixture[begin:end], '')
    baseline = replace_once(baseline, ' channel_wifi_tests();', '')
    baseline = replace_once(baseline, function(baseline, 'wifi_server_tests'), '')
    baseline = replace_once(baseline, ' wifi_server_tests();', '')
    baseline = baseline.replace('ch.sent=ch.acked=tcp_sent=tcp_acked=', 'tcp_sent=tcp_acked=').replace('ch.acked==ch.sent', 'tcp_acked==tcp_sent').replace('ch.sent==ch.acked', 'tcp_sent==tcp_acked')
    baseline = baseline.replace('COMM_CH_CLOSED, COMM_CH_PROBE_DONE', 'COMM_CH_CLOSED, COMM_CH_TX_DONE, COMM_CH_PROBE_DONE')
    before = run(baseline, out, 'baseline', ('measure', str(out / 'before.stream')))
    assert (out / 'before.stream').read_bytes() == (out / 'after.stream').read_bytes()
    run(baseline, out, 'baseline-mixed', ('mixed', str(out / 'before-mixed.stream')))
    run(code, out, 'current-mixed', ('mixed', str(out / 'after-mixed.stream')))
    assert (out / 'before-mixed.stream').read_bytes() == (out / 'after-mixed.stream').read_bytes()
    timing = {'before_ms': int(re.search(r' ms=(\d+)', before)[1]),
              'after_ms': int(re.search(r' ms=(\d+)', current)[1]), 'identical_stream_bytes': (out / 'after.stream').stat().st_size}
    timing['resume_cursor'] = int(re.search(r'resume: cursor=(\d+)', current)[1])
    if os.environ.get('DOMOVOI_ELF'):
        subprocess.run([__import__('sys').executable, str(ROOT / 'Tests/asm/test_tx_window_photo.py'), str(timing['resume_cursor'])], check=True)
    timing['windows'] = {}
    for limit in (534, 1068, 2048, 3072, 8192):
        log = run(replace_once(code, 'TX_WINDOW = 4096', f'TX_WINDOW = {limit}'), out, f'window-{limit}', ('measure',))
        timing['windows'][limit] = int(re.search(r' ms=(\d+)', log)[1])
    grouped = code.replace('#define LC_GROUP 1u', '#define LC_GROUP 2u').replace('frame[1040]', 'frame[1068]')
    log = run(grouped, out, 'two-frames', ('measure', str(out / 'grouped.stream')))
    assert (out / 'before.stream').read_bytes() == (out / 'grouped.stream').read_bytes()
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
            ('acquire-renews-deadline', 'if (ch.sent == ch.acked) ch.tx_since = ch.now;', 'ch.tx_since = ch.now;'),
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
