/* Доработка: Кряжев А., 2026. Автомат SLM336, один запрос AT в работе. */
#include "modem_slm336.h"
#include "comm_at.h"
#include "comm_uart.h"
#include "comm_timeouts.h"
#include <string.h>
#include <stdio.h>
static slm_config_t slm_config;
static slm_handlers_t slm_handlers;
static slm_diag_t slm_diag;
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
static void slm_finish_timing(slm_timing_t *t, bool ok)
{
  t->duration_ms = slm_now - t->start_ms;
  if (ok) { t->success_start_ms = t->start_ms; t->success_ms = t->duration_ms; }
  else { t->failure_start_ms = t->start_ms; t->failure_ms = t->duration_ms; }
}
static void slm_finish_step(bool ok)
{
  slm_finish_timing(&slm_diag.steps[slm_diag.step], ok);
  slm_diag.completed_steps |= (uint32_t)1u << slm_diag.step;
  if (ok) {
    slm_diag.success_steps |= (uint32_t)1u << slm_diag.step;
    slm_diag.last_success_step = slm_diag.step;
  } else {
    slm_diag.failure_steps |= (uint32_t)1u << slm_diag.step;
    slm_diag.last_failure_step = slm_diag.step;
  }
}
static void slm_fail(uint8_t reason)
{
  if (slm_diag.state == SLM_FAILED) return;
  if (slm_diag.state == SLM_OPEN && slm_diag.step == SLM_READ && slm_handlers.read_done)
    slm_handlers.read_done(slm_bytes, reason);
  slm_finish_step(false);
  if (slm_diag.state == SLM_STARTING) {
    slm_finish_timing(&slm_diag.phases[slm_diag.phase], false);
    slm_diag.completed_phases |= (uint8_t)(1u << slm_diag.phase);
    slm_diag.failure_phases |= (uint8_t)(1u << slm_diag.phase);
  }
  slm_diag.reason = reason;
  slm_diag.state = SLM_FAILED;
}
static void slm_step(uint8_t step)
{
  slm_diag.step = step;
  slm_diag.steps[step].start_ms = slm_now;
  slm_diag.steps[step].duration_ms = 0;
  slm_answer = slm_wait = slm_done = 0;
  slm_next = slm_now;
}
static void slm_phase(uint8_t phase, uint8_t step)
{
  slm_finish_timing(&slm_diag.phases[slm_diag.phase], true);
  slm_diag.success_phases |= (uint8_t)(1u << slm_diag.phase);
  slm_diag.completed_phases |= (uint8_t)(1u << slm_diag.phase);
  slm_diag.phase = phase;
  slm_diag.phases[phase].start_ms = slm_now;
  slm_diag.phases[phase].duration_ms = 0;
  slm_step(step);
}
static uint16_t slm_binary(const uint8_t *p, uint16_t n)
{
  if (slm_diag.state != SLM_OPEN || slm_diag.step != SLM_READ || !slm_handlers.binary) return 0;
  uint16_t used = slm_handlers.binary(p, n);
  if (used <= n) slm_bytes = (uint16_t)(slm_bytes + used);
  return used;
}
static void slm_event(uint8_t kind, const uint8_t *p, uint16_t n, uint16_t detail)
{
  uint16_t pos, a, b;
  if (kind == COMM_AT_NO_ANSWER || kind == COMM_AT_STREAM_ERROR) {
    slm_diag.at_fault = kind == COMM_AT_NO_ANSWER ? SLM_ERR_NO_ANSWER : SLM_ERR_STREAM;
    slm_fail(slm_diag.at_fault); return;
  }
  if (kind == COMM_AT_DONE) {
    slm_wait = 0; slm_done = 1; slm_result = (uint8_t)detail;
    slm_diag.modem_code = n;
    slm_finish_timing(&slm_diag.at, detail == COMM_AT_OK);
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
        && (slm_diag.state == SLM_OPEN || slm_diag.step == SLM_TCP)) slm_fail(SLM_ERR_TCP);
    return;
  }
  if (slm_prefix(p, n, "+QIURC: \"pdpdeact\",")) {
    pos = sizeof("+QIURC: \"pdpdeact\",") - 1u;
    if (slm_num(p, n, &pos, &a) && pos == n && a == 1
        && slm_diag.step >= SLM_ACTIVATE) slm_fail(SLM_ERR_PDP);
    return;
  }
  if (slm_diag.state != SLM_STARTING) return;
  if (slm_diag.step == SLM_TCP && slm_prefix(p, n, "+QIOPEN:")) {
    pos = sizeof("+QIOPEN:") - 1u;
    if (slm_num(p, n, &pos, &a) && pos < n && p[pos++] == ','
        && slm_num(p, n, &pos, &b) && pos == n && a == 0) {
      slm_open_seen = 1; slm_open_code = b;
    }
  } else if (slm_diag.step == SLM_MODEL) {
    if (slm_prefix(p, n, "SLM336") || slm_prefix(p, n, "MEIG_SLM336")) slm_answer = 1;
  } else if (slm_diag.step == SLM_SIM) {
    if (slm_is(p, n, "+CPIN: READY")) slm_answer = 1;
    else if (slm_is(p, n, "+CPIN: SIM PIN")) slm_answer = 2;
    else if (slm_is(p, n, "+CPIN: SIM PUK")) slm_answer = 3;
    else if (slm_is(p, n, "+CME ERROR: SIM not inserted")
             || slm_is(p, n, "+CME ERROR: SIM failure")) slm_answer = 4;
  } else if (slm_diag.step == SLM_NETWORK) {
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
  } else if (slm_diag.step == SLM_IMSI) {
    if (n == 15) {
      for (pos = 0; pos < n; ++pos) if (p[pos] < '0' || p[pos] > '9') return;
      slm_answer = 1;
      slm_mnc = !memcmp(p, "250", 3) ? (uint8_t)((p[3] - '0') * 10u + p[4] - '0') : 0;
    }
  } else if (slm_diag.step == SLM_URC_PORT) {
    if (slm_prefix(p, n, "+QURCCFG: \"urcport\","))
      slm_answer = slm_is(p, n, "+QURCCFG: \"urcport\",\"uart1\"") ? 1 : 2;
  } else if (slm_diag.step == SLM_RAT_QUERY) {
    if (slm_prefix(p, n, "+QCFG: \"nwscanmode\",")) {
      pos = sizeof("+QCFG: \"nwscanmode\",") - 1u;
      if (slm_num(p, n, &pos, &a) && pos == n && a <= 4)
        slm_answer = a == slm_config.network_type ? 1 : 2;
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
bool modem_slm336_start(const slm_config_t *config, const slm_handlers_t *handlers, uint32_t now_ms)
{
  static const comm_at_handlers_t at_handlers = {slm_event, slm_binary};
  if (slm_in_poll || !config || !handlers || !handlers->gpio || !config->port
      || (config->modem_type != 2 && config->modem_type != 0 && config->modem_type != 255)
      || config->network_type > 4 || !slm_field(config->host, sizeof(config->host), true)
      || !slm_field(config->apn, sizeof(config->apn), false)
      || !slm_field(config->login, sizeof(config->login), false)
      || !slm_field(config->password, sizeof(config->password), false)
      || slm_diag.state == SLM_STARTING || slm_diag.state == SLM_OPEN) return false;
  slm_config = *config;
  slm_handlers = *handlers;
  slm_now = now_ms;
  /* История успешного/неудачного времени сохраняется между запусками. */
  for (unsigned i = 0; i < sizeof(slm_diag.steps) / sizeof(slm_diag.steps[0]); ++i)
    slm_diag.steps[i].start_ms = slm_diag.steps[i].duration_ms = 0;
  for (unsigned i = 0; i < sizeof(slm_diag.phases) / sizeof(slm_diag.phases[0]); ++i)
    slm_diag.phases[i].start_ms = slm_diag.phases[i].duration_ms = 0;
  slm_diag.completed_steps = 0; slm_diag.completed_phases = slm_diag.at_fault = 0;
  slm_diag.state = SLM_STARTING;
  slm_diag.phase = SLM_PHASE_POWER;
  slm_diag.phases[SLM_PHASE_POWER].start_ms = now_ms;
  slm_diag.reason = SLM_ERR_NONE; slm_diag.modem_code = 0;
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
  comm_uart_abort(); comm_power_v5_off(); slm_diag.state = SLM_IDLE;
}
bool modem_slm336_ready(void) { return slm_diag.state == SLM_OPEN; }
const slm_diag_t *modem_slm336_diag(void) { return &slm_diag; }
void modem_slm336_rx_capacity(uint16_t count) { slm_capacity = count > 256u ? 256u : count; }
static const char *slm_apn(void)
{
  static const struct { uint8_t mnc; const char *apn; } apns[] = {
    {1, "internet.mts.ru"}, {2, "internet"}, {99, "internet.beeline.ru"},
    {20, "internet.tele2.ru"}, {11, "internet.yota"}
  };
  if (slm_config.apn[0]) return slm_config.apn;
  for (unsigned i = 0; i < sizeof(apns) / sizeof(apns[0]); ++i)
    if (slm_mnc == apns[i].mnc) return apns[i].apn;
  return "";
}
static void slm_send(void)
{
  /* Временная строка только для форматирования: AT немедленно копирует её
     в свой единственный статический TX-буфер. Указатель на стек не остаётся. */
  char cmd[128];
  const char *s = NULL;
  int n = 0;
  uint32_t limit = comm_timeouts[COMM_T_AT_SIMPLE];
  switch (slm_diag.step) {
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
  case SLM_RAT_SET: n = snprintf(cmd, sizeof(cmd), "AT+QCFG=\"nwscanmode\",%u,1", slm_config.network_type); break;
  case SLM_NETWORK: s = slm_query ? "AT+CEREG?" : "AT+CREG?"; break;
  case SLM_IMSI: s = "AT+CIMI"; break;
  case SLM_APN:
    n = snprintf(cmd, sizeof(cmd), "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",%u",
                 slm_apn(), slm_config.login, slm_config.password,
                 slm_config.login[0] || slm_config.password[0] ? 1u : 0u); break;
  case SLM_ACTIVATE: s = "AT+QIACT=1"; limit = comm_timeouts[COMM_T_QIACT]; break;
  case SLM_TCP:
    n = snprintf(cmd, sizeof(cmd), "AT+QIOPEN=1,0,\"TCP\",\"%s\",%u,0,0", slm_config.host, slm_config.port);
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
                          slm_diag.step == SLM_READ ? COMM_AT_SLM_QIRD : COMM_AT_TEXT, limit};
  slm_diag.at.start_ms = slm_now; slm_diag.at.duration_ms = 0;
  if (comm_at_start(&req, slm_now) != COMM_AT_ACCEPTED) { slm_fail(SLM_ERR_RESPONSE); return; }
  slm_wait = 1;
}
static void slm_completed(void)
{
  uint8_t step = slm_diag.step;
  slm_done = 0;
  if (slm_result != COMM_AT_OK) {
    uint8_t reason = slm_result == COMM_AT_TIMEOUT ? SLM_ERR_TIMEOUT
                     : slm_result > COMM_AT_TIMEOUT ? SLM_ERR_STREAM : SLM_ERR_AT;
    if (step == SLM_SIM && (slm_diag.modem_code == 10 || slm_answer == 4)) reason = SLM_ERR_NO_SIM;
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
  slm_finish_step(true);
  if (step == SLM_READ) {
    slm_recv = slm_recv || slm_bytes || slm_read_urc;
    if (slm_handlers.read_done) slm_handlers.read_done(slm_bytes, SLM_ERR_NONE);
    slm_diag.step = SLM_TCP; return;
  }
  switch (step) {
  case SLM_AT_READY: slm_step(slm_config.modem_type == 2 ? SLM_ECHO : SLM_MODEL); break;
  case SLM_MODEL:
    slm_config.modem_type = 2;
    if (slm_handlers.type_found) slm_handlers.type_found(2);
    slm_step(SLM_ECHO); break;
  case SLM_URC_PORT:
    if (slm_answer == 2) slm_step(SLM_URC_SET);
    else slm_phase(SLM_PHASE_SIM, SLM_SIM);
    break;
  case SLM_URC_SET: slm_phase(SLM_PHASE_SIM, SLM_SIM); break;
  case SLM_SIM: slm_phase(SLM_PHASE_NETWORK, SLM_RAT_QUERY); break;
  case SLM_RAT_QUERY: slm_step(slm_answer == 2 ? SLM_RAT_SET : SLM_NETWORK); break;
  case SLM_NETWORK: slm_phase(SLM_PHASE_PDP, slm_config.apn[0] ? SLM_APN : SLM_IMSI); break;
  case SLM_ACTIVATE: slm_phase(SLM_PHASE_TCP, SLM_TCP); break;
  default: slm_step((uint8_t)(step + 1u)); break;
  }
}
void modem_slm336_poll(uint32_t now_ms)
{
  if (slm_in_poll || slm_diag.state == SLM_IDLE) return;
  slm_in_poll = 1; slm_now = now_ms;
  comm_at_poll(now_ms); /* Даже после ошибки шага завершает ресинхронизацию. */
  if (slm_diag.state == SLM_FAILED) goto end;
  if (slm_diag.state == SLM_STARTING || slm_diag.step == SLM_READ)
    slm_diag.steps[slm_diag.step].duration_ms = now_ms - slm_diag.steps[slm_diag.step].start_ms;
  if (slm_diag.state == SLM_STARTING)
    slm_diag.phases[slm_diag.phase].duration_ms = now_ms - slm_diag.phases[slm_diag.phase].start_ms;
  if (slm_diag.step <= SLM_POWER_BOOT) {
    comm_power_v5_poll(now_ms);
    uint8_t p = comm_power_v5_state();
    if (p != slm_diag.step + 1u) {
      slm_finish_step(true);
      if (p == COMM_PWR_READY) slm_phase(SLM_PHASE_SETUP, SLM_AT_READY);
      else slm_step((uint8_t)(p - 1u));
    }
    goto end;
  }
  if (slm_wait) slm_diag.at.duration_ms = now_ms - slm_diag.at.start_ms;
  if (slm_done) slm_completed();
  if (slm_diag.state == SLM_FAILED) goto end;
  if (slm_diag.step == SLM_AT_READY && now_ms - slm_diag.steps[SLM_AT_READY].start_ms >= comm_timeouts[COMM_T_AT_READY]) {
    slm_fail(SLM_ERR_TIMEOUT); goto end;
  }
  if (slm_diag.step == SLM_NETWORK && now_ms - slm_diag.steps[SLM_NETWORK].start_ms >= comm_timeouts[COMM_T_NETWORK]) {
    slm_fail(SLM_ERR_TIMEOUT); goto end;
  }
  if (slm_diag.step == SLM_TCP && slm_diag.state == SLM_STARTING && slm_tcp_ack) {
    if (now_ms - slm_tcp_started >= comm_timeouts[COMM_T_QIOPEN]) { slm_fail(SLM_ERR_TIMEOUT); goto end; }
    if (slm_open_seen) {
      slm_diag.modem_code = slm_open_code;
      if (slm_open_code) { slm_fail(SLM_ERR_TCP); goto end; }
      slm_finish_step(true); slm_finish_timing(&slm_diag.phases[SLM_PHASE_TCP], true);
      slm_diag.success_phases |= (uint8_t)(1u << SLM_PHASE_TCP);
      slm_diag.completed_phases |= (uint8_t)(1u << SLM_PHASE_TCP);
      slm_diag.state = SLM_OPEN;
    }
    goto end; /* OK не означает открытие TCP. */
  }
  if (slm_diag.state == SLM_OPEN) {
    if (slm_diag.step != SLM_READ) {
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
