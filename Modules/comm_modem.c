/* Доработка: Кряжев А., 2026. Автомат SLM336, один запрос AT в работе. */
#include "comm_modem.h"
#include "comm_at.h"
#include "comm_uart.h"
#include "comm_timeouts.h"
#include <string.h>
static const comm_modem_profile_t *profile;
static slm_settings_t slm_settings;
/* GPIO хранит автомат питания; ядру нужны только три уведомления. */
static struct {
  uint16_t (*binary)(const uint8_t *, uint16_t);
  void (*read_done)(uint16_t, uint8_t);
  void (*type_found)(uint8_t);
} slm_handlers;
static slm_diag_t slm_diag;
static slm_status_t slm_status;
typedef char slm_diag_budget[(sizeof(slm_diag_t) <= 128u) ? 1 : -1];
static uint32_t slm_step_started, slm_phase_started, slm_attempt_started;
static uint32_t slm_now, slm_next, slm_tcp_started;
static uint16_t slm_capacity, slm_bytes, slm_open_code;
static uint8_t slm_done, slm_result, slm_answer, slm_query;
static uint8_t slm_recv, slm_open_seen, slm_tcp_ack, slm_in_poll;
static uint8_t slm_net_registered, slm_mnc;
static uint8_t slm_external, slm_hold;
static void (*slm_transport_event)(uint8_t, const uint8_t *, uint16_t, uint16_t);

/* Число с проверкой границ; окончания проверяет вызывающая сторона. */
static uint16_t slm_elapsed(uint32_t start)
{
  uint32_t ticks = (slm_now - start) / 10u;
  return ticks > UINT16_MAX ? UINT16_MAX : (uint16_t)ticks;
}
/* Единственная запись длительностей, завершений и кольца сбоев.
   STEP_COUNT+1 — текущий снимок, STEP_COUNT — завершение без нового шага. */
static void slm_record(uint8_t next, uint8_t reason)
{
  if (((next <= SLM_STEP_COUNT && (slm_status.state != SLM_OPEN || next != SLM_READ))
       || slm_status.state == SLM_STARTING || slm_status.step == SLM_READ)
      && slm_status.step < sizeof(slm_diag.steps) / sizeof(slm_diag.steps[0]))
    slm_diag.steps[slm_status.step] = slm_elapsed(slm_step_started);
  if (slm_status.state == SLM_STARTING && slm_status.phase < sizeof(slm_diag.phases) / sizeof(slm_diag.phases[0]))
    slm_diag.phases[slm_status.phase] = slm_elapsed(slm_phase_started);
  if (next > SLM_STEP_COUNT) return;
  if (slm_status.step < 32u) slm_status.completed_steps |= (uint32_t)1u << slm_status.step;
  uint8_t phase = next == SLM_STEP_COUNT ? SLM_PHASE_COUNT
    : next < SLM_AT_READY ? SLM_PHASE_POWER : (uint8_t)(profile->steps[next - SLM_AT_READY].action >> 4);
  if (slm_status.state == SLM_STARTING && phase != slm_status.phase) {
    slm_status.completed_phases |= (uint8_t)(1u << slm_status.phase);
    if (phase < SLM_PHASE_COUNT) {
      slm_status.phase = phase; slm_phase_started = slm_now; slm_diag.phases[phase] = 0;
    }
  }
  if (reason) {
    uint8_t slot = slm_diag.failure_next;
    if (slot >= sizeof(slm_diag.failure_step)) slot = 0;
    slm_diag.failure_seconds[slot] = (slm_now - slm_attempt_started) / 1000u;
    slm_diag.failure_step[slot] = slm_status.step;
    slm_diag.failure_reason[slot] = reason;
    slm_diag.last_code = slm_status.modem_code;
    slm_diag.failure_next = slot + 1u == sizeof(slm_diag.failure_step) ? 0 : (uint8_t)(slot + 1u);
    if (slm_diag.failure_count < sizeof(slm_diag.failure_step)) ++slm_diag.failure_count;
  }
  if (next < sizeof(slm_diag.steps) / sizeof(slm_diag.steps[0])) slm_diag.steps[next] = 0;
}
static void slm_fail(uint8_t reason)
{
  if (slm_status.state == SLM_FAILED) return;
  if (slm_status.state == SLM_OPEN && slm_status.step == SLM_READ && slm_handlers.read_done)
    slm_handlers.read_done(slm_bytes, reason);
  slm_record(SLM_STEP_COUNT, reason);
  slm_status.reason = reason; slm_status.state = SLM_FAILED;
}
static void slm_step(uint8_t step)
{
  slm_record(step, 0);
  slm_status.step = step; slm_step_started = slm_now;
  slm_answer = slm_done = 0; slm_next = slm_now;
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
  if (slm_transport_event && (slm_external || kind == COMM_AT_URC
      || kind == COMM_AT_NO_ANSWER || kind == COMM_AT_STREAM_ERROR))
    slm_transport_event(kind, p, n, detail);
  if (slm_external && kind == COMM_AT_DONE) { slm_external = 0; return; }
  if (slm_external && kind == COMM_AT_LINE) return;
  if (kind == COMM_AT_NO_ANSWER || kind == COMM_AT_STREAM_ERROR) {
    slm_status.at_fault = kind == COMM_AT_NO_ANSWER ? SLM_ERR_NO_ANSWER : SLM_ERR_STREAM;
    slm_fail(slm_status.at_fault); return;
  }
  if (kind == COMM_AT_DONE) {
    slm_done = 1; slm_result = (uint8_t)detail;
    slm_status.modem_code = n; return;
  }
  uint32_t values[2];
  uint8_t step = slm_status.step;
  uint8_t action = step >= SLM_AT_READY ? profile->steps[step - SLM_AT_READY].action & 15u : MODEM_OK;
  if (comm_at_parse(p,n,profile->recv,values) && !values[0]) slm_recv = 1;
  else if (comm_at_parse(p,n,profile->closed,values) && !values[0]) {
    if (slm_status.state == SLM_OPEN || step == SLM_TCP) slm_fail(SLM_ERR_TCP);
  } else if (comm_at_parse(p,n,profile->pdp,values) && values[0] == 1) {
    if (step >= SLM_ACTIVATE) slm_fail(SLM_ERR_PDP);
  } else if (slm_status.state == SLM_STARTING) {
    switch (action) {
    case MODEM_TCP:
      if (comm_at_parse(p,n,profile->open,values) && !values[0]) {
        slm_open_seen = 1; slm_open_code = (uint16_t)values[1];
      }
      break;
    case MODEM_NETWORK:
      if (comm_at_parse(p,n,profile->network_reply[slm_query],values) && values[0] <= 2 && values[1] <= 5) {
        slm_answer = values[1] == 3 ? 2 : 1;
        if (values[1] == 1 || values[1] == 5) slm_net_registered = 1;
      }
      break;
    case MODEM_RAT:
      if (comm_at_parse(p,n,profile->rat,values) && values[0] <= 4)
        slm_answer = values[0] == slm_settings.number(SLM_CFG_NETWORK) ? 1 : 2;
      break;
    case MODEM_PORT:
      if (comm_at_prefix(p,n,profile->port)) slm_answer = comm_at_equal(p + strlen(profile->port),(uint16_t)(n - strlen(profile->port)),profile->port_ready) ? 1 : 2;
      break;
    case MODEM_MODEL:
      if (comm_at_prefix(p,n,profile->model) || (comm_at_prefix(p,n,"MEIG_")
          && comm_at_prefix(p + 5,(uint16_t)(n - 5),profile->model))) slm_answer = 1;
      break;
    case MODEM_SIM: {
      static const char * const answers[] = {"+CPIN: READY", "+CPIN: SIM PIN", "+CPIN: SIM PUK",
        "+CME ERROR: SIM not inserted", "+CME ERROR: SIM failure"};
      for (uint8_t i = 0; i < sizeof(answers) / sizeof(answers[0]); ++i)
        if (comm_at_equal(p,n,answers[i])) slm_answer = i < 3 ? (uint8_t)(i + 1) : 4;
      break;
    }
    case MODEM_IMSI:
      if (n != 15) break;
      for (uint16_t i = 0; i < n; ++i) if (p[i] < '0' || p[i] > '9') return;
      slm_answer = 1;
      slm_mnc = !memcmp(p,"250",3) ? (uint8_t)((p[3] - '0') * 10u + p[4] - '0') : 0;
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
  return slm_settings.text(field, &capacity);
}
static bool slm_valid_settings(void)
{
  uint16_t type = slm_settings.number(SLM_CFG_TYPE);
  if (!slm_settings.number(SLM_CFG_PORT) || (type != profile->type && type != 0 && type != 255)
      || slm_settings.number(SLM_CFG_NETWORK) > 4) return false;
  for (uint8_t field = SLM_CFG_HOST; field <= SLM_CFG_PASSWORD; ++field) {
    uint16_t capacity = 0;
    const char *p = slm_settings.text(field, &capacity);
    if (!p || !capacity || !slm_field(p, capacity, field == SLM_CFG_HOST)) return false;
  }
  return true;
}
bool comm_modem_start(const comm_modem_profile_t *selected, const slm_settings_t *settings, const slm_handlers_t *handlers, uint32_t now_ms)
{
  static const comm_at_handlers_t at_handlers = {slm_event, slm_binary};
  if (slm_in_poll || !selected || !settings || !settings->text || !settings->number
      || !handlers || !handlers->gpio
      || slm_status.state == SLM_STARTING || slm_status.state == SLM_OPEN) return false;
  profile = selected;
  slm_settings = *settings;
  if (!slm_valid_settings()) return false;
  slm_handlers.binary = handlers->binary;
  slm_handlers.read_done = handlers->read_done;
  slm_handlers.type_found = handlers->type_found;
  slm_now = slm_attempt_started = slm_phase_started = now_ms;
  /* Между попытками сохраняется только кольцо последних десяти сбоев. */
  memset(slm_diag.steps, 0, sizeof(slm_diag.steps));
  memset(slm_diag.phases, 0, sizeof(slm_diag.phases));
  memset(&slm_status, 0, sizeof(slm_status));
  slm_status.state = SLM_STARTING;
  slm_status.phase = SLM_PHASE_POWER;
  slm_capacity = slm_bytes = 0; slm_external = slm_hold = 0;
  slm_recv = slm_query = slm_open_seen = slm_tcp_ack = slm_net_registered = slm_mnc = 0;
  comm_uart_abort();
  comm_uart_init(115200);
  comm_at_init(&at_handlers, comm_timeouts[COMM_T_AT_RESYNC]);
  comm_power_v5_start(handlers->gpio, now_ms);
  slm_status.step = SLM_DISCHARGE; slm_step_started = slm_next = now_ms;
  slm_answer = slm_done = 0;
  return true;
}
void comm_modem_off(void)
{
  if (slm_in_poll) return;
  comm_uart_abort(); comm_power_v5_off(); slm_status.state = SLM_IDLE;
}
void comm_modem_transport(void (*event)(uint8_t, const uint8_t *, uint16_t, uint16_t))
{ slm_transport_event = event; }
bool comm_modem_busy(void) { return comm_at_busy(); }
void comm_modem_hold(bool hold) { slm_hold = (uint8_t)hold; }
void comm_modem_socket(bool open)
{
  slm_status.state = open ? SLM_OPEN : SLM_FAILED;
  slm_status.step = SLM_TCP; slm_status.reason = open ? SLM_ERR_NONE : SLM_ERR_TCP;
  slm_recv = slm_done = 0;
}
uint8_t comm_modem_command(const comm_at_request_t *request, uint32_t now_ms)
{
  if (comm_modem_busy() || (slm_status.state != SLM_OPEN && slm_status.state != SLM_FAILED))
    return COMM_AT_BUSY;
  if (slm_status.at_fault) return COMM_AT_INVALID;
  uint8_t r = comm_at_start(request, now_ms);
  if (r == COMM_AT_ACCEPTED) slm_external = 1;
  return r;
}
uint8_t comm_modem_payload(const uint8_t *data, uint16_t count, uint32_t now_ms)
{
  uint8_t r = comm_at_payload(data, count, comm_timeouts[COMM_T_SEND_RESULT], now_ms);
  if (r == COMM_AT_ACCEPTED) slm_external = 1;
  return r;
}
bool comm_modem_reserve(void) { return slm_settings.number && slm_settings.number(SLM_CFG_RESERVE); }
/* Шаблон: 1 сеть, 2 APN, 3/4 учётные данные, 5/6 адрес/порт,
   7 длина QIRD, 8 авторизация, 9 второй запрос регистрации сети. */
static const char *slm_apn(void);
static uint16_t slm_build(char *cmd, uint16_t size, uint8_t step, uint8_t address)
{
  const char *p = profile->commands + profile->steps[step - SLM_AT_READY].command;
  uint16_t n = comm_at_copy(cmd, size, 0, *p && *p != 'E' ? "AT+" : "AT");
  while (*p && n < size) {
    uint8_t token = (uint8_t)*p++;
    const char *text = NULL;
    uint16_t number = 0;
    switch (token) {
    case 1: number = slm_settings.number(SLM_CFG_NETWORK); break;
    case 2: text = slm_apn(); break;
    case 3: text = slm_text(SLM_CFG_LOGIN); break;
    case 4: text = slm_text(SLM_CFG_PASSWORD); break;
    case 5: text = slm_text(address ? SLM_CFG_HOST2 : SLM_CFG_HOST); break;
    case 6: number = slm_settings.number(address ? SLM_CFG_PORT2 : SLM_CFG_PORT); break;
    case 7: number = slm_capacity; break;
    case 8: number = slm_text(SLM_CFG_LOGIN)[0] || slm_text(SLM_CFG_PASSWORD)[0]; break;
    case 9: text = profile->network[slm_query]; break;
    default: cmd[n++] = (char)token; continue;
    }
    n = text ? comm_at_copy(cmd, size, n, text) : comm_at_decimal(cmd, size, n, number);
  }
  if (n < size) cmd[n] = 0;
  return n;
}
uint8_t comm_modem_tcp_open(uint8_t address, uint32_t now_ms)
{
  uint16_t capacity = 0;
  const char *host = slm_settings.text(address ? SLM_CFG_HOST2 : SLM_CFG_HOST, &capacity);
  if (!host || !capacity || !slm_settings.number(address ? SLM_CFG_PORT2 : SLM_CFG_PORT)
      || !slm_field(host, capacity, true)) return COMM_AT_INVALID;
  char cmd[128]; uint16_t n = slm_build(cmd, sizeof(cmd), SLM_TCP, address);
  if (n >= sizeof(cmd)) return COMM_AT_INVALID;
  comm_at_request_t req = {(const uint8_t *)cmd, n, COMM_AT_TEXT, comm_timeouts[COMM_T_AT_SIMPLE]};
  return comm_modem_command(&req, now_ms);
}
bool comm_modem_ready(void) { return slm_status.state == SLM_OPEN; }
const slm_diag_t *comm_modem_diag(void) { return &slm_diag; }
const slm_status_t *comm_modem_status(void) { return &slm_status; }
void comm_modem_rx_capacity(uint16_t count) { slm_capacity = count > 256u ? 256u : count; }
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
  if (!slm_valid_settings()) { slm_fail(SLM_ERR_RESPONSE); return; }
  char cmd[128];
  const comm_modem_step_t *entry = &profile->steps[slm_status.step - SLM_AT_READY];
  uint16_t n = slm_build(cmd, sizeof(cmd), slm_status.step, 0);
  if (n >= sizeof(cmd)) { slm_fail(SLM_ERR_RESPONSE); return; }
  comm_at_request_t req = {(const uint8_t *)cmd, n,
    slm_status.step == SLM_READ ? profile->read_mode : COMM_AT_TEXT, comm_timeouts[entry->timeout]};
  if (comm_at_start(&req, slm_now) != COMM_AT_ACCEPTED) { slm_fail(SLM_ERR_RESPONSE); return; }
  if (slm_status.step == SLM_TCP) slm_tcp_started = slm_now;
  if (slm_status.step == SLM_READ) { slm_recv = 0; slm_bytes = 0; }
}
static void slm_completed(void)
{
  uint8_t step = slm_status.step;
  const comm_modem_step_t *entry = &profile->steps[step - SLM_AT_READY];
  uint8_t action = entry->action & 15u;
  slm_done = 0;
  if (slm_result != COMM_AT_OK) {
    uint8_t reason = slm_result == COMM_AT_TIMEOUT ? SLM_ERR_TIMEOUT
                     : slm_result > COMM_AT_TIMEOUT ? SLM_ERR_STREAM : SLM_ERR_AT;
    if (action == MODEM_SIM && (slm_status.modem_code == 10 || slm_answer == 4)) reason = SLM_ERR_NO_SIM;
    if (action == MODEM_PDP && slm_result != COMM_AT_TIMEOUT) reason = SLM_ERR_PDP;
    if (action == MODEM_TCP && slm_result != COMM_AT_TIMEOUT) reason = SLM_ERR_TCP;
    if (action == MODEM_READY && slm_result <= COMM_AT_TIMEOUT) {
      slm_next = slm_now + comm_timeouts[COMM_T_AT_RETRY]; return;
    }
    slm_fail(reason); return;
  }
  if (action == MODEM_TCP) { slm_tcp_ack = 1; return; }
  if (action == MODEM_NETWORK) {
    if (!slm_answer) { slm_fail(SLM_ERR_RESPONSE); return; }
    if (slm_answer == 2) { slm_fail(SLM_ERR_NETWORK); return; }
    if (slm_query == 0) { slm_query = 1; return; }
    if (!slm_net_registered) {
      slm_query = 0; slm_next = slm_now + comm_timeouts[COMM_T_NETWORK_POLL]; return;
    }
  }
  if ((action == MODEM_MODEL || action == MODEM_IMSI || action == MODEM_RAT || action == MODEM_PORT)
      && !slm_answer) { slm_fail(action == MODEM_MODEL ? SLM_ERR_TYPE : SLM_ERR_RESPONSE); return; }
  if (action == MODEM_SIM && slm_answer != 1) {
    slm_fail(slm_answer == 2 ? SLM_ERR_PIN : slm_answer == 3 ? SLM_ERR_PUK : SLM_ERR_RESPONSE); return;
  }
  if (action == MODEM_READ) {
    slm_recv = slm_recv || slm_bytes;
    slm_record(SLM_STEP_COUNT, 0);
    if (slm_handlers.read_done) slm_handlers.read_done(slm_bytes, SLM_ERR_NONE);
    slm_status.step = SLM_TCP; return;
  }
  uint8_t alternate = action == MODEM_READY ? slm_settings.number(SLM_CFG_TYPE) == profile->type
    : action == MODEM_NETWORK ? slm_text(SLM_CFG_APN)[0] != 0 : slm_answer == 1;
  if (action == MODEM_MODEL && slm_handlers.type_found) slm_handlers.type_found(profile->type);
  uint8_t next = alternate ? entry->alternate : entry->next;
  slm_step(next);
}
void comm_modem_poll(uint32_t now_ms)
{
  if (slm_in_poll || slm_status.state == SLM_IDLE) return;
  slm_in_poll = 1; slm_now = now_ms;
  comm_at_poll(now_ms); /* Даже после ошибки шага завершает ресинхронизацию. */
  if (slm_status.state == SLM_FAILED) goto end;
  slm_record(SLM_STEP_COUNT + 1u, 0);
  if (slm_status.step <= SLM_POWER_BOOT) {
    comm_power_v5_poll(now_ms);
    uint8_t p = comm_power_v5_state();
    if (p != slm_status.step + 1u) {
      slm_step(p == COMM_PWR_READY ? SLM_AT_READY : (uint8_t)(p - 1u));
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
      slm_record(SLM_STEP_COUNT, 0);
      slm_status.state = SLM_OPEN;
    }
    goto end; /* OK не означает открытие TCP. */
  }
  if (slm_status.state == SLM_OPEN) {
    if (slm_status.step != SLM_READ) {
      if (slm_hold || !slm_recv || !slm_capacity || !slm_handlers.binary) goto end;
      slm_step(SLM_READ);
    }
  }
  if (!comm_at_busy() && (int32_t)(now_ms - slm_next) >= 0) {
    slm_answer = 0;
    slm_send();
  }
end:
  slm_in_poll = 0;
}
