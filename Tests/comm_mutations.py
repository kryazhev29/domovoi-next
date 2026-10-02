"""Доработка: Кряжев А., 2026. Прежние мутации UART/AT/модема после табличного переноса."""
from pathlib import Path
import os
import subprocess
import tempfile
from comm_at_host import ROOT, ZIG, unit as at_unit
from comm_slm_host import unit as modem_unit
from comm_server_host import unit as channel_unit


def uart_unit():
    source = lambda name: (ROOT / name).read_bytes().decode('cp1251')
    model = source('Tests/test_comm_uart.c')
    return model.replace('/* HEADER */', source('Modules/comm_uart.h')).replace(
        '/* PRODUCTION */', '\n'.join(line for line in source('Modules/comm_uart.c').splitlines()
                                      if not line.startswith('#include')))


def run():
    out = ROOT / 'Release/comm-stage4_5/mutations'
    out.mkdir(parents=True, exist_ok=True)
    os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(out / 'zig-global')
    os.environ['ZIG_LOCAL_CACHE_DIR'] = str(out / 'zig-local')
    sources = dict(uart=uart_unit(), at=at_unit(), modem=modem_unit(), channel=channel_unit())
    sources = {name: text.replace('\r\n', '\n') for name, text in sources.items()}
    cases = (
        ('uart', 'rts-stop-boundary', '- 1u - used < 128u', '- 1u - used <= 128u'),
        ('uart', 'rts-resume-boundary', 'else if (used < 128u)', 'else if (used <= 128u)'),
        ('uart', 'rts-not-updated-in-irq', '        rts_update();', ''),
        ('uart', 'consume-unmasked', '  __disable_irq();\n  __DMB();\n  rx_read', '  __DMB();\n  rx_read'),
        ('uart', 'abort-keeps-pointer', '  tx_data = NULL;\n  rx_write', '  rx_write'),
        ('uart', 'tx-not-atomic', '  __disable_irq();\n  tx_data', '  tx_data'),
        ('at', 'final-prefix', 'count == n && !memcmp(data, names, n)', 'count >= n && !memcmp(data, names, n)'),
        ('at', 'line-off-by-one', 'line_count >= sizeof(line)', 'line_count + 1u >= sizeof(line)'),
        ('at', 'late-qird-as-text', '(state == RUN || (state >= DRAIN && state <= SYNC_WAIT))\n      && (mode == COMM_AT_SLM_QIRD', 'state == RUN\n      && (mode == COMM_AT_SLM_QIRD'),
        ('at', 'late-body-to-sink', 'state != RUN ? count : handlers.binary(view, count)', 'handlers.binary(view, count)'),
        ('at', 'poll-unlimited', 'if (count > budget) count = budget;', ''),
        ('at', 'callback-start', 'return state != IDLE || in_poll;', 'return state != IDLE;'),
        ('at', 'old-OK-sync', 'state == SYNC_WAIT && sync_seen && code == COMM_AT_OK', 'state == SYNC_WAIT && code == COMM_AT_OK'),
        ('channel', 'direct-push', ',0,0\\0"', ',0,1\\0"'),
        ('modem', 'QIACT-20s', '150000, /* COMM_T_QIACT */', '20000, /* COMM_T_QIACT */'),
        ('channel', 'TCP-before-URC', 'else if (!ch.flags.open_seen) return;', 'else if (!ch.flags.open_ack) return;'),
        ('channel', 'drop-QIRD-followup', 'ch.flags.recv = ch.flags.recv || ch.bytes;', 'ch.flags.recv = 0;'),
        ('modem', 'ignore-manual-APN', 'if (slm_text(SLM_CFG_APN)[0]) return slm_text(SLM_CFG_APN);', 'if (slm_text(SLM_CFG_APN)[0]) return "";'),
        ('channel', 'grow-final-TCP-time', 'if (slm.attempt.recording) {', 'if (slm.attempt.status.state != SLM_IDLE) {'),
        ('channel', 'TCP-time-overwritten-by-read', 'slm.attempt.recording = next < SLM_STEP_COUNT;', 'slm.attempt.recording = 1;'),
    )
    for group, label, old, new in cases:
        text = sources[group]
        assert old in text, label
        with tempfile.TemporaryDirectory(dir=out) as tmp:
            tmp = Path(tmp)
            (tmp / 'test.c').write_text(text.replace(old, new), encoding='utf-8', newline='\n')
            subprocess.run([ZIG, 'cc', '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                            str(tmp / 'test.c'), '-o', str(tmp / 'test.exe')], check=True,
                           capture_output=True)
            result = subprocess.run([str(tmp / 'test.exe')], capture_output=True, timeout=120)
            (out / (label + '.log')).write_bytes(result.stdout + result.stderr)
            assert result.returncode and b'FAIL ' in result.stderr, ('undetected', label)
        print(label, 'detected', flush=True)
    print('comm mutations:', len(cases), '/', len(cases), 'detected')


if __name__ == '__main__':
    run()
