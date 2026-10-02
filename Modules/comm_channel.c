/* Доработка: Кряжев А., 2026. Слот принадлежит каналу до TCP ACK/отмены. */
#include "comm_channel.h"
#include "comm_modem.h"
#include "comm_uart.h"
#include "comm_timeouts.h"
#include <string.h>
static uint8_t frame[1040];
static struct {
  void (*event)(uint8_t, uint8_t);
  uint16_t (*binary)(const uint8_t *, uint16_t);
  void (*read_done)(uint16_t, uint8_t);
  uint32_t now, tx_since, operation_since, ack_next, reopen_pause, dns_since, probe_since;
  uint16_t frame_count, capacity, bytes, open_code;
  uint8_t state, tx_state, operation, answer;
  uint8_t address, address_failures, connections, retry_step, probe;
  uint8_t probe_active, probe_answer, generation, probe_generation;
  struct { uint8_t open_seen:1, open_ack:1, recv:1, read_active:1, urgent:1, dns_fence:1, registered_session:1, peer_closed:1; } flags;
} ch;
enum { CH_INITIAL, CH_OPEN, CH_CLOSE, CH_CLOSING, CH_VERIFY, CH_PAUSE, CH_OPENING, CH_FAULT };
enum { TX_FREE, TX_BUILD, TX_QUEUED, TX_PROMPT, TX_PAYLOAD, TX_SENDING, TX_ACK };
enum { OP_NONE, OP_PROMPT, OP_SEND, OP_ACK, OP_CLOSE, OP_VERIFY, OP_OPEN, OP_PING, OP_DNS, OP_READ };
static void channel_emit(uint8_t kind, uint8_t value)
{ if (ch.event) ch.event(kind, value); }
/* IPv4: четыре проверенных октета, с кавычками или без. */
#ifdef __IAR_SYSTEMS_ICC__
#pragma inline=forced
#endif
static bool channel_ip(const uint8_t *p, uint16_t n, const char *quoted, const char *plain, uint32_t *values)
{
  if (!comm_at_parse(p,n,quoted,values) && !comm_at_parse(p,n,plain,values)) return false;
  return true;
}
static void channel_read_done(uint8_t reason)
{
  if (ch.flags.read_active) {
    ch.flags.read_active = 0;
    if (ch.read_done) ch.read_done(ch.bytes, reason);
  }
}
static uint16_t channel_binary(const uint8_t *p, uint16_t n)
{
  if (!ch.flags.read_active || !ch.binary) return 0;
  uint16_t used = ch.binary(p, n);
  if (used <= n) ch.bytes = (uint16_t)(ch.bytes + used);
  return used;
}
static void channel_at_event(uint8_t kind, const uint8_t *p, uint16_t n, uint16_t detail)
{
  if (kind == COMM_AT_NO_ANSWER || kind == COMM_AT_STREAM_ERROR) {
    uint8_t reason = kind == COMM_AT_NO_ANSWER ? SLM_ERR_NO_ANSWER : SLM_ERR_STREAM;
    channel_read_done(reason);
    if (comm_modem_ready()) comm_modem_record(SLM_STEP_COUNT, reason, comm_modem_status()->modem_code);
    ch.state = CH_FAULT; channel_emit(COMM_CH_FAULT, (uint8_t)detail); return;
  }
  if (kind == COMM_AT_DONE) return;
  if (!p) return;
  uint32_t v[8];
  if (comm_at_parse(p,n,comm_modem_profile()->recv,v) && v[0] == 0) ch.flags.recv = 1;
  if (comm_at_parse(p,n,comm_modem_profile()->closed,v) && v[0] == 0) {
    channel_read_done(SLM_ERR_TCP);
    if (comm_modem_ready()) comm_modem_record(SLM_STEP_COUNT, SLM_ERR_TCP, comm_modem_status()->modem_code);
    ch.flags.peer_closed = 1; comm_channel_reopen(0); return;
  }
  if (comm_at_parse(p,n,comm_modem_profile()->pdp,v) && v[0] == 1) {
    channel_read_done(SLM_ERR_PDP);
    comm_channel_reopen(0); return;
  }
  if (ch.operation == OP_VERIFY && comm_at_prefix(p,n,"+QISTATE:")) ch.answer = 1;
  if (ch.operation == OP_OPEN) {
    if (comm_at_parse(p,n,comm_modem_profile()->open,v) && v[0] == 0) {
      ch.flags.open_seen = 1; ch.open_code = (uint16_t)v[1];
    }
  } else if (ch.operation == OP_ACK) {
    if (comm_at_parse(p,n,"+QISEND:\001,\001,\001",v) && v[1] <= v[0] && v[2] == v[0] - v[1])
      ch.answer = v[2] == 0 ? 1 : 2;
  } else if (ch.probe_generation == ch.generation) {
    if (ch.probe_active == COMM_CH_PING) {
      if (!comm_at_prefix(p,n,"+QPING:")) return;
      /* Фактический ответ: код, IPv4, bytes, time, ttl. */
      uint16_t pos = sizeof("+QPING:") - 1u;
      uint32_t code;
      if (!comm_at_scan(p,n,&pos,UINT32_MAX,&code) || code || pos >= n || p[pos++] != ',') return;
      p += pos; n = (uint16_t)(n - pos);
      if (channel_ip(p,n,"\"\006.\006.\006.\006\",\001,\001,\001",
                        "\006.\006.\006.\006,\001,\001,\001",v)) ch.probe_answer = 1;
      else if (comm_at_parse(p,n,"\001,\001,\001,\001,\001,\001",v) && v[1] > 0 && v[1] <= v[0]) ch.probe_answer = 1;
    } else if (ch.probe_active == COMM_CH_DNS && comm_at_prefix(p,n,"+QIURC: \"dnsgip\",")) {
      uint16_t pos = sizeof("+QIURC: \"dnsgip\",") - 1u;
      p += pos; n = (uint16_t)(n - pos);
      if (channel_ip(p,n,"\"\006.\006.\006.\006\"","\006.\006.\006.\006",v)
          && ch.probe_answer == 2) ch.probe_answer = 1;
      else if (comm_at_parse(p,n,"\001,\001,\001",v)) ch.probe_answer = v[0] == 0 && v[1] > 0 ? 2 : 3;
    }
  }
}
void comm_channel_init(void (*event)(uint8_t, uint8_t))
{
  memset(&ch, 0, sizeof(ch));
  ch.event = event; comm_modem_transport(channel_at_event);
  comm_at_sink(channel_binary);
}
void comm_channel_receive(uint16_t (*binary)(const uint8_t *, uint16_t),
                          void (*read_done)(uint16_t, uint8_t))
{ ch.binary = binary; ch.read_done = read_done; }
void comm_channel_rx_capacity(uint16_t count)
{ ch.capacity = count > 256u ? 256u : count; }
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
  char cmd[128];
  if (op == OP_OPEN || op == OP_READ) {
    const comm_modem_profile_t *profile = comm_modem_profile();
    const char *p = profile->commands + (op == OP_OPEN ? profile->tcp : profile->read);
    const char *host = NULL;
    uint16_t value = ch.capacity;
    if (op == OP_OPEN) {
      const slm_settings_t *settings = comm_modem_settings();
      uint16_t capacity = 0;
      host = settings->text(ch.address ? SLM_CFG_HOST2 : SLM_CFG_HOST, &capacity);
      value = settings->number(ch.address ? SLM_CFG_PORT2 : SLM_CFG_PORT);
      if (!host || !capacity || !value || !comm_at_field(host, capacity, true)) return false;
    } else mode = profile->read_mode;
    uint16_t n = comm_at_copy(cmd, sizeof(cmd), 0, "AT+");
    while (*p && n < sizeof(cmd)) {
      uint8_t token = (uint8_t)*p++;
      if (token == 5) n = comm_at_copy(cmd, sizeof(cmd), n, host);
      else if (token == 6 || token == 7) n = comm_at_decimal(cmd, sizeof(cmd), n, value);
      else cmd[n++] = (char)token;
    }
    if (n >= sizeof(cmd)) return false;
    cmd[n] = 0; s = cmd;
  }
  comm_at_request_t req = {(const uint8_t *)s, s ? (uint16_t)strlen(s) : 0, mode, comm_timeouts[timeout_id]};
  uint8_t r = op == OP_SEND ? comm_modem_payload(frame, ch.frame_count, ch.now)
    : comm_modem_request(&req, ch.now);
  if (r != COMM_AT_ACCEPTED) return false;
  ch.operation = op; ch.answer = 0; ch.operation_since = ch.now;
  if (op == OP_OPEN || op == OP_READ) {
    comm_modem_record(op == OP_OPEN ? SLM_TCP : SLM_READ, 0, 0);
    if (op == OP_OPEN) ch.flags.open_ack = ch.flags.open_seen = 0;
    else { ch.flags.recv = 0; ch.flags.read_active = 1; ch.bytes = 0; }
  }
  return true;
}
static void channel_completed(void)
{
  uint8_t op = ch.operation, result = comm_modem_result();
  if (op == OP_OPEN) {
    if (result == COMM_AT_OK) ch.flags.open_ack = 1;
    if (result == UINT8_MAX || result == COMM_AT_OK) {
      if (!ch.flags.open_ack) return;
      if (ch.now - ch.operation_since >= comm_timeouts[COMM_T_QIOPEN]) result = COMM_AT_TIMEOUT;
      else if (!ch.flags.open_seen) return;
      else result = ch.open_code ? COMM_AT_ERROR : COMM_AT_OK;
    }
    ch.operation = OP_NONE;
    uint8_t reason = result == COMM_AT_OK ? SLM_ERR_NONE
      : result == COMM_AT_TIMEOUT ? SLM_ERR_TIMEOUT : SLM_ERR_TCP;
    comm_modem_record(SLM_STEP_COUNT, reason, ch.flags.open_seen ? ch.open_code : comm_modem_status()->modem_code);
    if (ch.state == CH_OPENING) {
      if (reason) comm_channel_reopen(0);
      else { ch.state = CH_OPEN; channel_emit(COMM_CH_OPENED, 0); }
    }
    return;
  }
  if (result == UINT8_MAX) return;
  ch.operation = OP_NONE;
  if (op == OP_READ) {
    uint8_t reason = result == COMM_AT_OK ? SLM_ERR_NONE : result == COMM_AT_TIMEOUT ? SLM_ERR_TIMEOUT
      : result > COMM_AT_TIMEOUT ? SLM_ERR_STREAM : SLM_ERR_AT;
    ch.flags.recv = ch.flags.recv || ch.bytes;
    if (ch.flags.read_active) {
      comm_modem_record(SLM_STEP_COUNT, reason, comm_modem_status()->modem_code);
      channel_read_done(reason);
    }
    if (reason) comm_channel_reopen(0);
    return;
  }
  if (op == OP_PING || op == OP_DNS) {
    if (result != COMM_AT_OK && ch.probe_active == (op == OP_PING ? COMM_CH_PING : COMM_CH_DNS)) {
      ch.probe = ch.probe_active = 0; channel_emit(COMM_CH_PROBE_DONE, 0);
    }
    return;
  }
  if (ch.state == CH_CLOSE && op != OP_CLOSE) return;
  if (op == OP_CLOSE || op == OP_VERIFY) {
    if (op == OP_CLOSE && result == COMM_AT_ERROR && !ch.flags.peer_closed) {
      ch.state = CH_VERIFY; return;
    }
    /* После URC closed дождались финала QICLOSE; иначе требуется QISTATE. */
    if ((result != COMM_AT_OK && !(result == COMM_AT_ERROR && ch.flags.peer_closed))
        || (op == OP_VERIFY && ch.answer)) {
      comm_modem_record(SLM_STEP_COUNT, SLM_ERR_TCP, comm_modem_status()->modem_code);
      ch.state = CH_FAULT; channel_emit(COMM_CH_FAULT, result); return;
    }
    ch.flags.recv = 0; ch.tx_state = TX_FREE; ch.frame_count = 0;
    ch.state = CH_PAUSE; ch.operation_since = ch.now; return;
  }
  if (result != (op == OP_PROMPT ? COMM_AT_GOT_PROMPT : COMM_AT_OK)) {
    comm_modem_record(SLM_STEP_COUNT, result == COMM_AT_TIMEOUT ? SLM_ERR_TIMEOUT : SLM_ERR_AT,
                      comm_modem_status()->modem_code);
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
  modem_slm336_poll(now_ms);
  if (ch.state == CH_FAULT) return;
  if (ch.operation == OP_OPEN || ch.operation == OP_READ)
    comm_modem_record(SLM_STEP_COUNT + 1u, 0, comm_modem_status()->modem_code);
  if (ch.operation) channel_completed();
  if (ch.state == CH_INITIAL) {
    if (!comm_modem_ready()) return;
    ch.state = CH_PAUSE;
  }
  if (ch.state == CH_OPEN && !comm_modem_ready()) comm_channel_reopen(0);
  if (ch.state == CH_OPEN && ch.tx_state != TX_FREE && now_ms - ch.tx_since > comm_timeouts[COMM_T_UNACKED])
    comm_channel_reopen(0);
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
    if (!channel_command(NULL, COMM_AT_TEXT, COMM_T_AT_SIMPLE, OP_OPEN)) return;
    ch.flags.peer_closed = 0;
    ch.state = CH_OPENING; ++ch.connections; return;
  }
  if (ch.state != CH_OPEN || ch.operation) return;
  if (ch.tx_state == TX_QUEUED) {
    char cmd[32]; uint16_t n = comm_at_copy(cmd, sizeof(cmd), 0, "AT+QISEND=0,");
    n = comm_at_decimal(cmd, sizeof(cmd), n, ch.frame_count);
    if (n < sizeof(cmd) && channel_command(cmd, COMM_AT_PROMPT, COMM_T_SEND_PROMPT, OP_PROMPT)) ch.tx_state = TX_PROMPT;
  } else if (ch.tx_state == TX_PAYLOAD) {
    if (channel_command(NULL, COMM_AT_SEND, COMM_T_SEND_RESULT, OP_SEND)) ch.tx_state = TX_SENDING;
  } else if (ch.tx_state == TX_ACK && (int32_t)(now_ms - ch.ack_next) >= 0)
    channel_command("AT+QISEND=0,0", COMM_AT_TEXT, COMM_T_AT_SIMPLE, OP_ACK);
  else if (ch.tx_state == TX_FREE && ch.probe && !ch.probe_active) {
    bool dns = ch.probe == COMM_CH_DNS;
    /* URC DNS не содержит ID: новое поколение не запускаем в старом окне. */
    if (dns && ch.flags.dns_fence && now_ms - ch.dns_since < comm_timeouts[COMM_T_DNS_FENCE]) {
      ch.probe = 0; channel_emit(COMM_CH_PROBE_DONE, 0);
    } else if (channel_command(dns ? "AT+QIDNSGIP=1,\"ya.ru\"" : "AT+QPING=1,\"ya.ru\",3,2",
                              COMM_AT_TEXT, COMM_T_AT_SIMPLE, dns ? OP_DNS : OP_PING)) {
      if (dns) { ch.flags.dns_fence = 1; ch.dns_since = now_ms; }
      ch.probe_active = ch.probe; ch.probe_answer = 0;
      ch.probe_since = now_ms; ch.probe_generation = ch.generation;
    }
  }
  else if (ch.tx_state == TX_FREE && !ch.flags.urgent && ch.flags.recv && ch.capacity && ch.binary)
    channel_command(NULL, COMM_AT_TEXT, COMM_T_QIRD, OP_READ);
}
