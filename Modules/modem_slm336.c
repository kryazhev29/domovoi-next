/* Доработка: Кряжев А., 2026. Автомат SLM336, один запрос AT в работе. */
#include "modem_slm336.h"
#include "comm_at.h"
#include "comm_uart.h"
#include "comm_timeouts.h"
#include <string.h>
#include <stdio.h>
static slm_settings_t slm_settings;
static slm_handlers_t slm_handlers;
static slm_diag_t slm_diag;
static slm_status_t slm_status;
typedef char slm_diag_budget[(sizeof(slm_diag_t) <= 128u) ? 1 : -1];
static uint32_t slm_step_started, slm_phase_started, slm_attempt_started;
static uint32_t slm_now, slm_next, slm_tcp_started;
static uint16_t slm_capacity, slm_bytes, slm_open_code;
static uint8_t slm_wait, slm_done, slm_result, slm_answer, slm_query;
static uint8_t slm_recv, slm_read_urc, slm_open_seen, slm_tcp_ack, slm_in_poll;
static uint8_t slm_net_registered, slm_mnc;

static bool slm_is(const uint8_t *p, uint16_t n, const char *s)
{
  return n == strlen(s) && !memcmp(p, s, n);
}
static bool slm_prefix(const uint8_t *p, uint16_t n, const char *s)
{
  size_t len = strlen(s);
  return n >= len && !memcmp(p, s, len);
}
/* Число с проверкой границ; окончания проверяет вызывающая сторона. */
static bool slm_num(const uint8_t *p, uint16_t n, uint16_t *pos, uint16_t *out)
{
  uint32_t v = 0;
  while (*pos < n && p[*pos] == ' ') ++*pos;
  uint16_t begin = *pos;
  while (*pos < n && p[*pos] >= '0' && p[*pos] <= '9') {
    v = v * 10u + p[(*pos)++] - '0';
    if (v > UINT16_MAX) return false;
  }
  *out = (uint16_t)v;
  return *pos != begin;
}
static uint16_t slm_elapsed(uint32_t start)
{
  uint32_t ticks = (slm_now - start) / 10u;
  return ticks > UINT16_MAX ? UINT16_MAX : (uint16_t)ticks;
}
static void slm_finish_step(void)
{
  if (slm_status.step < sizeof(slm_diag.steps) / sizeof(slm_diag.steps[0]))
    slm_diag.steps[slm_status.step] = slm_elapsed(slm_step_started);
  if (slm_status.step < 32u) slm_status.completed_steps |= (uint32_t)1u << slm_status.step;
}
static void slm_fail(uint8_t reason)
{
  if (slm_status.state == SLM_FAILED) return;
  if (slm_status.state == SLM_OPEN && slm_status.step == SLM_READ && slm_handlers.read_done)
    slm_handlers.read_done(slm_bytes, reason);
  slm_finish_step();
  if (slm_status.state == SLM_STARTING) {
    if (slm_status.phase < sizeof(slm_diag.phases) / sizeof(slm_diag.phases[0]))
      slm_diag.phases[slm_status.phase] = slm_elapsed(slm_phase_started);
    slm_status.completed_phases |= (uint8_t)(1u << slm_status.phase);
  }
  uint8_t slot = slm_diag.failure_next;
  if (slot >= sizeof(slm_diag.failure_step) / sizeof(slm_diag.failure_step[0])) slot = 0;
  slm_diag.failure_seconds[slot] = (slm_now - slm_attempt_started) / 1000u;
  slm_diag.failure_step[slot] = slm_status.step;
  slm_diag.failure_reason[slot] = reason;
  slm_diag.last_code = slm_status.modem_code;
  slm_diag.failure_next = (uint8_t)((slot + 1u) % (sizeof(slm_diag.failure_step) / sizeof(slm_diag.failure_step[0])));
  if (slm_diag.failure_count < sizeof(slm_diag.failure_step) / sizeof(slm_diag.failure_step[0])) ++slm_diag.failure_count;
  slm_status.reason = reason;
  slm_status.state = SLM_FAILED;
}
static void slm_step(uint8_t step)
{
  if (step >= sizeof(slm_diag.steps) / sizeof(slm_diag.steps[0])) { slm_fail(SLM_ERR_RESPONSE); return; }
  slm_status.step = step;
  slm_step_started = slm_now;
  slm_diag.steps[step] = 0;
  slm_answer = slm_wait = slm_done = 0;
  slm_next = slm_now;
}
static void slm_phase(uint8_t phase, uint8_t step)
{
  if (phase >= sizeof(slm_diag.phases) / sizeof(slm_diag.phases[0])) { slm_fail(SLM_ERR_RESPONSE); return; }
  if (slm_status.phase < sizeof(slm_diag.phases) / sizeof(slm_diag.phases[0]))
    slm_diag.phases[slm_status.phase] = slm_elapsed(slm_phase_started);
  slm_status.completed_phases |= (uint8_t)(1u << slm_status.phase);
  slm_status.phase = phase;
  slm_phase_started = slm_now;
  slm_diag.phases[phase] = 0;
  slm_step(step);
}
static uint16_t slm_binary(const uint8_t *p, uint16_t n)
{
  if (slm_status.state != SLM_OPEN || slm_status.step != SLM_READ || !slm_handlers.binary) return 0;
  uint16_t used = slm_handlers.binary(p, n);
  if (used <= n) slm_bytes = (uint16_t)(slm_bytes + used);
  return used;
}
static void slm_event(uint8_t kind, const uint8_t *p, uint16_t n, uint16_t detail)
{
  uint16_t pos, a, b;
  if (kind == COMM_AT_NO_ANSWER || kind == COMM_AT_STREAM_ERROR) {
    slm_status.at_fault = kind == COMM_AT_NO_ANSWER ? SLM_ERR_NO_ANSWER : SLM_ERR_STREAM;
    slm_fail(slm_status.at_fault); return;
  }
  if (kind == COMM_AT_DONE) {
    slm_wait = 0; slm_done = 1; slm_result = (uint8_t)detail;
    slm_status.modem_code = n;
    return;
  }
  /* SIM2 не является признаком отсутствия SIM1; SIM1 URC тоже лишь подсказка. */
  if (slm_prefix(p, n, "+SIM1") || slm_prefix(p, n, "+SIM2")) return;
  if (slm_prefix(p, n, "+QIURC: \"recv\",")) {
    pos = sizeof("+QIURC: \"recv\",") - 1u;
    if (slm_num(p, n, &pos, &a) && pos == n && a == 0) {
      slm_recv = 1; slm_read_urc = 1;
    }
    return;
  }
  if (slm_prefix(p, n, "+QIURC: \"closed\",")) {
    pos = sizeof("+QIURC: \"closed\",") - 1u;
    if (slm_num(p, n, &pos, &a) && pos == n && a == 0
        && (slm_status.state == SLM_OPEN || slm_status.step == SLM_TCP)) slm_fail(SLM_ERR_TCP);
    return;
  }
  if (slm_prefix(p, n, "+QIURC: \"pdpdeact\",")) {
    pos = sizeof("+QIURC: \"pdpdeact\",") - 1u;
    if (slm_num(p, n, &pos, &a) && pos == n && a == 1
        && slm_status.step >= SLM_ACTIVATE) slm_fail(SLM_ERR_PDP);
    return;
  }
  if (slm_status.state != SLM_STARTING) return;
  if (slm_status.step == SLM_TCP && slm_prefix(p, n, "+QIOPEN:")) {
    pos = sizeof("+QIOPEN:") - 1u;
    if (slm_num(p, n, &pos, &a) && pos < n && p[pos++] == ','
        && slm_num(p, n, &pos, &b) && pos == n && a == 0) {
      slm_open_seen = 1; slm_open_code = b;
    }
  } else if (slm_status.step == SLM_MODEL) {
    if (slm_prefix(p, n, "SLM336") || slm_prefix(p, n, "MEIG_SLM336")) slm_answer = 1;
  } else if (slm_status.step == SLM_SIM) {
    if (slm_is(p, n, "+CPIN: READY")) slm_answer = 1;
    else if (slm_is(p, n, "+CPIN: SIM PIN")) slm_answer = 2;
    else if (slm_is(p, n, "+CPIN: SIM PUK")) slm_answer = 3;
    else if (slm_is(p, n, "+CME ERROR: SIM not inserted")
             || slm_is(p, n, "+CME ERROR: SIM failure")) slm_answer = 4;
  } else if (slm_status.step == SLM_NETWORK) {
    const char *mark = slm_query == 0 ? "+CREG:" : "+CEREG:";
    if (slm_prefix(p, n, mark)) {
      pos = (uint16_t)strlen(mark);
      if (slm_num(p, n, &pos, &a) && pos < n && p[pos++] == ','
          && slm_num(p, n, &pos, &b) && (pos == n || p[pos] == ',') && a <= 2 && b <= 5) {
        slm_answer = 1;
        if (b == 1 || b == 5) slm_net_registered = 1;
        if (b == 3) slm_answer = 2;
      }
    }
  } else if (slm_status.step == SLM_IMSI) {
    if (n == 15) {
      for (pos = 0; pos < n; ++pos) if (p[pos] < '0' || p[pos] > '9') return;
      slm_answer = 1;
      slm_mnc = !memcmp(p, "250", 3) ? (uint8_t)((p[3] - '0') * 10u + p[4] - '0') : 0;
    }
  } else if (slm_status.step == SLM_URC_PORT) {
    if (slm_prefix(p, n, "+QURCCFG: \"urcport\","))
      slm_answer = slm_is(p, n, "+QURCCFG: \"urcport\",\"uart1\"") ? 1 : 2;
  } else if (slm_status.step == SLM_RAT_QUERY) {
    if (slm_prefix(p, n, "+QCFG: \"nwscanmode\",")) {
      pos = sizeof("+QCFG: \"nwscanmode\",") - 1u;
      if (slm_num(p, n, &pos, &a) && pos == n && a <= 4)
        slm_answer = a == slm_settings.number(SLM_CFG_NETWORK) ? 1 : 2;
    }
  }
}
static bool slm_field(const char *p, size_t size, bool host)
{
  size_t i;
  for (i = 0; i < size; ++i) {
    unsigned char c = (unsigned char)p[i];
    if (!c) return !host || i != 0;
    if (host ? !((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
           || (c >= '0' && c <= '9') || c == '.' || c == '-')
        : c < 32 || c > 126 || c == '"' || c == '\\') return false;
  }
  return false;
}
static const char *slm_text(uint8_t field)
{
  uint16_t capacity;
  return slm_settings.text(field, &capacity);
}
static bool slm_valid_settings(void)
{
  uint16_t type = slm_settings.number(SLM_CFG_TYPE);
  if (!slm_settings.number(SLM_CFG_PORT) || (type != 2 && type != 0 && type != 255)
      || slm_settings.number(SLM_CFG_NETWORK) > 4) return false;
  for (uint8_t field = SLM_CFG_HOST; field <= SLM_CFG_PASSWORD; ++field) {
    uint16_t capacity = 0;
    const char *p = slm_settings.text(field, &capacity);
    if (!p || !capacity || !slm_field(p, capacity, field == SLM_CFG_HOST)) return false;
  }
  return true;
}
bool modem_slm336_start(const slm_settings_t *settings, const slm_handlers_t *handlers, uint32_t now_ms)
{
  static const comm_at_handlers_t at_handlers = {slm_event, slm_binary};
  if (slm_in_poll || !settings || !settings->text || !settings->number
      || !handlers || !handlers->gpio
      || slm_status.state == SLM_STARTING || slm_status.state == SLM_OPEN) return false;
  slm_settings = *settings;
  if (!slm_valid_settings()) return false;
  slm_handlers = *handlers;
  slm_now = slm_attempt_started = slm_phase_started = now_ms;
  /* Между попытками сохраняется только кольцо последних десяти сбоев. */
  memset(slm_diag.steps, 0, sizeof(slm_diag.steps));
  memset(slm_diag.phases, 0, sizeof(slm_diag.phases));
  memset(&slm_status, 0, sizeof(slm_status));
  slm_status.state = SLM_STARTING;
  slm_status.phase = SLM_PHASE_POWER;
  slm_capacity = slm_bytes = 0;
  slm_recv = slm_read_urc = slm_query = slm_open_seen = slm_tcp_ack = slm_net_registered = slm_mnc = 0;
  comm_uart_abort();
  comm_uart_init(115200);
  comm_at_init(&at_handlers, comm_timeouts[COMM_T_AT_RESYNC]);
  comm_power_v5_start(handlers->gpio, now_ms);
  slm_step(SLM_DISCHARGE);
  return true;
}
void modem_slm336_off(void)
{
  if (slm_in_poll) return;
  comm_uart_abort(); comm_power_v5_off(); slm_status.state = SLM_IDLE;
}
bool modem_slm336_ready(void) { return slm_status.state == SLM_OPEN; }
const slm_diag_t *modem_slm336_diag(void) { return &slm_diag; }
const slm_status_t *modem_slm336_status(void) { return &slm_status; }
void modem_slm336_rx_capacity(uint16_t count) { slm_capacity = count > 256u ? 256u : count; }
static const char *slm_apn(void)
{
  static const struct { uint8_t mnc; const char *apn; } apns[] = {
    {1, "internet.mts.ru"}, {2, "internet"}, {99, "internet.beeline.ru"},
    {20, "internet.tele2.ru"}, {11, "internet.yota"}
  };
  if (slm_text(SLM_CFG_APN)[0]) return slm_text(SLM_CFG_APN);
  for (unsigned i = 0; i < sizeof(apns) / sizeof(apns[0]); ++i)
    if (slm_mnc == apns[i].mnc) return apns[i].apn;
  return "";
}
static void slm_send(void)
{
  /* Временная строка только для форматирования: AT немедленно копирует её
     в свой единственный статический TX-буфер. Указатель на стек не остаётся. */
  if (!slm_valid_settings()) { slm_fail(SLM_ERR_RESPONSE); return; }
  char cmd[128];
  const char *s = NULL;
  int n = 0;
  uint32_t limit = comm_timeouts[COMM_T_AT_SIMPLE];
  switch (slm_status.step) {
  case SLM_AT_READY: s = "AT"; break;
  case SLM_MODEL: s = "AT+GMM"; break;
  case SLM_ECHO: s = "ATE0"; break;
  case SLM_CMEE: s = "AT+CMEE=1"; break;
  case SLM_IFC: s = "AT+IFC=2,2"; break;
  case SLM_CLIP: s = "AT+CLIP=1"; break;
  case SLM_CMER: s = "AT+CMER=3,0,0,0"; break;
  case SLM_CREG_URC: s = "AT+CREG=0"; break;
  case SLM_CGREG_URC: s = "AT+CGREG=0"; break;
  case SLM_CEREG_URC: s = "AT+CEREG=0"; break;
  case SLM_URC_PORT: s = "AT+QURCCFG=\"urcport\""; break;
  case SLM_URC_SET: s = "AT+QURCCFG=\"urcport\",\"uart1\""; break;
  case SLM_SIM: s = "AT+CPIN?"; limit = comm_timeouts[COMM_T_SIM]; break;
  case SLM_RAT_QUERY: s = "AT+QCFG=\"nwscanmode\""; break;
  case SLM_RAT_SET: n = snprintf(cmd, sizeof(cmd), "AT+QCFG=\"nwscanmode\",%u,1", slm_settings.number(SLM_CFG_NETWORK)); break;
  case SLM_NETWORK: s = slm_query ? "AT+CEREG?" : "AT+CREG?"; break;
  case SLM_IMSI: s = "AT+CIMI"; break;
  case SLM_APN:
    n = snprintf(cmd, sizeof(cmd), "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",%u",
                 slm_apn(), slm_text(SLM_CFG_LOGIN), slm_text(SLM_CFG_PASSWORD),
                 slm_text(SLM_CFG_LOGIN)[0] || slm_text(SLM_CFG_PASSWORD)[0] ? 1u : 0u); break;
  case SLM_ACTIVATE: s = "AT+QIACT=1"; limit = comm_timeouts[COMM_T_QIACT]; break;
  case SLM_TCP:
    n = snprintf(cmd, sizeof(cmd), "AT+QIOPEN=1,0,\"TCP\",\"%s\",%u,0,0", slm_text(SLM_CFG_HOST), slm_settings.number(SLM_CFG_PORT));
    slm_tcp_started = slm_now; break;
  case SLM_READ:
    n = snprintf(cmd, sizeof(cmd), "AT+QIRD=0,%u", slm_capacity);
    slm_recv = slm_read_urc = 0; slm_bytes = 0;
    limit = comm_timeouts[COMM_T_QIRD]; break;
  default: return;
  }
  if (s) n = snprintf(cmd, sizeof(cmd), "%s", s);
  if (n <= 0 || (size_t)n >= sizeof(cmd)) { slm_fail(SLM_ERR_RESPONSE); return; }
  comm_at_request_t req = {(const uint8_t *)cmd, (uint16_t)n,
                          slm_status.step == SLM_READ ? COMM_AT_SLM_QIRD : COMM_AT_TEXT, limit};
  if (comm_at_start(&req, slm_now) != COMM_AT_ACCEPTED) { slm_fail(SLM_ERR_RESPONSE); return; }
  slm_wait = 1;
}
static void slm_completed(void)
{
  uint8_t step = slm_status.step;
  slm_done = 0;
  if (slm_result != COMM_AT_OK) {
    uint8_t reason = slm_result == COMM_AT_TIMEOUT ? SLM_ERR_TIMEOUT
                     : slm_result > COMM_AT_TIMEOUT ? SLM_ERR_STREAM : SLM_ERR_AT;
    if (step == SLM_SIM && (slm_status.modem_code == 10 || slm_answer == 4)) reason = SLM_ERR_NO_SIM;
    if (step == SLM_ACTIVATE && slm_result != COMM_AT_TIMEOUT) reason = SLM_ERR_PDP;
    if (step == SLM_TCP && slm_result != COMM_AT_TIMEOUT) reason = SLM_ERR_TCP;
    if (step == SLM_AT_READY && slm_result <= COMM_AT_TIMEOUT) {
      slm_next = slm_now + comm_timeouts[COMM_T_AT_RETRY]; return;
    }
    slm_fail(reason); return;
  }
  if (step == SLM_TCP) { slm_tcp_ack = 1; return; }
  if (step == SLM_NETWORK) {
    if (!slm_answer) { slm_fail(SLM_ERR_RESPONSE); return; }
    if (slm_answer == 2) { slm_fail(SLM_ERR_NETWORK); return; }
    if (slm_query == 0) { slm_query = 1; return; }
    if (!slm_net_registered) {
      slm_query = 0; slm_next = slm_now + comm_timeouts[COMM_T_NETWORK_POLL]; return;
    }
  }
  if ((step == SLM_MODEL || step == SLM_IMSI || step == SLM_RAT_QUERY || step == SLM_URC_PORT)
      && !slm_answer) { slm_fail(step == SLM_MODEL ? SLM_ERR_TYPE : SLM_ERR_RESPONSE); return; }
  if (step == SLM_SIM && slm_answer != 1) {
    slm_fail(slm_answer == 2 ? SLM_ERR_PIN : slm_answer == 3 ? SLM_ERR_PUK : SLM_ERR_RESPONSE); return;
  }
  slm_finish_step();
  if (step == SLM_READ) {
    slm_recv = slm_recv || slm_bytes || slm_read_urc;
    if (slm_handlers.read_done) slm_handlers.read_done(slm_bytes, SLM_ERR_NONE);
    slm_status.step = SLM_TCP; return;
  }
  switch (step) {
  case SLM_AT_READY: slm_step(slm_settings.number(SLM_CFG_TYPE) == 2 ? SLM_ECHO : SLM_MODEL); break;
  case SLM_MODEL:
    if (slm_handlers.type_found) slm_handlers.type_found(2);
    slm_step(SLM_ECHO); break;
  case SLM_URC_PORT:
    if (slm_answer == 2) slm_step(SLM_URC_SET);
    else slm_phase(SLM_PHASE_SIM, SLM_SIM);
    break;
  case SLM_URC_SET: slm_phase(SLM_PHASE_SIM, SLM_SIM); break;
  case SLM_SIM: slm_phase(SLM_PHASE_NETWORK, SLM_RAT_QUERY); break;
  case SLM_RAT_QUERY: slm_step(slm_answer == 2 ? SLM_RAT_SET : SLM_NETWORK); break;
  case SLM_NETWORK: slm_phase(SLM_PHASE_PDP, slm_text(SLM_CFG_APN)[0] ? SLM_APN : SLM_IMSI); break;
  case SLM_ACTIVATE: slm_phase(SLM_PHASE_TCP, SLM_TCP); break;
  default: slm_step((uint8_t)(step + 1u)); break;
  }
}
void modem_slm336_poll(uint32_t now_ms)
{
  if (slm_in_poll || slm_status.state == SLM_IDLE) return;
  slm_in_poll = 1; slm_now = now_ms;
  comm_at_poll(now_ms); /* Даже после ошибки шага завершает ресинхронизацию. */
  if (slm_status.state == SLM_FAILED) goto end;
  if ((slm_status.state == SLM_STARTING || slm_status.step == SLM_READ)
      && slm_status.step < sizeof(slm_diag.steps) / sizeof(slm_diag.steps[0]))
    slm_diag.steps[slm_status.step] = slm_elapsed(slm_step_started);
  if (slm_status.state == SLM_STARTING
      && slm_status.phase < sizeof(slm_diag.phases) / sizeof(slm_diag.phases[0]))
    slm_diag.phases[slm_status.phase] = slm_elapsed(slm_phase_started);
  if (slm_status.step <= SLM_POWER_BOOT) {
    comm_power_v5_poll(now_ms);
    uint8_t p = comm_power_v5_state();
    if (p != slm_status.step + 1u) {
      slm_finish_step();
      if (p == COMM_PWR_READY) slm_phase(SLM_PHASE_SETUP, SLM_AT_READY);
      else slm_step((uint8_t)(p - 1u));
    }
    goto end;
  }
  if (slm_done) slm_completed();
  if (slm_status.state == SLM_FAILED) goto end;
  if (slm_status.step == SLM_AT_READY && now_ms - slm_step_started >= comm_timeouts[COMM_T_AT_READY]) {
    slm_fail(SLM_ERR_TIMEOUT); goto end;
  }
  if (slm_status.step == SLM_NETWORK && now_ms - slm_step_started >= comm_timeouts[COMM_T_NETWORK]) {
    slm_fail(SLM_ERR_TIMEOUT); goto end;
  }
  if (slm_status.step == SLM_TCP && slm_status.state == SLM_STARTING && slm_tcp_ack) {
    if (now_ms - slm_tcp_started >= comm_timeouts[COMM_T_QIOPEN]) { slm_fail(SLM_ERR_TIMEOUT); goto end; }
    if (slm_open_seen) {
      slm_status.modem_code = slm_open_code;
      if (slm_open_code) { slm_fail(SLM_ERR_TCP); goto end; }
      slm_finish_step(); slm_diag.phases[SLM_PHASE_TCP] = slm_elapsed(slm_phase_started);
      slm_status.completed_phases |= (uint8_t)(1u << SLM_PHASE_TCP);
      slm_status.state = SLM_OPEN;
    }
    goto end; /* OK не означает открытие TCP. */
  }
  if (slm_status.state == SLM_OPEN) {
    if (slm_status.step != SLM_READ) {
      if (!slm_recv || !slm_capacity || !slm_handlers.binary) goto end;
      slm_step(SLM_READ);
    }
  }
  if (!slm_wait && !comm_at_busy() && (int32_t)(now_ms - slm_next) >= 0) {
    slm_answer = 0;
    slm_send();
  }
end:
  slm_in_poll = 0;
}
