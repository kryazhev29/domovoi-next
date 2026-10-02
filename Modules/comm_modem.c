/* Доработка: Кряжев А., 2026. Автомат SLM336, один запрос AT в работе. */
#include "comm_modem.h"
#include "comm_at.h"
#include "comm_uart.h"
#include "comm_timeouts.h"
#include <string.h>
/* Настройки и история живут между попытками; текущая попытка обнуляется целиком. */
static struct {
  struct {
    uint8_t done, result, answer, query, recv, open_seen, tcp_ack, in_poll;
    uint8_t net_registered, mnc, external, hold;
    uint16_t capacity, bytes, open_code;
    slm_status_t status;
    uint32_t step_started, phase_started, attempt_started, now, next, tcp_started;
  } attempt;
  const comm_modem_profile_t *profile;
  slm_settings_t settings;
  struct {
    uint16_t (*binary)(const uint8_t *, uint16_t);
    void (*read_done)(uint16_t, uint8_t);
    void (*type_found)(uint8_t);
  } handlers;
  void (*transport_event)(uint8_t, const uint8_t *, uint16_t, uint16_t);
  slm_diag_t diag;
} slm;
typedef char slm_diag_budget[(sizeof(slm_diag_t) <= 128u) ? 1 : -1];

/* Число с проверкой границ; окончания проверяет вызывающая сторона. */
static uint16_t slm_elapsed(uint32_t start)
{
  uint32_t ticks = (slm.attempt.now - start) / 10u;
  return ticks > UINT16_MAX ? UINT16_MAX : (uint16_t)ticks;
}
/* Единственная запись длительностей, завершений и кольца сбоев.
   STEP_COUNT+1 — текущий снимок, STEP_COUNT — завершение без нового шага. */
static void slm_record(uint8_t next, uint8_t reason)
{
  if (slm.attempt.status.step < sizeof(slm.diag.steps) / sizeof(slm.diag.steps[0])
      && (slm.attempt.status.state == SLM_STARTING || slm.attempt.status.step == SLM_READ
          || (next <= SLM_STEP_COUNT && (slm.attempt.status.state != SLM_OPEN || next != SLM_READ))))
    slm.diag.steps[slm.attempt.status.step] = slm_elapsed(slm.attempt.step_started);
  if (slm.attempt.status.state == SLM_STARTING && slm.attempt.status.phase < sizeof(slm.diag.phases) / sizeof(slm.diag.phases[0]))
    slm.diag.phases[slm.attempt.status.phase] = slm_elapsed(slm.attempt.phase_started);
  if (next > SLM_STEP_COUNT) return;
  if (slm.attempt.status.step < 32u) slm.attempt.status.completed_steps |= (uint32_t)1u << slm.attempt.status.step;
  uint8_t phase = next == SLM_STEP_COUNT ? SLM_PHASE_COUNT
    : next < SLM_AT_READY ? SLM_PHASE_POWER : (uint8_t)(slm.profile->steps[next - SLM_AT_READY].action >> 4);
  if (slm.attempt.status.state == SLM_STARTING && phase != slm.attempt.status.phase) {
    slm.attempt.status.completed_phases |= (uint8_t)(1u << slm.attempt.status.phase);
    if (phase < SLM_PHASE_COUNT) {
      slm.attempt.status.phase = phase; slm.attempt.phase_started = slm.attempt.now; slm.diag.phases[phase] = 0;
    }
  }
  if (reason) {
    uint8_t slot = slm.diag.failure_next;
    if (slot >= sizeof(slm.diag.failure_step)) slot = 0;
    slm.diag.failure_seconds[slot] = (slm.attempt.now - slm.attempt.attempt_started) / 1000u;
    slm.diag.failure_step[slot] = slm.attempt.status.step;
    slm.diag.failure_reason[slot] = reason;
    slm.diag.last_code = slm.attempt.status.modem_code;
    slm.diag.failure_next = slot + 1u == sizeof(slm.diag.failure_step) ? 0 : (uint8_t)(slot + 1u);
    if (slm.diag.failure_count < sizeof(slm.diag.failure_step)) ++slm.diag.failure_count;
  }
  if (next < sizeof(slm.diag.steps) / sizeof(slm.diag.steps[0])) slm.diag.steps[next] = 0;
}
static void slm_fail(uint8_t reason)
{
  if (slm.attempt.status.state == SLM_FAILED) return;
  if (slm.attempt.status.state == SLM_OPEN && slm.attempt.status.step == SLM_READ && slm.handlers.read_done)
    slm.handlers.read_done(slm.attempt.bytes, reason);
  slm_record(SLM_STEP_COUNT, reason);
  slm.attempt.status.reason = reason; slm.attempt.status.state = SLM_FAILED;
}
static void slm_step(uint8_t step)
{
  slm_record(step, 0);
  slm.attempt.status.step = step; slm.attempt.step_started = slm.attempt.now;
  slm.attempt.answer = slm.attempt.done = 0; slm.attempt.next = slm.attempt.now;
}
static uint16_t slm_binary(const uint8_t *p, uint16_t n)
{
  if (slm.attempt.status.state != SLM_OPEN || slm.attempt.status.step != SLM_READ || !slm.handlers.binary) return 0;
  uint16_t used = slm.handlers.binary(p, n);
  if (used <= n) slm.attempt.bytes = (uint16_t)(slm.attempt.bytes + used);
  return used;
}
static void slm_event(uint8_t kind, const uint8_t *p, uint16_t n, uint16_t detail)
{
  if (slm.transport_event && (slm.attempt.external || kind == COMM_AT_URC
      || kind == COMM_AT_NO_ANSWER || kind == COMM_AT_STREAM_ERROR))
    slm.transport_event(kind, p, n, detail);
  if (slm.attempt.external && kind == COMM_AT_DONE) { slm.attempt.external = 0; return; }
  if (slm.attempt.external && kind == COMM_AT_LINE) return;
  if (kind == COMM_AT_NO_ANSWER || kind == COMM_AT_STREAM_ERROR) {
    slm.attempt.status.at_fault = kind == COMM_AT_NO_ANSWER ? SLM_ERR_NO_ANSWER : SLM_ERR_STREAM;
    slm_fail(slm.attempt.status.at_fault); return;
  }
  if (kind == COMM_AT_DONE) {
    slm.attempt.done = 1; slm.attempt.result = (uint8_t)detail;
    slm.attempt.status.modem_code = n; return;
  }
  uint32_t values[2];
  uint8_t step = slm.attempt.status.step;
  uint8_t action = step >= SLM_AT_READY ? slm.profile->steps[step - SLM_AT_READY].action & 15u : MODEM_OK;
  if (comm_at_parse(p,n,slm.profile->recv,values) && !values[0]) slm.attempt.recv = 1;
  else if (comm_at_parse(p,n,slm.profile->closed,values) && !values[0]) {
    if (slm.attempt.status.state == SLM_OPEN || step == SLM_TCP) slm_fail(SLM_ERR_TCP);
  } else if (comm_at_parse(p,n,slm.profile->pdp,values) && values[0] == 1) {
    if (step >= SLM_ACTIVATE) slm_fail(SLM_ERR_PDP);
  } else if (slm.attempt.status.state == SLM_STARTING) {
    switch (action) {
    case MODEM_TCP:
      if (comm_at_parse(p,n,slm.profile->open,values) && !values[0]) {
        slm.attempt.open_seen = 1; slm.attempt.open_code = (uint16_t)values[1];
      }
      break;
    case MODEM_NETWORK:
      if (comm_at_parse(p,n,slm.profile->network_reply[slm.attempt.query],values) && values[0] <= 2 && values[1] <= 5) {
        slm.attempt.answer = values[1] == 3 ? 2 : 1;
        if (values[1] == 1 || values[1] == 5) slm.attempt.net_registered = 1;
      }
      break;
    case MODEM_RAT:
      if (comm_at_parse(p,n,slm.profile->rat,values) && values[0] <= 4)
        slm.attempt.answer = values[0] == slm.settings.number(SLM_CFG_NETWORK) ? 1 : 2;
      break;
    case MODEM_PORT:
      if (comm_at_prefix(p,n,slm.profile->port)) slm.attempt.answer = comm_at_equal(p + strlen(slm.profile->port),(uint16_t)(n - strlen(slm.profile->port)),slm.profile->port_ready) ? 1 : 2;
      break;
    case MODEM_MODEL:
      if (comm_at_prefix(p,n,slm.profile->model) || (comm_at_prefix(p,n,"MEIG_")
          && comm_at_prefix(p + 5,(uint16_t)(n - 5),slm.profile->model))) slm.attempt.answer = 1;
      break;
    case MODEM_SIM: {
      uint8_t answer = comm_at_choice(p, n, "+CPIN: READY\0+CPIN: SIM PIN\0+CPIN: SIM PUK\0"
        "+CME ERROR: SIM not inserted\0+CME ERROR: SIM failure\0");
      if (answer) slm.attempt.answer = answer < 4 ? answer : 4;
      break;
    }
    case MODEM_IMSI:
      if (n != 15) break;
      for (uint16_t i = 0; i < n; ++i) if (p[i] < '0' || p[i] > '9') return;
      slm.attempt.answer = 1;
      slm.attempt.mnc = !memcmp(p,"250",3) ? (uint8_t)((p[3] - '0') * 10u + p[4] - '0') : 0;
      break;
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
#ifdef __IAR_SYSTEMS_ICC__
#pragma inline=never
#endif
static const char *slm_text(uint8_t field)
{
  uint16_t capacity;
  return slm.settings.text(field, &capacity);
}
static bool slm_valid_settings(void)
{
  uint16_t type = slm.settings.number(SLM_CFG_TYPE);
  if (!slm.settings.number(SLM_CFG_PORT) || (type != slm.profile->type && type != 0 && type != 255)
      || slm.settings.number(SLM_CFG_NETWORK) > 4) return false;
  for (uint8_t field = SLM_CFG_HOST; field <= SLM_CFG_PASSWORD; ++field) {
    uint16_t capacity = 0;
    const char *p = slm.settings.text(field, &capacity);
    if (!p || !capacity || !slm_field(p, capacity, field == SLM_CFG_HOST)) return false;
  }
  return true;
}
bool comm_modem_start(const comm_modem_profile_t *selected, const slm_settings_t *settings, const slm_handlers_t *handlers, uint32_t now_ms)
{
  static const comm_at_handlers_t at_handlers = {slm_event, slm_binary};
  if (slm.attempt.in_poll || !selected || !settings || !settings->text || !settings->number
      || !handlers || !handlers->gpio
      || slm.attempt.status.state == SLM_STARTING || slm.attempt.status.state == SLM_OPEN) return false;
  slm.profile = selected;
  slm.settings = *settings;
  if (!slm_valid_settings()) return false;
  slm.handlers.binary = handlers->binary;
  slm.handlers.read_done = handlers->read_done;
  slm.handlers.type_found = handlers->type_found;
  /* Между попытками сохраняется только кольцо последних десяти сбоев. */
  memset(slm.diag.steps, 0, sizeof(slm.diag.steps));
  memset(slm.diag.phases, 0, sizeof(slm.diag.phases));
  memset(&slm.attempt, 0, sizeof(slm.attempt));
  slm.attempt.now = slm.attempt.attempt_started = slm.attempt.phase_started = now_ms;
  slm.attempt.status.state = SLM_STARTING;
  slm.attempt.status.phase = SLM_PHASE_POWER;
  comm_uart_abort();
  comm_uart_init(115200);
  comm_at_init(&at_handlers, comm_timeouts[COMM_T_AT_RESYNC]);
  comm_power_v5_start(handlers->gpio, now_ms);
  slm.attempt.status.step = SLM_DISCHARGE; slm.attempt.step_started = slm.attempt.next = now_ms;
  return true;
}
void comm_modem_off(void)
{
  if (slm.attempt.in_poll) return;
  comm_uart_abort(); comm_power_v5_off(); slm.attempt.status.state = SLM_IDLE;
}
void comm_modem_transport(void (*event)(uint8_t, const uint8_t *, uint16_t, uint16_t))
{ slm.transport_event = event; }
bool comm_modem_busy(void) { return comm_at_busy(); }
void comm_modem_hold(bool hold) { slm.attempt.hold = (uint8_t)hold; }
void comm_modem_socket(bool open)
{
  slm.attempt.status.state = open ? SLM_OPEN : SLM_FAILED;
  slm.attempt.status.step = SLM_TCP; slm.attempt.status.reason = open ? SLM_ERR_NONE : SLM_ERR_TCP;
  slm.attempt.recv = slm.attempt.done = 0;
}
uint8_t comm_modem_command(const comm_at_request_t *request, uint32_t now_ms)
{
  if (comm_modem_busy() || (slm.attempt.status.state != SLM_OPEN && slm.attempt.status.state != SLM_FAILED))
    return COMM_AT_BUSY;
  if (slm.attempt.status.at_fault) return COMM_AT_INVALID;
  uint8_t r = comm_at_start(request, now_ms);
  if (r == COMM_AT_ACCEPTED) slm.attempt.external = 1;
  return r;
}
uint8_t comm_modem_payload(const uint8_t *data, uint16_t count, uint32_t now_ms)
{
  uint8_t r = comm_at_payload(data, count, comm_timeouts[COMM_T_SEND_RESULT], now_ms);
  if (r == COMM_AT_ACCEPTED) slm.attempt.external = 1;
  return r;
}
bool comm_modem_reserve(void) { return slm.settings.number && slm.settings.number(SLM_CFG_RESERVE); }
/* Шаблон: 1 сеть, 2 APN, 3/4 учётные данные, 5/6 адрес/порт,
   7 длина QIRD, 8 авторизация, 9 второй запрос регистрации сети. */
static const char *slm_apn(void);
static uint16_t slm_build(char *cmd, uint16_t size, uint8_t step, uint8_t address)
{
  const char *p = slm.profile->commands + slm.profile->steps[step - SLM_AT_READY].command;
  uint16_t n = comm_at_copy(cmd, size, 0, *p && *p != 'E' ? "AT+" : "AT");
  while (*p && n < size) {
    uint8_t token = (uint8_t)*p++;
    const char *text = NULL;
    uint16_t number = 0;
    switch (token) {
    case 1: number = slm.settings.number(SLM_CFG_NETWORK); break;
    case 2: text = slm_apn(); break;
    case 3: text = slm_text(SLM_CFG_LOGIN); break;
    case 4: text = slm_text(SLM_CFG_PASSWORD); break;
    case 5: text = slm_text(address ? SLM_CFG_HOST2 : SLM_CFG_HOST); break;
    case 6: number = slm.settings.number(address ? SLM_CFG_PORT2 : SLM_CFG_PORT); break;
    case 7: number = slm.attempt.capacity; break;
    case 8: number = slm_text(SLM_CFG_LOGIN)[0] || slm_text(SLM_CFG_PASSWORD)[0]; break;
    case 9: text = slm.profile->network[slm.attempt.query]; break;
    default: cmd[n++] = (char)token; continue;
    }
    n = text ? comm_at_copy(cmd, size, n, text) : comm_at_decimal(cmd, size, n, number);
  }
  if (n < size) cmd[n] = 0;
  return n;
}
static uint8_t slm_issue(uint8_t step, uint8_t address, uint32_t now_ms, bool external)
{
  char cmd[128];
  uint16_t n = slm_build(cmd, sizeof(cmd), step, address);
  if (n >= sizeof(cmd)) return COMM_AT_INVALID;
  const comm_modem_step_t *entry = &slm.profile->steps[step - SLM_AT_READY];
  comm_at_request_t req = {(const uint8_t *)cmd, n,
    step == SLM_READ ? slm.profile->read_mode : COMM_AT_TEXT, comm_timeouts[entry->timeout]};
  return external ? comm_modem_command(&req, now_ms) : comm_at_start(&req, now_ms);
}
uint8_t comm_modem_tcp_open(uint8_t address, uint32_t now_ms)
{
  uint16_t capacity = 0;
  const char *host = slm.settings.text(address ? SLM_CFG_HOST2 : SLM_CFG_HOST, &capacity);
  if (!host || !capacity || !slm.settings.number(address ? SLM_CFG_PORT2 : SLM_CFG_PORT)
      || !slm_field(host, capacity, true)) return COMM_AT_INVALID;
  return slm_issue(SLM_TCP, address, now_ms, true);
}
bool comm_modem_ready(void) { return slm.attempt.status.state == SLM_OPEN; }
const slm_diag_t *comm_modem_diag(void) { return &slm.diag; }
const slm_status_t *comm_modem_status(void) { return &slm.attempt.status; }
void comm_modem_rx_capacity(uint16_t count) { slm.attempt.capacity = count > 256u ? 256u : count; }
static const char *slm_apn(void)
{
  static const struct { uint8_t mnc, offset; } apn_map[] = {
    {1, 0}, {2, 16}, {99, 25}, {20, 45}, {11, 63}
  };
  static const char apns[] = "internet.mts.ru\0internet\0internet.beeline.ru\0internet.tele2.ru\0internet.yota\0";
  if (slm_text(SLM_CFG_APN)[0]) return slm_text(SLM_CFG_APN);
  for (uint8_t i = 0; i < sizeof(apn_map) / sizeof(apn_map[0]); ++i)
    if (slm.attempt.mnc == apn_map[i].mnc) return apns + apn_map[i].offset;
  return "";
}
static void slm_send(void)
{
  if (!slm_valid_settings()) { slm_fail(SLM_ERR_RESPONSE); return; }
  if (slm_issue(slm.attempt.status.step, 0, slm.attempt.now, false) != COMM_AT_ACCEPTED) {
    slm_fail(SLM_ERR_RESPONSE); return;
  }
  if (slm.attempt.status.step == SLM_TCP) slm.attempt.tcp_started = slm.attempt.now;
  if (slm.attempt.status.step == SLM_READ) { slm.attempt.recv = 0; slm.attempt.bytes = 0; }
}
static void slm_completed(void)
{
  uint8_t step = slm.attempt.status.step;
  const comm_modem_step_t *entry = &slm.profile->steps[step - SLM_AT_READY];
  uint8_t action = entry->action & 15u;
  slm.attempt.done = 0;
  if (slm.attempt.result != COMM_AT_OK) {
    uint8_t reason = slm.attempt.result == COMM_AT_TIMEOUT ? SLM_ERR_TIMEOUT
                     : slm.attempt.result > COMM_AT_TIMEOUT ? SLM_ERR_STREAM : SLM_ERR_AT;
    if (action == MODEM_SIM && (slm.attempt.status.modem_code == 10 || slm.attempt.answer == 4)) reason = SLM_ERR_NO_SIM;
    if (action == MODEM_PDP && slm.attempt.result != COMM_AT_TIMEOUT) reason = SLM_ERR_PDP;
    if (action == MODEM_TCP && slm.attempt.result != COMM_AT_TIMEOUT) reason = SLM_ERR_TCP;
    if (action == MODEM_READY && slm.attempt.result <= COMM_AT_TIMEOUT) {
      slm.attempt.next = slm.attempt.now + comm_timeouts[COMM_T_AT_RETRY]; return;
    }
    slm_fail(reason); return;
  }
  switch (action) {
  case MODEM_TCP: slm.attempt.tcp_ack = 1; return;
  case MODEM_NETWORK:
    if (!slm.attempt.answer) { slm_fail(SLM_ERR_RESPONSE); return; }
    if (slm.attempt.answer == 2) { slm_fail(SLM_ERR_NETWORK); return; }
    if (slm.attempt.query == 0) { slm.attempt.query = 1; return; }
    if (!slm.attempt.net_registered) {
      slm.attempt.query = 0; slm.attempt.next = slm.attempt.now + comm_timeouts[COMM_T_NETWORK_POLL]; return;
    }
    break;
  case MODEM_MODEL: case MODEM_IMSI: case MODEM_RAT: case MODEM_PORT:
    if (!slm.attempt.answer) { slm_fail(action == MODEM_MODEL ? SLM_ERR_TYPE : SLM_ERR_RESPONSE); return; }
    break;
  case MODEM_SIM:
    if (slm.attempt.answer != 1) {
      slm_fail(slm.attempt.answer == 2 ? SLM_ERR_PIN : slm.attempt.answer == 3 ? SLM_ERR_PUK : SLM_ERR_RESPONSE); return;
    }
    break;
  case MODEM_READ:
    slm.attempt.recv = slm.attempt.recv || slm.attempt.bytes;
    slm_record(SLM_STEP_COUNT, 0);
    if (slm.handlers.read_done) slm.handlers.read_done(slm.attempt.bytes, SLM_ERR_NONE);
    slm.attempt.status.step = SLM_TCP; return;
  }
  uint8_t alternate = action == MODEM_READY ? slm.settings.number(SLM_CFG_TYPE) == slm.profile->type
    : action == MODEM_NETWORK ? slm_text(SLM_CFG_APN)[0] != 0 : slm.attempt.answer == 1;
  if (action == MODEM_MODEL && slm.handlers.type_found) slm.handlers.type_found(slm.profile->type);
  uint8_t next = alternate ? entry->alternate : entry->next;
  slm_step(next);
}
void comm_modem_poll(uint32_t now_ms)
{
  if (slm.attempt.in_poll || slm.attempt.status.state == SLM_IDLE) return;
  slm.attempt.in_poll = 1; slm.attempt.now = now_ms;
  comm_at_poll(now_ms); /* Даже после ошибки шага завершает ресинхронизацию. */
  if (slm.attempt.status.state == SLM_FAILED) goto end;
  slm_record(SLM_STEP_COUNT + 1u, 0);
  if (slm.attempt.status.step <= SLM_POWER_BOOT) {
    comm_power_v5_poll(now_ms);
    uint8_t p = comm_power_v5_state();
    if (p != slm.attempt.status.step + 1u) {
      slm_step(p == COMM_PWR_READY ? SLM_AT_READY : (uint8_t)(p - 1u));
    }
    goto end;
  }
  if (slm.attempt.done) slm_completed();
  if (slm.attempt.status.state == SLM_FAILED) goto end;
  if (slm.attempt.status.step == SLM_AT_READY && now_ms - slm.attempt.step_started >= comm_timeouts[COMM_T_AT_READY]) {
    slm_fail(SLM_ERR_TIMEOUT); goto end;
  }
  if (slm.attempt.status.step == SLM_NETWORK && now_ms - slm.attempt.step_started >= comm_timeouts[COMM_T_NETWORK]) {
    slm_fail(SLM_ERR_TIMEOUT); goto end;
  }
  if (slm.attempt.status.step == SLM_TCP && slm.attempt.status.state == SLM_STARTING && slm.attempt.tcp_ack) {
    if (now_ms - slm.attempt.tcp_started >= comm_timeouts[COMM_T_QIOPEN]) { slm_fail(SLM_ERR_TIMEOUT); goto end; }
    if (slm.attempt.open_seen) {
      slm.attempt.status.modem_code = slm.attempt.open_code;
      if (slm.attempt.open_code) { slm_fail(SLM_ERR_TCP); goto end; }
      slm_record(SLM_STEP_COUNT, 0);
      slm.attempt.status.state = SLM_OPEN;
    }
    goto end; /* OK не означает открытие TCP. */
  }
  if (slm.attempt.status.state == SLM_OPEN) {
    if (slm.attempt.status.step != SLM_READ) {
      if (slm.attempt.hold || !slm.attempt.recv || !slm.attempt.capacity || !slm.handlers.binary) goto end;
      slm_step(SLM_READ);
    }
  }
  if (!comm_at_busy() && (int32_t)(now_ms - slm.attempt.next) >= 0) {
    slm.attempt.answer = 0;
    slm_send();
  }
end:
  slm.attempt.in_poll = 0;
}
