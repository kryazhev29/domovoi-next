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
    tick = 'void SysTick_Handler(void)' + source('stm32_it.c').split('void SysTick_Handler(void)', 1)[1].split('\n}', 1)[0] + '\n}'
    return model.replace('/* TICK */', tick).replace('/* HEADER */', source('Modules/comm_uart.h')).replace(
        '/* PRODUCTION */', '\n'.join(line for line in source('Modules/comm_uart.c').splitlines()
                                      if not line.startswith('#include')))


def run():
    out = ROOT / 'Release/comm-stage4_6/final'
    out.mkdir(parents=True, exist_ok=True)
    os.environ['ZIG_GLOBAL_CACHE_DIR'] = str(out / 'zig-global')
    os.environ['ZIG_LOCAL_CACHE_DIR'] = str(out / 'zig-local')
    sources = dict(uart=uart_unit(), at=at_unit(), modem=modem_unit(), channel=channel_unit())
    sources = {name: text.replace('\r\n', '\n') for name, text in sources.items()}
    cases = (
        ('channel', 'ping-lost-during-ack', 'if (ch.probe_active == COMM_CH_PING) {', 'if (ch.operation != OP_ACK && ch.probe_active == COMM_CH_PING) {'),
        ('channel', 'dns-lost-during-ack', 'else if (ch.probe_active == COMM_CH_DNS && comm_at_prefix', 'else if (ch.operation != OP_ACK && ch.probe_active == COMM_CH_DNS && comm_at_prefix'),
        ('at', 'idle-fragment-never-times-out', 'if (state == IDLE && (line_count || cr) && expired(now_ms, recovery))', 'if (0)'),
        ('at', 'idle-fragment-old-timestamp', 'if (state == IDLE && !line_count && !cr) started = now_ms;', ''),
        ('modem', 'busy-start-is-fatal', 'if (result == COMM_AT_BUSY) {', 'if (0) {'),
        ('modem', 'busy-start-unbounded', 'if (slm.attempt.now - slm.attempt.next >= comm_timeouts[COMM_T_AT_SIMPLE]) slm_fail(SLM_ERR_TIMEOUT);', 'if (0) slm_fail(SLM_ERR_TIMEOUT);'),
        ('channel', 'data-echo-left-enabled', '"QISDE=0\\0"', '"QISDE=1\\0"'),
        ('uart', 'idle-steals-rx-byte', 'status & (USART_SR_ORE | USART_SR_FE | USART_SR_NE)', 'status & (USART_SR_IDLE | USART_SR_ORE | USART_SR_FE | USART_SR_NE)'),
        ('modem', 'power-noise-not-cleared', 'if (p == COMM_PWR_READY) comm_uart_init(115200);', 'if (0) comm_uart_init(115200);'),
        ('modem', 'reject-valid-current-rat', 'values[0] <= 8)', 'values[0] <= 5)'),
        ('modem', 'deny-other-network-domain', 'if (values[1] == 1 || values[1] == 5)', 'if (values[1] == 3) slm_fail(SLM_ERR_NETWORK); if (values[1] == 1 || values[1] == 5)'),
        ('uart', 'rts-stop-boundary', 'used > sizeof(rx) - 1u - 128u', 'used >= sizeof(rx) - 1u - 128u'),
        ('uart', 'rts-resume-boundary', 'else if (used < 128u)', 'else if (used <= 128u)'),
        ('uart', 'rts-not-updated-in-irq', 'void DMA1_Channel3_IRQHandler(void)\n{\n  comm_uart_tick();\n}', 'void DMA1_Channel3_IRQHandler(void)\n{\n}'),
        ('uart', 'no-periodic-tick', '  comm_uart_tick(); /* DMA/RTS', '  /* DMA/RTS'),
        ('uart', 'wrong-CNDTR-index', 'sizeof(rx) - DMA1_Channel3->CNDTR', 'DMA1_Channel3->CNDTR'),
        ('uart', 'overrun-undetected', '(delta && used >= sizeof(rx))', '0'),
        ('uart', 'whole-lap-undetected', 'delta += sizeof(rx);', 'delta += 0;'),
        ('uart', 'consume-unmasked', '  __disable_irq();\n  comm_uart_tick();\n  uint16_t view', '  comm_uart_tick();\n  uint16_t view'),
        ('uart', 'abort-keeps-pointer', '  uart_state.tx_ptr = NULL;\n  uart_state.rx_write', '  uart_state.rx_write'),
        ('uart', 'tx-not-atomic', '  __disable_irq();\n  uart_state.tx_ptr', '  uart_state.tx_ptr'),
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
        # Этап 6, звонок без ответа.
        ('channel', 'call-command-after-prompt', 'comm_at_busy() || comm_at_prompt()) return;', 'comm_at_busy()) return;'),
        ('channel', 'call-second-open', 'call.state != CALL_NUMBER && (call.state <= CALL_WAIT_NUMBER || strcmp(number, call.number))', 'call.state != CALL_NUMBER'),
        ('channel', 'call-hidden-number-not-hung-up', 'comm_timeouts[COMM_T_CALL_NUMBER]) call.state = CALL_HANGUP;', 'comm_timeouts[COMM_T_CALL_NUMBER]) call.state = CALL_DONE;'),
        ('channel', 'sms-long-body-fault', 'if (!pdu_body || state != RUN) { fault(COMM_AT_LINE_OVERFLOW); return; }', 'if (1) { fault(COMM_AT_LINE_OVERFLOW); return; }'),
        ('channel', 'sms-bad-cell-kept', 'msg.erase = msg.queue[0];', 'msg.erase = r == COMM_AT_OK ? msg.queue[0] : 0;'),
        ('channel', 'sms-udh-fill-bits', 'msg.skip = (uint8_t)(((o + 1u) * 8u + 6u) / 7u);', 'msg.skip = (uint8_t)(o + 1u);'),
        ('channel', 'sms-cmgs-length-with-smsc', 'uint16_t tpdu = (uint16_t)(msg.out_count / 2u - 1u);', 'uint16_t tpdu = (uint16_t)(msg.out_count / 2u);'),
        ('channel', 'sms-cms-error-not-final', '&& !comm_at_prefix(line, line_count, "+CMS ERROR:")', ''),
        ('channel', 'ussd-long-line-fault', '    if (comm_at_prefix(line, line_count, "+CUSD:")) return;\n', ''),
        ('channel', 'ussd-ucs2-ignored', '  if (dcs == 72u) {', '  if (0) {'),
        ('channel', 'ussd-first-number-only', 'if (f) { s = f; break; }', 'if (0) { s = f; break; }'),
        ('channel', 'ussd-failure-waits-a-day', 'bal.next = bal.now + comm_timeouts[COMM_T_USSD_RETRY];', 'bal.next = bal.now + comm_timeouts[COMM_T_DAY];'),
        ('channel', 'ussd-menu-left-open', 'if (bal.menu && comm_modem_side(&bal_cancel))', 'if (0 && comm_modem_side(&bal_cancel))'),
        ('channel', 'volte-unsupported-retried', 'call.volte_modem = VOLTE_UNSUPPORTED;', 'call.volte_modem = VOLTE_UNKNOWN;'),
        ('channel', 'volte-during-call', 'call.state != CALL_IDLE || call.volte_modem', 'call.volte_modem'),
        ('channel', 'volte-written-every-time', 'call.volte_modem != want ? &volte_set[want] : NULL', '&volte_set[want]'),
        ('channel', 'volte-new-start-not-checked', 'call.session = session; call.volte_modem = VOLTE_UNKNOWN;', 'call.session = session;'),
        ('channel', 'call-side-kept-after-restart', '  memset(&slm.attempt, 0, sizeof(slm.attempt));\n  slm_side_cancel();', '  memset(&slm.attempt, 0, sizeof(slm.attempt));'),
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
