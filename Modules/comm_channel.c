/* Доработка: Кряжев А., 2026. Слот принадлежит каналу до TCP ACK/отмены. */
#include "comm_channel.h"
#include "modem_slm336.h"
#include "comm_uart.h"
#include "comm_timeouts.h"
#include <string.h>
#include <stdio.h>
static uint8_t frame[1040];
static struct {
  void (*event)(uint8_t, uint8_t);
  uint32_t now, tx_since, operation_since, ack_next, reopen_pause, dns_since, probe_since;
  uint16_t frame_count;
  uint8_t state, tx_state, operation, done, result, answer;
  uint8_t address, address_failures, connections, retry_step, probe;
  uint8_t probe_active, probe_answer, generation, probe_generation;
  struct { uint8_t open_seen:1, open_ok:1, urgent:1, dns_fence:1, registered_session:1, peer_closed:1; } flags;
} ch;
enum { CH_INITIAL, CH_OPEN, CH_CLOSE, CH_CLOSING, CH_VERIFY, CH_PAUSE, CH_OPENING, CH_FAULT };
enum { TX_FREE, TX_BUILD, TX_QUEUED, TX_PROMPT, TX_PAYLOAD, TX_SENDING, TX_ACK };
enum { OP_NONE, OP_PROMPT, OP_SEND, OP_ACK, OP_CLOSE, OP_VERIFY, OP_OPEN, OP_PING, OP_DNS };
static void channel_emit(uint8_t kind, uint8_t value)
{ if (ch.event) ch.event(kind, value); }
static bool channel_prefix(const uint8_t *p, uint16_t n, const char *s)
{ size_t len = strlen(s); return n >= len && !memcmp(p, s, len); }
static bool channel_num(const uint8_t *p, uint16_t n, uint16_t *pos, uint32_t *out)
{
  uint32_t v = 0; while (*pos < n && p[*pos] == ' ') ++*pos;
  uint16_t begin = *pos;
  while (*pos < n && p[*pos] >= '0' && p[*pos] <= '9') {
    uint8_t d = p[(*pos)++] - '0';
    if (v > (UINT32_MAX - d) / 10u) return false;
    v = v * 10u + d;
  }
  *out = v; return *pos != begin;
}
/* IPv4 целиком; неудачная проба не двигает позицию числового итога. */
static bool channel_ip(const uint8_t *p, uint16_t n, uint16_t *pos)
{
  uint32_t v; uint16_t at = *pos;
  bool quoted = at < n && p[at] == '"';
  if (quoted) ++at;
  for (unsigned i = 0; i < 4; ++i) {
    if (!channel_num(p, n, &at, &v) || v > 255u) return false;
    if (i != 3 && (at >= n || p[at++] != '.')) return false;
  }
  if (quoted && (at >= n || p[at++] != '"')) return false;
  *pos = at; return true;
}
static void channel_at_event(uint8_t kind, const uint8_t *p, uint16_t n, uint16_t detail)
{
  uint16_t pos; uint32_t a, b, c;
  if (kind == COMM_AT_NO_ANSWER || kind == COMM_AT_STREAM_ERROR) {
    ch.state = CH_FAULT; channel_emit(COMM_CH_FAULT, (uint8_t)detail); return;
  }
  if (kind == COMM_AT_DONE) { ch.done = 1; ch.result = (uint8_t)detail; return; }
  if (!p) return;
  if (channel_prefix(p, n, "+QIURC: \"closed\",")) {
    pos = sizeof("+QIURC: \"closed\",") - 1u;
    if (channel_num(p, n, &pos, &a) && pos == n && a == 0) {
      ch.flags.peer_closed = 1; comm_channel_reopen(0);
    }
    return;
  }
  if (ch.operation == OP_VERIFY && channel_prefix(p, n, "+QISTATE:")) ch.answer = 1;
  if (ch.operation == OP_OPEN && channel_prefix(p, n, "+QIOPEN:")) {
    pos = sizeof("+QIOPEN:") - 1u;
    if (channel_num(p, n, &pos, &a) && pos < n && p[pos++] == ','
        && channel_num(p, n, &pos, &b) && pos == n && a == 0) {
      ch.flags.open_seen = 1; ch.flags.open_ok = b == 0;
    }
  } else if (ch.operation == OP_ACK && channel_prefix(p, n, "+QISEND:")) {
    pos = sizeof("+QISEND:") - 1u;
    if (channel_num(p, n, &pos, &a) && pos < n && p[pos++] == ','
        && channel_num(p, n, &pos, &b) && pos < n && p[pos++] == ','
        && channel_num(p, n, &pos, &c) && pos == n && b <= a && c == a - b)
      ch.answer = c == 0 ? 1 : 2;
  } else if (ch.probe_generation == ch.generation && ch.probe_active == COMM_CH_PING && channel_prefix(p, n, "+QPING:")) {
    pos = sizeof("+QPING:") - 1u;
    if (!channel_num(p, n, &pos, &a) || pos >= n || p[pos++] != ',') return;
    if (a == 0 && channel_ip(p, n, &pos)) {
      /* Фактический ответ: IP, bytes, time, ttl. */
      for (unsigned i = 0; i < 3; ++i)
        if (pos >= n || p[pos++] != ',' || !channel_num(p, n, &pos, &b)) return;
      if (pos == n) ch.probe_answer = 1;
    } else if (a == 0 && channel_num(p, n, &pos, &b) && pos < n && p[pos++] == ','
               && channel_num(p, n, &pos, &c) && c > 0 && c <= b) {
      /* Итог: sent, rcvd, lost, min, max, avg. */
      for (unsigned i = 0; i < 4; ++i)
        if (pos >= n || p[pos++] != ',' || !channel_num(p, n, &pos, &a)) return;
      if (pos == n) ch.probe_answer = 1;
    }
  } else if (ch.probe_generation == ch.generation && ch.probe_active == COMM_CH_DNS && channel_prefix(p, n, "+QIURC: \"dnsgip\",")) {
    pos = sizeof("+QIURC: \"dnsgip\",") - 1u;
    if (channel_ip(p, n, &pos) && pos == n && ch.probe_answer == 2) ch.probe_answer = 1;
    else if (channel_num(p, n, &pos, &a) && pos < n && p[pos++] == ','
        && channel_num(p, n, &pos, &b) && pos < n && p[pos++] == ','
        && channel_num(p, n, &pos, &c) && pos == n) ch.probe_answer = a == 0 && b > 0 ? 2 : 3;
  }
}
void comm_channel_init(void (*event)(uint8_t, uint8_t))
{
  ch.event = event; ch.state = CH_INITIAL; ch.tx_state = TX_FREE;
  ch.operation = ch.done = ch.answer = ch.probe = ch.flags.open_seen = ch.flags.urgent = ch.flags.dns_fence = 0;
  ch.address = ch.address_failures = ch.connections = ch.retry_step = ch.generation = ch.probe_generation = 0;
  ch.now = ch.tx_since = ch.operation_since = ch.ack_next = ch.reopen_pause = ch.dns_since = ch.probe_since = 0;
  ch.probe_active = ch.probe_answer = ch.flags.registered_session = ch.flags.peer_closed = 0;
  ch.frame_count = 0; modem_slm336_transport(channel_at_event);
}
bool comm_channel_ready(void) { return ch.state == CH_OPEN; }
bool comm_tx_busy(void) { return ch.tx_state != TX_FREE; }
void comm_channel_priority(bool pending) { ch.flags.urgent = (uint8_t)pending; }
uint8_t *comm_tx_acquire(uint16_t *capacity)
{
  if (capacity) *capacity = sizeof(frame);
  if (!comm_channel_ready() || ch.tx_state != TX_FREE || ch.flags.urgent) return NULL;
  ch.tx_state = TX_BUILD; ch.tx_since = ch.now; return frame;
}
bool comm_tx_commit(uint16_t count)
{
  if (ch.tx_state != TX_BUILD || !count || count > sizeof(frame)) return false;
  ch.frame_count = count; ch.tx_state = TX_QUEUED; return true;
}
void comm_tx_release(void) { if (ch.tx_state == TX_BUILD) ch.tx_state = TX_FREE; }
void comm_channel_stable(void) { ch.retry_step = 0; }
void comm_channel_registered(void) { ch.connections = 0; ch.address_failures = 0; ch.flags.registered_session = 1; }
void comm_channel_reopen(uint32_t pause_ms)
{
  if (ch.state == CH_CLOSE || ch.state == CH_CLOSING || ch.state == CH_VERIFY || ch.state == CH_PAUSE || ch.state == CH_FAULT) return;
  if (ch.state == CH_INITIAL) ch.connections = 1;
  bool had_socket = ch.state == CH_OPEN;
  ++ch.generation;
  ch.state = CH_CLOSE; ch.reopen_pause = pause_ms; ch.probe = ch.probe_active = ch.probe_answer = 0;
  if (!ch.flags.registered_session && ++ch.address_failures >= 3) {
    ch.address_failures = 0;
    if (modem_slm336_reserve()) ch.address ^= 1u;
  }
  ch.flags.registered_session = 0;
  if (ch.connections >= 3) {
    uint8_t t = ch.retry_step < 3 ? (uint8_t)(COMM_T_SERVER_RETRY_1 + ch.retry_step) : COMM_T_SERVER_RETRY_MAX;
    if (ch.reopen_pause < comm_timeouts[t]) ch.reopen_pause = comm_timeouts[t];
    if (ch.retry_step < 3) ++ch.retry_step;
    ch.connections = 0;
  }
  if (had_socket) channel_emit(COMM_CH_CLOSED, 0);
}
bool comm_channel_probe(uint8_t kind)
{
  if (!comm_channel_ready() || ch.probe || ch.probe_active || (kind != COMM_CH_PING && kind != COMM_CH_DNS)) return false;
  ch.probe = kind; return true;
}
static bool channel_command(const char *s, uint8_t mode, uint8_t timeout_id, uint8_t op)
{
  comm_at_request_t req = {(const uint8_t *)s, (uint16_t)strlen(s), mode, comm_timeouts[timeout_id]};
  uint8_t r = modem_slm336_command(&req, ch.now);
  if (r == COMM_AT_BUSY) return false;
  if (r != COMM_AT_ACCEPTED) { ch.state = CH_FAULT; channel_emit(COMM_CH_FAULT, r); return false; }
  ch.operation = op; ch.done = ch.answer = 0; ch.operation_since = ch.now; return true;
}
static void channel_completed(void)
{
  uint8_t op = ch.operation; ch.done = 0;
  if (op == OP_OPEN || op == OP_PING || op == OP_DNS) {
    if (op != OP_OPEN) ch.operation = OP_NONE;
    if (ch.result != COMM_AT_OK) { ch.operation = OP_NONE;
      if (op == OP_OPEN) comm_channel_reopen(0);
      else if (ch.probe_active == (op == OP_PING ? COMM_CH_PING : COMM_CH_DNS)) {
        ch.probe = ch.probe_active = 0; channel_emit(COMM_CH_PROBE_DONE, 0);
      }
    }
    return;
  }
  ch.operation = OP_NONE;
  if (ch.state == CH_CLOSE && op != OP_CLOSE) return;
  if (op == OP_CLOSE || op == OP_VERIFY) {
    if (op == OP_CLOSE && ch.result == COMM_AT_ERROR && !ch.flags.peer_closed) {
      ch.state = CH_VERIFY; return;
    }
    /* После URC closed старого сокета уже нет. QICLOSE всё равно послан,
       финал дождались; ERROR не разрешает новый TCP без этого свидетельства. */
    if ((ch.result != COMM_AT_OK && !(ch.result == COMM_AT_ERROR && ch.flags.peer_closed))
        || (op == OP_VERIFY && ch.answer)) {
      ch.state = CH_FAULT; channel_emit(COMM_CH_FAULT, ch.result); return;
    }
    modem_slm336_socket(false); ch.tx_state = TX_FREE; ch.frame_count = 0;
    ch.state = CH_PAUSE; ch.operation_since = ch.now; return;
  }
  if (ch.result != (op == OP_PROMPT ? COMM_AT_GOT_PROMPT : COMM_AT_OK)) {
    comm_channel_reopen(0); return;
  }
  if (op == OP_PROMPT) ch.tx_state = TX_PAYLOAD;
  else if (op == OP_SEND) { ch.tx_state = TX_ACK; ch.ack_next = ch.now; }
  else if (op == OP_ACK) {
    if (!ch.answer) { comm_channel_reopen(0); return; }
    if (ch.answer == 1) { ch.tx_state = TX_FREE; channel_emit(COMM_CH_TX_DONE, 1); }
    else ch.ack_next = ch.now + comm_timeouts[COMM_T_ACK_POLL];
  }
}
void comm_channel_poll(uint32_t now_ms)
{
  ch.now = now_ms;
  modem_slm336_hold(ch.state != CH_OPEN || ch.tx_state != TX_FREE || ch.flags.urgent || ch.operation);
  modem_slm336_poll(now_ms);
  if (ch.state == CH_FAULT) return;
  if (ch.done) channel_completed();
  if (ch.state == CH_INITIAL) {
    if (modem_slm336_ready()) {
      ch.state = CH_OPEN; ch.connections = 1; channel_emit(COMM_CH_OPENED, 0);
    } else if (modem_slm336_status()->state == SLM_FAILED && modem_slm336_status()->step == SLM_TCP)
      { ch.connections = 1; comm_channel_reopen(0); }
    return;
  }
  if (ch.state == CH_OPEN && !modem_slm336_ready()) comm_channel_reopen(0);
  if (ch.state == CH_OPEN && ch.tx_state != TX_FREE && now_ms - ch.tx_since > comm_timeouts[COMM_T_UNACKED])
    comm_channel_reopen(0);
  if (ch.operation == OP_OPEN && !modem_slm336_busy()) {
    if (ch.flags.open_seen) {
      ch.operation = OP_NONE;
      if (ch.state == CH_OPENING) {
        if (!ch.flags.open_ok) comm_channel_reopen(0);
        else { modem_slm336_socket(true); ch.state = CH_OPEN; channel_emit(COMM_CH_OPENED, 0); }
      }
    } else if (now_ms - ch.operation_since >= comm_timeouts[COMM_T_QIOPEN]) {
      ch.operation = OP_NONE;
      if (ch.state == CH_OPENING) comm_channel_reopen(0);
    }
  }
  if (ch.probe_active) {
    uint8_t t = ch.probe_active == COMM_CH_PING ? COMM_T_PING : COMM_T_DNS;
    if (ch.probe_answer == 1 || now_ms - ch.probe_since >= comm_timeouts[t] || (ch.probe_active == COMM_CH_DNS && ch.probe_answer == 3)) {
      uint8_t ok = ch.probe_answer == 1; ch.probe_active = ch.probe = 0; channel_emit(COMM_CH_PROBE_DONE, ok);
    }
  }
  if (modem_slm336_busy()) return; /* В том числе безопасная ресинхронизация AT. */
  if (ch.state == CH_CLOSE) {
    if (ch.operation == OP_OPEN) return;
    ch.operation = OP_NONE;
    if (channel_command("AT+QICLOSE=0", COMM_AT_TEXT, COMM_T_TCP_CLOSE, OP_CLOSE)) ch.state = CH_CLOSING;
    return;
  }
  if (ch.state == CH_VERIFY) {
    /* EX §8.1.6: OK без QISTATE означает отсутствие сокетов PDP 1.
       После ERROR закрытия наличие старого сокета нельзя угадывать. */
    if (!ch.operation) channel_command("AT+QISTATE=0,1", COMM_AT_TEXT, COMM_T_AT_SIMPLE, OP_VERIFY);
    return;
  }
  if (ch.state == CH_PAUSE) {
    if (now_ms - ch.operation_since < ch.reopen_pause) return;
    uint8_t r = modem_slm336_tcp_open(ch.address, now_ms);
    if (r == COMM_AT_BUSY) return;
    if (r != COMM_AT_ACCEPTED) { ch.state = CH_FAULT; channel_emit(COMM_CH_FAULT, r); return; }
    ch.flags.peer_closed = 0;
    ch.state = CH_OPENING; ch.operation = OP_OPEN; ch.flags.open_seen = ch.done = ch.answer = 0;
    ch.operation_since = now_ms; ++ch.connections; return;
  }
  if (ch.state != CH_OPEN || ch.operation) return;
  if (ch.tx_state == TX_QUEUED) {
    char cmd[32]; int n = snprintf(cmd, sizeof(cmd), "AT+QISEND=0,%u", ch.frame_count);
    if (n > 0 && (size_t)n < sizeof(cmd) && channel_command(cmd, COMM_AT_PROMPT, COMM_T_SEND_PROMPT, OP_PROMPT)) ch.tx_state = TX_PROMPT;
  } else if (ch.tx_state == TX_PAYLOAD) {
    uint8_t r = modem_slm336_payload(frame, ch.frame_count, now_ms);
    if (r == COMM_AT_ACCEPTED) { ch.operation = OP_SEND; ch.done = ch.answer = 0; ch.tx_state = TX_SENDING; }
    else if (r != COMM_AT_BUSY) comm_channel_reopen(0);
  } else if (ch.tx_state == TX_ACK && (int32_t)(now_ms - ch.ack_next) >= 0)
    channel_command("AT+QISEND=0,0", COMM_AT_TEXT, COMM_T_AT_SIMPLE, OP_ACK);
  else if (ch.tx_state == TX_FREE && ch.probe == COMM_CH_PING && !ch.probe_active) {
    if (channel_command("AT+QPING=1,\"ya.ru\",3,2", COMM_AT_TEXT, COMM_T_AT_SIMPLE, OP_PING)) {
      ch.probe_active = COMM_CH_PING; ch.probe_answer = 0; ch.probe_since = now_ms; ch.probe_generation = ch.generation;
    }
  }
  else if (ch.tx_state == TX_FREE && ch.probe == COMM_CH_DNS && !ch.probe_active) {
    /* URC DNS не содержит ID: новое поколение не запускаем в старом окне. */
    if (ch.flags.dns_fence && now_ms - ch.dns_since < comm_timeouts[COMM_T_DNS_FENCE]) {
      ch.probe = 0; channel_emit(COMM_CH_PROBE_DONE, 0);
    } else if (channel_command("AT+QIDNSGIP=1,\"ya.ru\"", COMM_AT_TEXT, COMM_T_AT_SIMPLE, OP_DNS)) {
      ch.flags.dns_fence = 1; ch.dns_since = now_ms;
      ch.probe_active = COMM_CH_DNS; ch.probe_answer = 0; ch.probe_since = now_ms; ch.probe_generation = ch.generation;
    }
  }
}
