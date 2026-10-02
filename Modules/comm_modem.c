/* Доработка: Кряжев А., 2026. Автомат SLM336, один запрос AT в работе. */
#include "comm_modem.h"
#include "comm_at.h"
#include "comm_uart.h"
#include "comm_timeouts.h"
#include <string.h>
/* Настройки и история живут между попытками; текущая попытка обнуляется целиком. */
static struct {
  struct {
    uint8_t done, result, answer, query, in_poll, recording;
    uint8_t net_registered, mnc;
    slm_status_t status;
    uint32_t step_started, phase_started, attempt_started, now, next;
  } attempt;
  const comm_modem_profile_t *profile;
  slm_settings_t settings;
  void (*type_found)(uint8_t);
  void (*imei_found)(const char *);
  comm_power_gpio_t gpio;
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
void comm_modem_record(uint8_t next, uint8_t reason, uint16_t code)
{
  if (slm.attempt.recording) {
    slm.diag.steps[slm.attempt.status.step] = slm_elapsed(slm.attempt.step_started);
    if (!(slm.attempt.status.completed_phases & (1u << slm.attempt.status.phase)))
      slm.diag.phases[slm.attempt.status.phase] = slm_elapsed(slm.attempt.phase_started);
  }
  if (next > SLM_STEP_COUNT) return;
  slm.attempt.status.modem_code = code;
  slm.attempt.status.reason = reason;
  slm.attempt.status.completed_steps |= (uint32_t)1u << slm.attempt.status.step;
  uint8_t phase = next == SLM_STEP_COUNT ? SLM_PHASE_COUNT
    : next < SLM_AT_READY ? SLM_PHASE_POWER : next >= SLM_TCP ? SLM_PHASE_TCP
    : (uint8_t)(slm.profile->steps[next - SLM_AT_READY].action >> 4);
  if (phase != slm.attempt.status.phase) {
    slm.attempt.status.completed_phases |= (uint8_t)(1u << slm.attempt.status.phase);
    if (phase < SLM_PHASE_COUNT) {
      slm.attempt.status.phase = phase; slm.attempt.phase_started = slm.attempt.now;
      if (!(slm.attempt.status.completed_phases & (1u << phase))) slm.diag.phases[phase] = 0;
    }
  }
  if (reason) {
    uint8_t slot = slm.diag.failure_next;
    if (slot >= sizeof(slm.diag.failure_step)) slot = 0;
    uint32_t seconds = (slm.attempt.now - slm.attempt.attempt_started) / 1000u;
    slm.diag.failure_seconds[slot] = seconds > UINT16_MAX ? UINT16_MAX : (uint16_t)seconds;
    slm.diag.failure_step[slot] = slm.attempt.status.step;
    slm.diag.failure_reason[slot] = reason;
    slm.diag.last_code = code;
    slm.diag.failure_next = slot + 1u == sizeof(slm.diag.failure_step) ? 0 : (uint8_t)(slot + 1u);
    if (slm.diag.failure_count < sizeof(slm.diag.failure_step)) ++slm.diag.failure_count;
  }
  if (next == SLM_STEP_COUNT && slm.attempt.status.step == SLM_READ && !reason)
    slm.attempt.status.step = SLM_TCP;
  slm.attempt.recording = next < SLM_STEP_COUNT;
  if (slm.attempt.recording) {
    slm.attempt.status.step = next; slm.attempt.step_started = slm.attempt.now;
    slm.diag.steps[next] = 0;
    slm.attempt.answer = slm.attempt.done = 0; slm.attempt.next = slm.attempt.now;
  }
}
static void slm_fail(uint8_t reason)
{
  if (slm.attempt.status.state == SLM_FAILED) return;
  comm_modem_record(SLM_STEP_COUNT, reason, slm.attempt.status.modem_code);
  slm.attempt.status.state = SLM_FAILED;
}


static void slm_event(uint8_t kind, const uint8_t *p, uint16_t n, uint16_t detail)
{
  if (kind == COMM_AT_DONE) {
    slm.attempt.done = 1; slm.attempt.result = (uint8_t)detail;
    slm.attempt.status.modem_code = n;
    /* Ошибка грамматики/UART оставляет AT в FAULT: обычный TCP close уже невозможен. */
    if (detail > COMM_AT_TIMEOUT) kind = COMM_AT_STREAM_ERROR;
  }
  if (slm.transport_event && (slm.attempt.status.state == SLM_READY || kind == COMM_AT_URC
      || kind == COMM_AT_NO_ANSWER || kind == COMM_AT_STREAM_ERROR))
    slm.transport_event(kind, p, n, detail);
  if (slm.attempt.status.state == SLM_READY) {
    if (kind == COMM_AT_NO_ANSWER || kind == COMM_AT_STREAM_ERROR) {
      slm.attempt.status.at_fault = (uint8_t)detail;
      uint8_t reason = kind == COMM_AT_NO_ANSWER ? SLM_ERR_NO_ANSWER : SLM_ERR_STREAM;
      if (!slm.transport_event) comm_modem_record(SLM_STEP_COUNT, reason, 0);
      slm.attempt.status.reason = reason; slm.attempt.status.state = SLM_FAILED;
      return;
    }
    uint32_t value;
    if (p && comm_at_parse(p,n,slm.profile->pdp,&value) && value == 1) slm_fail(SLM_ERR_PDP);
    return;
  }
  if (kind == COMM_AT_NO_ANSWER || kind == COMM_AT_STREAM_ERROR) {
    slm.attempt.status.at_fault = (uint8_t)detail;
    slm_fail(kind == COMM_AT_NO_ANSWER ? SLM_ERR_NO_ANSWER : SLM_ERR_STREAM); return;
  }
  if (kind == COMM_AT_DONE) return;
  /* После отказа поздние URC не индексируют таблицу шагом TCP/READ. */
  if (slm.attempt.status.state != SLM_STARTING) return;
  uint32_t values[2];
  uint8_t step = slm.attempt.status.step;
  uint8_t action = step >= SLM_AT_READY ? slm.profile->steps[step - SLM_AT_READY].action & 15u : MODEM_OK;
  if (comm_at_parse(p,n,slm.profile->pdp,values) && values[0] == 1) {
    if (step >= SLM_ACTIVATE) slm_fail(SLM_ERR_PDP);
  } else if (slm.attempt.status.state == SLM_STARTING) {
    switch (action) {
    case MODEM_NETWORK:
      if (comm_at_parse(p,n,slm.profile->network_reply[slm.attempt.query],values) && values[0] <= 2 && values[1] <= 5) {
        slm.attempt.answer = 1; /* Один домен может быть запрещён при регистрации в другом. */
        if (values[1] == 1 || values[1] == 5) slm.attempt.net_registered = 1;
      }
      break;
    case MODEM_RAT:
      if (comm_at_parse(p,n,slm.profile->rat,values) && values[0] <= 8)
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
    case MODEM_IMEI: {
      /* AT+GSN: строка из 15 цифр - IMEI для регистрации на сервере. */
      char imei[16];
      if (n != 15) break;
      for (uint16_t i = 0; i < n; ++i) { if (p[i] < '0' || p[i] > '9') return; imei[i] = (char)p[i]; }
      imei[15] = 0;
      slm.attempt.answer = 1;
      if (slm.imei_found) slm.imei_found(imei);
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
      || slm.settings.number(SLM_CFG_NETWORK) > 5) return false;
  for (uint8_t field = SLM_CFG_HOST; field <= SLM_CFG_PASSWORD; ++field) {
    uint16_t capacity = 0;
    const char *p = slm.settings.text(field, &capacity);
    if (!p || !capacity || !comm_at_field(p, capacity, field == SLM_CFG_HOST)) return false;
  }
  return true;
}
/* Новая попытка: обнуляется всё, кроме кольца последних десяти сбоев. */
static void slm_begin(bool power, uint32_t now_ms)
{
  static const comm_at_handlers_t at_handlers = {slm_event, NULL};
  memset(slm.diag.steps, 0, sizeof(slm.diag.steps));
  memset(slm.diag.phases, 0, sizeof(slm.diag.phases));
  memset(&slm.attempt, 0, sizeof(slm.attempt));
  slm.attempt.now = slm.attempt.attempt_started = slm.attempt.phase_started = now_ms;
  slm.attempt.status.state = SLM_STARTING; slm.attempt.recording = 1;
  comm_uart_abort();
  comm_uart_init(115200);
  comm_at_init(&at_handlers, comm_timeouts[COMM_T_AT_RESYNC]);
  if (power) {
    slm.attempt.status.phase = SLM_PHASE_POWER;
    comm_power_v5_start(slm.gpio, now_ms);
    slm.attempt.status.step = SLM_DISCHARGE;
  } else {
    slm.attempt.status.phase = SLM_PHASE_SETUP;
    slm.attempt.status.step = SLM_AT_READY;
  }
  slm.attempt.step_started = slm.attempt.next = now_ms;
}
bool comm_modem_start(const comm_modem_profile_t *selected, const slm_settings_t *settings, const slm_handlers_t *handlers, uint32_t now_ms)
{
  if (slm.attempt.in_poll || !selected || !settings || !settings->text || !settings->number
      || !handlers || !handlers->gpio
      || slm.attempt.status.state == SLM_STARTING || slm.attempt.status.state == SLM_READY) return false;
  slm.profile = selected;
  slm.settings = *settings;
  if (!slm_valid_settings()) return false;
  slm.type_found = handlers->type_found;
  slm.imei_found = handlers->imei_found;
  slm.gpio = handlers->gpio;
  slm_begin(true, now_ms);
  return true;
}
/* Перезапуск по сохранённым профилю и настройкам (восстановление связи).
   power - со снятием питания; без него - с AT при включённом модеме, а если
   питание уже снято, то всё равно с полного включения. */
bool comm_modem_restart(bool power, uint32_t now_ms)
{
  if (slm.attempt.in_poll || !slm.profile || !slm.gpio) return false;
  if (comm_power_v5_state() != COMM_PWR_READY) power = true;
  if (power) comm_power_v5_off();
  slm_begin(power, now_ms);
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


uint8_t comm_modem_request(const comm_at_request_t *request, uint32_t now_ms)
{
  if ((slm.attempt.status.state == SLM_STARTING && !slm.attempt.in_poll)
      || (slm.attempt.status.state != SLM_STARTING && slm.attempt.status.state != SLM_READY)) return COMM_AT_BUSY;
  uint8_t r = comm_at_start(request, now_ms);
  if (r == COMM_AT_ACCEPTED) slm.attempt.done = 0;
  return r;
}
uint8_t comm_modem_payload(const uint8_t *data, uint16_t count, uint32_t now_ms)
{
  uint8_t r = comm_at_payload(data, count, comm_timeouts[COMM_T_SEND_RESULT], now_ms);
  if (r == COMM_AT_ACCEPTED) slm.attempt.done = 0;
  return r;
}
uint8_t comm_modem_result(void)
{
  if (!slm.attempt.done) return UINT8_MAX;
  slm.attempt.done = 0; return slm.attempt.result;
}
uint8_t comm_modem_state(void) { return slm.attempt.status.state; }
const comm_modem_profile_t *comm_modem_profile(void) { return slm.profile; }
const slm_settings_t *comm_modem_settings(void) { return &slm.settings; }
bool comm_modem_reserve(void) { return slm.settings.number && slm.settings.number(SLM_CFG_RESERVE); }
/* Шаблон: 1 сеть, 2 APN, 3/4 учётные данные, 5 адрес сервера (host),
   6/7 число (порт или длина QIRD), 8 авторизация, 9 второй запрос регистрации.
   Одна сборка на модем и канал. */
static const char *slm_apn(void);
uint16_t comm_modem_build(char *cmd, uint16_t size, const char *p, const char *host, uint16_t value)
{
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
    case 5: text = host; break;
    case 6: case 7: number = value; break;
    case 8: number = slm_text(SLM_CFG_LOGIN)[0] || slm_text(SLM_CFG_PASSWORD)[0]; break;
    case 9: text = slm.profile->network[slm.attempt.query]; break;
    default: cmd[n++] = (char)token; continue;
    }
    n = text ? comm_at_copy(cmd, size, n, text) : comm_at_decimal(cmd, size, n, number);
  }
  if (n < size) cmd[n] = 0;
  return n;
}
static uint8_t slm_issue(uint8_t step)
{
  char cmd[128];
  uint16_t n = comm_modem_build(cmd, sizeof(cmd),
    slm.profile->commands + slm.profile->steps[step - SLM_AT_READY].command, NULL, 0);
  if (n >= sizeof(cmd)) return COMM_AT_INVALID;
  comm_at_request_t req = {(const uint8_t *)cmd, n, COMM_AT_TEXT,
    comm_timeouts[slm.profile->steps[step - SLM_AT_READY].timeout]};
  return comm_modem_request(&req, slm.attempt.now);
}

bool comm_modem_ready(void) { return slm.attempt.status.state == SLM_READY; }
const slm_diag_t *comm_modem_diag(void) { return &slm.diag; }
const slm_status_t *comm_modem_status(void) { return &slm.attempt.status; }

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
  uint8_t result = slm_issue(slm.attempt.status.step);
  /* После poll мог прийти хвост ответа/URC: BUSY означает повтор, не поломку. */
  if (result == COMM_AT_BUSY) {
    if (slm.attempt.now - slm.attempt.next >= comm_timeouts[COMM_T_AT_SIMPLE]) slm_fail(SLM_ERR_TIMEOUT);
  } else if (result != COMM_AT_ACCEPTED) slm_fail(SLM_ERR_RESPONSE);
}
static void slm_completed(void)
{
  uint8_t step = slm.attempt.status.step;
  const comm_modem_step_t *entry = &slm.profile->steps[step - SLM_AT_READY];
  uint8_t action = entry->action & 15u;
  slm.attempt.done = 0; slm.attempt.next = slm.attempt.now;
  /* MODEM_ANY: ответ модема есть (OK, ERROR, CME) - шаг пройден; срок или сбой потока - нет. */
  if (slm.attempt.result != COMM_AT_OK && !(action == MODEM_ANY && slm.attempt.result < COMM_AT_GOT_PROMPT)) {
    uint8_t reason = slm.attempt.result == COMM_AT_TIMEOUT ? SLM_ERR_TIMEOUT
                     : slm.attempt.result > COMM_AT_TIMEOUT ? SLM_ERR_STREAM : SLM_ERR_AT;
    if (action == MODEM_SIM && (slm.attempt.status.modem_code == 10 || slm.attempt.answer == 4)) reason = SLM_ERR_NO_SIM;
    if (action == MODEM_PDP && slm.attempt.result != COMM_AT_TIMEOUT) reason = SLM_ERR_PDP;
    if (action == MODEM_READY && slm.attempt.result <= COMM_AT_TIMEOUT) {
      slm.attempt.next = slm.attempt.now + comm_timeouts[COMM_T_AT_RETRY]; return;
    }
    slm_fail(reason); return;
  }
  switch (action) {
  case MODEM_NETWORK:
    if (!slm.attempt.answer) { slm_fail(SLM_ERR_RESPONSE); return; }
    if (slm.attempt.query == 0) { slm.attempt.query = 1; return; }
    if (!slm.attempt.net_registered) {
      slm.attempt.query = 0; slm.attempt.next = slm.attempt.now + comm_timeouts[COMM_T_NETWORK_POLL]; return;
    }
    break;
  case MODEM_MODEL: case MODEM_IMEI: case MODEM_IMSI: case MODEM_RAT: case MODEM_PORT:
    if (!slm.attempt.answer) { slm_fail(action == MODEM_MODEL ? SLM_ERR_TYPE : SLM_ERR_RESPONSE); return; }
    break;
  case MODEM_SIM:
    if (slm.attempt.answer != 1) {
      slm_fail(slm.attempt.answer == 2 ? SLM_ERR_PIN : slm.attempt.answer == 3 ? SLM_ERR_PUK : SLM_ERR_RESPONSE); return;
    }
    break;
  }
  uint8_t alternate = action == MODEM_READY ? slm.settings.number(SLM_CFG_TYPE) == slm.profile->type
    : action == MODEM_NETWORK ? slm_text(SLM_CFG_APN)[0] != 0 : slm.attempt.answer == 1;
  if (action == MODEM_MODEL && slm.type_found) slm.type_found(slm.profile->type);
  uint8_t next = alternate ? entry->alternate : entry->next;
  comm_modem_record(next, 0, slm.attempt.status.modem_code);
  if (next == SLM_STEP_COUNT) slm.attempt.status.state = SLM_READY;
}
void comm_modem_poll(uint32_t now_ms)
{
  if (slm.attempt.in_poll || slm.attempt.status.state == SLM_IDLE) return;
  slm.attempt.in_poll = 1; slm.attempt.now = now_ms;
  /* При снятом/нарастающем питании UART может ловить FE и шум загрузки.
     Это не ответ AT: начинаем чистый поток после выдержки POWER_BOOT. */
  if (slm.attempt.status.state == SLM_STARTING && slm.attempt.status.step <= SLM_POWER_BOOT) {
    comm_modem_record(SLM_STEP_COUNT + 1u, 0, slm.attempt.status.modem_code);
    comm_power_v5_poll(now_ms);
    uint8_t p = comm_power_v5_state();
    if (p != slm.attempt.status.step + 1u) {
      if (p == COMM_PWR_READY) comm_uart_init(115200);
      comm_modem_record(p == COMM_PWR_READY ? SLM_AT_READY : (uint8_t)(p - 1u), 0, slm.attempt.status.modem_code);
    }
    goto end;
  }
  comm_at_poll(now_ms); /* После ошибки шага завершаем ресинхронизацию. */
  if (slm.attempt.status.state != SLM_STARTING) goto end;
  comm_modem_record(SLM_STEP_COUNT + 1u, 0, slm.attempt.status.modem_code);
  if (slm.attempt.done) slm_completed();
  if (slm.attempt.status.state != SLM_STARTING) goto end;
  if (slm.attempt.status.step == SLM_AT_READY && now_ms - slm.attempt.step_started >= comm_timeouts[COMM_T_AT_READY]) {
    slm_fail(SLM_ERR_TIMEOUT); goto end;
  }
  if (slm.attempt.status.step == SLM_NETWORK && now_ms - slm.attempt.step_started >= comm_timeouts[COMM_T_NETWORK]) {
    slm_fail(SLM_ERR_TIMEOUT); goto end;
  }
  if (!comm_at_busy() && (int32_t)(now_ms - slm.attempt.next) >= 0) {
    slm.attempt.answer = 0;
    slm_send();
  }
end:
  slm.attempt.in_poll = 0;
}
