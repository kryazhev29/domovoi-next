/* Доработка: Кряжев А., 2026.
   Запускает единственный SLM336 и распределяет его ответы.
   Шаги питания и настройки заданы постоянной таблицей.
   Один владелец AT; служебные команды ждут свободного слота.
*/
#include "comm_modem.h"
#include "comm_at.h"
#include "comm_uart.h"
#include "comm_timeouts.h"
#include "comm_recover.h"
#include "comm_channel.h"
#include <string.h>
/* Настройки и история живут между попытками; текущая попытка обнуляется целиком. */
static struct {
  struct {
    uint8_t done, result, answer, query, recording;
    uint8_t net_registered;
    slm_status_t status;
    uint32_t step_started;
    uint32_t phase_started, attempt_started;
    uint32_t now, next;
  } attempt;
  /* Удачный источник и APN живут до выключения модема, включая потерю PDP. */
  struct { char network[101]; uint8_t source, locked, mnc; } apn;
  slm_settings_t settings;
  void (*type_found)(uint8_t);
  void (*imei_found)(const char *);
  comm_power_gpio_t gpio;
  void (*transport_event)(uint8_t, const uint8_t *, uint16_t, uint16_t);
  void (*urc[COMM_URC_COUNT])(const uint8_t *, uint16_t);
  uint8_t call_active;
  uint8_t session; /* растёт с каждой попыткой запуска */
  /* Служебная команда вне очереди канала: queued ждёт AT, prompted - получен '>',
     данные уходят в том же poll; результат забирает только владелец req. */
  struct { const comm_side_t *req; uint32_t started; uint8_t queued, running, prompted, done, result; uint16_t code; } side;
  slm_diag_t diag;
} slm;
#if DOMOVOI_DIAG
typedef char slm_diag_budget[(sizeof(slm_diag_t) <= 136u) ? 1 : -1];
#endif

#if DOMOVOI_DIAG
/* Преобразовать длительность шага в насыщаемое значение диагностики. */
static uint16_t slm_elapsed(uint32_t start)
{
  uint32_t ticks = (slm.attempt.now - start) / 10u;
  return ticks > UINT16_MAX ? UINT16_MAX : (uint16_t)ticks;
}
#endif
/* Записать сбой, не изменяя рабочий шаг модема. */
void comm_modem_failure(uint8_t reason, uint16_t code)
{
  uint8_t slot = slm.diag.failure_next;
  uint32_t seconds = (slm.attempt.now - slm.attempt.attempt_started) / 1000u;
  slm.diag.failures[slot] = slm.attempt.status.phase | (uint32_t)reason << 3 | seconds << 8;
  slm.diag.failure_next = slot == 9u ? 0 : (uint8_t)(slot + 1u);
  if (slm.diag.failure_count < 10u) ++slm.diag.failure_count;
#if DOMOVOI_DIAG
  slm.diag.last_code = code;
#else
  (void)code;
#endif
}
/* Единственная запись длительностей, завершений и кольца сбоев.
   STEP_COUNT+1 — текущий снимок, STEP_COUNT — завершение без нового шага. */
void comm_modem_record(uint8_t next, uint8_t reason, uint16_t code)
{
  if (slm.attempt.recording) {
#if DOMOVOI_DIAG
    slm.diag.steps[slm.attempt.status.step] = slm_elapsed(slm.attempt.step_started);
#endif
    if (!(slm.attempt.status.completed_phases & (1u << slm.attempt.status.phase)))
      slm.diag.phases[slm.attempt.status.phase] = slm.attempt.now - slm.attempt.phase_started;
  }
  if (next > SLM_STEP_COUNT) return;
  slm.attempt.status.modem_code = code;
  slm.attempt.status.reason = reason;
#if DOMOVOI_DIAG
  slm.attempt.status.completed_steps |= (uint32_t)1u << slm.attempt.status.step;
#endif
  uint8_t phase = next == SLM_STEP_COUNT ? SLM_PHASE_COUNT
    : next < SLM_AT_READY ? SLM_PHASE_POWER : next >= SLM_TCP ? SLM_PHASE_TCP
    : (uint8_t)(slm_steps[next - SLM_AT_READY].action >> 4);
  if (phase != slm.attempt.status.phase) {
    slm.attempt.status.completed_phases |= (uint8_t)(1u << slm.attempt.status.phase);
    if (phase < SLM_PHASE_COUNT) {
      slm.attempt.status.phase = phase;
      slm.attempt.phase_started = slm.attempt.now;
      if (!(slm.attempt.status.completed_phases & (1u << phase))) slm.diag.phases[phase] = 0;
    }
  }
  if (reason && !comm_channel_expected()) {
    comm_recover_failure(slm.attempt.status.step, reason, slm.attempt.now);
    comm_modem_failure(reason, code);
  }
  if (next == SLM_STEP_COUNT && slm.attempt.status.step == SLM_READ && !reason)
    slm.attempt.status.step = SLM_TCP;
  slm.attempt.recording = next < SLM_STEP_COUNT;
  if (slm.attempt.recording) {
    if (next == SLM_TCP) {
      slm.attempt.status.completed_phases &= (uint8_t)~(1u << SLM_PHASE_TCP);
      slm.attempt.phase_started = slm.attempt.now;
      slm.diag.phases[SLM_PHASE_TCP] = 0;
    }
    slm.attempt.status.step = next; slm.attempt.step_started = slm.attempt.now;
#if DOMOVOI_DIAG
    slm.diag.steps[next] = 0;
#endif
    slm.attempt.answer = slm.attempt.done = 0; slm.attempt.next = slm.attempt.now;
  }
}
/* Закончить попытку модема с заданной причиной. */
static void slm_fail(uint8_t reason)
{
  if (slm.attempt.status.state == SLM_FAILED) return;
  comm_modem_record(SLM_STEP_COUNT, reason, slm.attempt.status.modem_code);
  slm.attempt.status.state = SLM_FAILED;
}

/* Третье поле CGCONTRDP — APN активного CID 1, с кавычками или без них. */
static void slm_network_apn(const uint8_t *p, uint16_t n)
{
  static const char prefix[] = "+CGCONTRDP:";
  uint16_t pos = sizeof(prefix) - 1u;
  uint32_t value;
  if (slm.apn.network[0] || !comm_at_prefix(p, n, prefix)) return;
  for (uint8_t i = 0; i < 2; ++i) {
    if (!comm_at_scan(p, n, &pos, UINT16_MAX, &value) || (i == 0 && value != 1)
        || pos >= n || p[pos++] != ',') return;
  }
  pos = comm_at_spaces(p, n, pos);
  bool quoted = pos < n && p[pos] == '"';
  if (quoted) ++pos;
  uint16_t begin = pos;
  while (pos < n && p[pos] != (quoted ? '"' : ',')) ++pos;
  uint16_t len = pos - begin;
  if (!len || len >= sizeof(slm.apn.network)) return;
  if (quoted && (pos == n || (++pos < n && p[pos] != ','))) return;
  memcpy(slm.apn.network, p + begin, len); slm.apn.network[len] = 0;
  if (!comm_at_field(slm.apn.network, sizeof(slm.apn.network), true)) slm.apn.network[0] = 0;
}

/* Распределить AT между запуском, каналом и служебным владельцем. */
static void slm_event(uint8_t kind, const uint8_t *p, uint16_t n, uint16_t detail)
{
  if (kind == COMM_AT_URC)
    for (uint8_t i = 0; i < COMM_URC_COUNT; ++i) slm.urc[i](p, n);
  /* Во время одноимённого запроса движок выдаёт URC как LINE; формат одинарный. */
  if (kind == COMM_AT_URC || kind == COMM_AT_LINE) {
    uint32_t registered;
    /* Регистрация будит и отказавшую попытку; CREG не заменяет проверку пакетной сети. */
    if ((comm_at_parse(p,n,"+CGREG:\005",&registered) || comm_at_parse(p,n,"+CEREG:\005",&registered)
        || comm_at_parse(p,n,"+CGREG:\005,\"*",&registered) || comm_at_parse(p,n,"+CEREG:\005,\"*",&registered))
        && (registered == 1 || registered == 5)) {
      slm.attempt.net_registered = 1; slm.attempt.next = slm.attempt.now;
      comm_recover_registered();
    } else if ((comm_at_parse(p,n,"+CREG:\005",&registered) || comm_at_parse(p,n,"+CREG:\005,\"*",&registered))
        && (registered == 1 || registered == 5)) comm_recover_registered();
  }
  if (slm.side.running) {
    if (kind == COMM_AT_LINE) { if (slm.side.req->line) slm.side.req->line(p, n, detail); return; }
    if (kind == COMM_AT_DONE) {
      slm.side.running = 0;
      if (detail == COMM_AT_GOT_PROMPT && slm.side.req->payload) { slm.side.prompted = 1; return; }
      slm.side.done = 1; slm.side.result = (uint8_t)detail; slm.side.code = n;
      if (detail <= COMM_AT_TIMEOUT) return;
      kind = COMM_AT_STREAM_ERROR; /* поток потерян - как у любой команды */
    }
  }
  if (kind == COMM_AT_DONE) {
    slm.attempt.done = 1; slm.attempt.result = (uint8_t)detail;
    slm.attempt.status.modem_code = n;
    /* Ошибка грамматики/UART оставляет AT в FAULT: обычный TCP close уже невозможен. */
    if (detail > COMM_AT_TIMEOUT) kind = COMM_AT_STREAM_ERROR;
  }
  if (slm.attempt.status.state == SLM_READY || kind == COMM_AT_URC
      || kind == COMM_AT_NO_ANSWER || kind == COMM_AT_STREAM_ERROR)
    slm.transport_event(kind, p, n, detail);
  if (slm.attempt.status.state == SLM_READY) {
    if (kind == COMM_AT_NO_ANSWER || kind == COMM_AT_STREAM_ERROR) {
      slm.attempt.status.at_fault = (uint8_t)detail;
      uint8_t reason = kind == COMM_AT_NO_ANSWER ? SLM_ERR_NO_ANSWER : SLM_ERR_STREAM;
      slm.attempt.status.reason = reason; slm.attempt.status.state = SLM_FAILED;
      return;
    }
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
  uint8_t action = step >= SLM_AT_READY ? slm_steps[step - SLM_AT_READY].action & 15u : MODEM_OK;
  if (comm_at_parse(p,n,"+QIURC: \"pdpdeact\",\005",values) && values[0] == 1) {
    if (step >= SLM_ACTIVATE) slm_fail(SLM_ERR_PDP);
  } else {
    switch (action) {
    case MODEM_APN_QUERY:
      slm_network_apn(p, n);
      break;
    case MODEM_NETWORK:
      if (comm_at_parse(p,n,(slm.attempt.query ? "+CEREG:\005,\005|" : "+CGREG:\005,\005|"),values) && values[0] <= 2 && values[1] <= 5) {
        slm.attempt.answer = 1; /* Один домен может быть запрещён при регистрации в другом. */
        if (values[1] == 1 || values[1] == 5) slm.attempt.net_registered = 1;
      }
      break;
    case MODEM_RAT:
      if (comm_at_parse(p,n,"+QCFG: \"nwscanmode\",\005",values) && values[0] <= 8)
        slm.attempt.answer = values[0] == slm.settings.number(SLM_CFG_NETWORK) ? 1 : 2;
      break;
    case MODEM_PORT:
      if (comm_at_prefix(p,n,"+QURCCFG: \"urcport\",")) slm.attempt.answer = comm_at_equal(p + strlen("+QURCCFG: \"urcport\","),(uint16_t)(n - strlen("+QURCCFG: \"urcport\",")),"\"uart1\"") ? 1 : 2;
      break;
    case MODEM_MODEL:
      if (comm_at_prefix(p,n,"SLM336") || (comm_at_prefix(p,n,"MEIG_")
          && comm_at_prefix(p + 5,(uint16_t)(n - 5),"SLM336"))) slm.attempt.answer = 1;
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
      slm.imei_found(imei);
      break;
    }
    case MODEM_IMSI:
      if (n != 15) break;
      for (uint16_t i = 0; i < n; ++i) if (p[i] < '0' || p[i] > '9') return;
      slm.attempt.answer = 1;
      slm.apn.mnc = !memcmp(p,"250",3) ? (uint8_t)((p[3] - '0') * 10u + p[4] - '0') : 0;
      break;
    }
  }
}

#ifdef __IAR_SYSTEMS_ICC__
#pragma inline=never
#endif
/* Запросить строковую настройку у владельца конфигурации. */
static const char *slm_text(uint8_t field)
{
  uint16_t capacity;
  return slm.settings.text(field, &capacity);
}
/* Проверить внешнюю строку настройки перед первым построением AT. */
static bool slm_valid_field(uint8_t field)
{
  uint16_t capacity = 0;
  const char *p = slm.settings.text(field, &capacity);
  return p && capacity && comm_at_field(p, capacity, field == SLM_CFG_HOST || field == SLM_CFG_HOST2);
}
/* Проверить внешние настройки один раз при запуске модема. */
bool comm_modem_valid_settings(void)
{
  uint16_t type = slm.settings.number(SLM_CFG_TYPE);
  if (!slm.settings.number(SLM_CFG_PORT) || (type != MODEM_TYPE_SLM336 && type != 0 && type != 255)
      || slm.settings.number(SLM_CFG_NETWORK) > 5) return false;
  for (uint8_t field = SLM_CFG_HOST; field <= SLM_CFG_PASSWORD; ++field)
    if (!slm_valid_field(field)) return false;
  return !slm.settings.number(SLM_CFG_RESERVE)
    || (slm.settings.number(SLM_CFG_PORT2) && slm_valid_field(SLM_CFG_HOST2));
}
/* Перезапуск или выключение снимает служебную команду: владелец получает ERROR. */
static void slm_side_cancel(void)
{
  if (slm.side.queued || slm.side.running || slm.side.prompted) { slm.side.done = 1; slm.side.result = COMM_AT_ERROR; }
  slm.side.queued = slm.side.running = slm.side.prompted = 0;
}
/* Новая попытка: обнуляется всё, кроме кольца последних десяти сбоев. */
static void slm_begin(bool power, uint32_t now_ms)
{
  static const comm_at_handlers_t at_handlers = {slm_event, NULL};
#if DOMOVOI_DIAG
  memset(slm.diag.steps, 0, sizeof(slm.diag.steps));
#endif
  memset(slm.diag.phases, 0, sizeof(slm.diag.phases));
  memset(&slm.attempt, 0, sizeof(slm.attempt));
  slm_side_cancel(); slm.call_active = 0;
  if (power || !slm.apn.locked) memset(&slm.apn, 0, sizeof(slm.apn));
  ++slm.session;
  slm.attempt.now = now_ms;
  slm.attempt.attempt_started = now_ms;
  slm.attempt.phase_started = now_ms;
  slm.attempt.status.state = SLM_STARTING; slm.attempt.recording = 1;
  comm_uart_abort();
  comm_uart_init(115200);
  comm_at_init(&at_handlers, comm_timeouts[COMM_T_AT_RESYNC]);
  if (power) {
    slm.attempt.status.phase = SLM_PHASE_POWER;
    comm_power_v5_start(slm.gpio, now_ms);
    slm.attempt.status.step = (uint8_t)(comm_power_v5_state() - 1u);
  } else {
    slm.attempt.status.phase = SLM_PHASE_SETUP;
    slm.attempt.status.step = SLM_AT_READY;
  }
  slm.attempt.step_started = slm.attempt.next = now_ms;
}
/* Проверить настройки и включить единственный модем платы. */
bool modem_slm336_start(const slm_settings_t *settings, const slm_handlers_t *handlers, uint32_t now_ms)
{
  if (slm.attempt.status.state == SLM_STARTING || slm.attempt.status.state == SLM_READY) return false;
  slm.settings = *settings;
  if (!comm_modem_valid_settings()) return false;
  slm.type_found = handlers->type_found;
  slm.imei_found = handlers->imei_found;
  slm.gpio = handlers->gpio;
  slm_begin(true, now_ms);
  return true;
}
/* Перезапуск по сохранённым настройкам (восстановление связи).
   power - со снятием питания; без него - с AT при включённом модеме, а если
   питание уже снято, то всё равно с полного включения. */
bool comm_modem_restart(bool power, uint32_t now_ms)
{
  if (comm_power_v5_state() != COMM_PWR_READY) power = true;
  if (power) comm_power_v5_off();
  bool data_lost = !power && slm.attempt.status.reason == SLM_ERR_PDP && slm.attempt.status.step >= SLM_TCP;
  slm_begin(power, now_ms);
  /* После потери PDP настройки UART/SIM уже действуют: обычное восстановление начинается с сети. */
  if (data_lost) { slm.attempt.status.step = SLM_NETWORK; slm.attempt.status.phase = SLM_PHASE_NETWORK; }
  return true;
}
/* Снять питание и отменить прежний сеанс UART. */
void comm_modem_off(void)
{
  comm_uart_abort(); comm_power_v5_off(); slm.attempt.status.state = SLM_IDLE;
  slm_side_cancel();
}
/* Подключить обязательный обработчик канала. */
void comm_modem_transport(void (*event)(uint8_t, const uint8_t *, uint16_t, uint16_t))
{ slm.transport_event = event; }
/* Подключить постоянного потребителя уведомлений. */
void comm_modem_listen(uint8_t slot, void (*urc)(const uint8_t *, uint16_t)) { slm.urc[slot] = urc; }
/* Выдать поколение запуска для отмены старых запросов. */
uint8_t comm_modem_session(void) { return slm.session; }
/* После настройки порта служебные AT работают и в паузе отказавшего подключения. */
static bool slm_side_allowed(void)
{
  return slm.attempt.status.state == SLM_READY
    || (slm.attempt.status.state >= SLM_STARTING && slm.attempt.status.step >= SLM_SIM);
}
/* Занять служебный слот для его владельца. */
static bool slm_side_queue(const comm_side_t *req)
{
  if (slm.side.queued || slm.side.running
      || slm.side.prompted || (slm.side.done && slm.side.req != req) || !slm_side_allowed()) return false;
  slm.side.req = req; slm.side.queued = 1; slm.side.done = 0; slm.side.code = 0;
  return true;
}
/* Передать голосу приоритет в служебном слоте. */
void comm_modem_call(bool active) { slm.call_active = (uint8_t)active; }
/* Передать потерю мобильных данных обычному восстановлению. */
void comm_modem_data_failed(uint8_t reason) { slm_fail(reason); }
/* Поставить служебный запрос вне голосового вызова. */
bool comm_modem_side(const comm_side_t *req) { return !slm.call_active && slm_side_queue(req); }
/* Передать свободному AT команду или данные служебного владельца. */
static void slm_side_issue(void);
/* Поставить голосовую команду с приоритетом над запуском данных. */
bool comm_modem_call_side(const comm_side_t *req)
{
  if (!slm_side_queue(req)) return false;
  slm_side_issue(); return true;
}
/* Выдать время фактического начала голосовой команды. */
bool comm_modem_side_started(const comm_side_t *req, uint32_t *since)
{
  if (slm.side.req != req || slm.side.queued || !(slm.side.running || slm.side.done)) return false;
  *since = slm.side.started; return true;
}
/* Передать движку AT аварийный отбой. */
bool comm_modem_call_abort(uint32_t now_ms) { return comm_at_call_abort(now_ms); }
/* Выдать результат служебного запроса только его владельцу. */
uint8_t comm_modem_side_result(const comm_side_t *req)
{
  if (!slm.side.done || slm.side.req != req) return UINT8_MAX;
  slm.side.done = 0; return slm.side.result;
}
/* Снимок CMS/CME сохраняется до следующего владельца, а не следующей AT-команды. */
uint16_t comm_modem_side_code(void) { return slm.side.code; }
/* Вне очереди: между командами канала и шагами запуска, но не после '>'. */
static void slm_side_issue(void)
{
  const comm_side_t *s = slm.side.req;
  uint8_t r;
  if (slm.side.prompted) { /* модем ждёт данные: ничто другое не уходит в UART */
    r = comm_at_payload(s->payload, s->count, COMM_AT_TEXT, comm_timeouts[s->timeout], slm.attempt.now);
    if (r == COMM_AT_BUSY) return;
    slm.side.prompted = 0;
  } else {
    if (!slm.side.queued || comm_at_busy() || comm_at_prompt()) return;
    if (!slm_side_allowed()) { slm_side_cancel(); return; }
    comm_at_request_t req = {(const uint8_t *)s->cmd, (uint16_t)strlen(s->cmd), s->mode,
      comm_timeouts[s->timeout]};
    r = comm_at_start(&req, slm.attempt.now);
    if (r == COMM_AT_BUSY) return;
    slm.side.queued = 0;
  }
  if (r == COMM_AT_ACCEPTED) { slm.side.running = 1; slm.side.started = slm.attempt.now; }
  else { slm.side.done = 1; slm.side.result = COMM_AT_ERROR; }
}
/* Сообщить о занятом AT или восстановлении его границ. */
bool comm_modem_busy(void) { return comm_at_busy(); }

/* Передать команду канала после завершения настройки модема. */
uint8_t comm_modem_request(const comm_at_request_t *request, uint32_t now_ms)
{
  if (slm.attempt.status.state != SLM_READY || slm.side.prompted || slm.side.queued || slm.call_active) return COMM_AT_BUSY;
  uint8_t r = comm_at_start(request, now_ms);
  if (r == COMM_AT_ACCEPTED) slm.attempt.done = 0;
  return r;
}
/* Передать данные канала после приглашения QISEND. */
uint8_t comm_modem_payload(const uint8_t *data, uint16_t count, uint32_t now_ms)
{
  if (slm.side.prompted) return COMM_AT_BUSY;
  uint8_t r = comm_at_payload(data, count, COMM_AT_SEND, comm_timeouts[COMM_T_SEND_RESULT], now_ms);
  if (r == COMM_AT_ACCEPTED) slm.attempt.done = 0;
  return r;
}
/* Забрать завершение операции канала один раз. */
uint8_t comm_modem_result(void)
{
  if (!slm.attempt.done) return UINT8_MAX;
  slm.attempt.done = 0; return slm.attempt.result;
}
/* Выдать состояние запуска модема. */
uint8_t comm_modem_state(void) { return slm.attempt.status.state; }
/* Выдать назначенные геттеры настроек. */
const slm_settings_t *comm_modem_settings(void) { return &slm.settings; }
/* Узнать, разрешён ли резервный адрес сервера. */
bool comm_modem_reserve(void) { return slm.settings.number(SLM_CFG_RESERVE); }
/* Шаблон: 1 сеть, 2 APN, 3/4 учётные данные, 5 адрес сервера (host),
   6/7 число (порт или длина QIRD), 8 авторизация, 9 второй запрос регистрации.
   Одна сборка на модем и канал. */
static const char *slm_apn(void);
/* Собрать команду из постоянного шаблона и проверенных настроек. */
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
    case 3: text = slm.apn.source == SLM_APN_MANUAL ? slm_text(SLM_CFG_LOGIN) : ""; break;
    case 4: text = slm.apn.source == SLM_APN_MANUAL ? slm_text(SLM_CFG_PASSWORD) : ""; break;
    case 5: text = host; break;
    case 6: case 7: number = value; break;
    case 8: number = slm.apn.source == SLM_APN_MANUAL && (slm_text(SLM_CFG_LOGIN)[0] || slm_text(SLM_CFG_PASSWORD)[0]); break;
    case 9: text = (slm.attempt.query ? "CEREG?" : "CGREG?"); break;
    default: cmd[n++] = (char)token; continue;
    }
    n = text ? comm_at_copy(cmd, size, n, text) : comm_at_decimal(cmd, size, n, number);
  }
  if (n < size) cmd[n] = 0;
  return n;
}
/* Запустить команду текущего шага таблицы. */
static uint8_t slm_issue(uint8_t step)
{
  char cmd[128];
  uint16_t n = comm_modem_build(cmd, sizeof(cmd),
    slm_steps[step - SLM_AT_READY].command, NULL, 0);
  if (n >= sizeof(cmd)) return COMM_AT_INVALID;
  comm_at_request_t req = {(const uint8_t *)cmd, n, COMM_AT_TEXT,
    comm_timeouts[slm_steps[step - SLM_AT_READY].timeout]};
  return comm_at_start(&req, slm.attempt.now);
}

/* Сообщить, готовы ли SIM, сеть и мобильный интернет. */
bool comm_modem_ready(void) { return slm.attempt.status.state == SLM_READY; }
const slm_diag_t *comm_modem_diag(void)
{
#if DOMOVOI_DIAG
  slm.diag.apn_source = slm.apn.source;
#endif
  return &slm.diag;
}
/* Выдать текущее состояние и причину отказа модема. */
const slm_status_t *comm_modem_status(void) { return &slm.attempt.status; }

/* Данные выбранного источника; сетевой APN не подменяется таблицей. */
static const char *slm_apn(void)
{
  static const struct { uint8_t mnc, offset; } apn_map[] = {
    {1, 0}, {2, 16}, {99, 25}, {20, 45}, {11, 63}
  };
  static const char apns[] = "internet.mts.ru\0internet\0internet.beeline.ru\0internet.tele2.ru\0internet.yota\0";
  if (slm.apn.source == SLM_APN_MANUAL) return slm_text(SLM_CFG_APN);
  if (slm.apn.source == SLM_APN_NETWORK) return slm.apn.network;
  if (slm.apn.source != SLM_APN_MNC) return "";
  for (uint8_t i = 0; i < sizeof(apn_map) / sizeof(apn_map[0]); ++i)
    if (slm.apn.mnc == apn_map[i].mnc) return apns + apn_map[i].offset;
  return "";
}
/* Запустить шаг или дождаться освобождения входного потока. */
static void slm_send(void)
{
  uint8_t result = slm_issue(slm.attempt.status.step);
  /* После poll мог прийти хвост ответа/URC: BUSY означает повтор, не поломку. */
  if (result == COMM_AT_BUSY) {
    if (slm.attempt.now - slm.attempt.next >= comm_timeouts[COMM_T_AT_SIMPLE]) slm_fail(SLM_ERR_TIMEOUT);
  } else if (result != COMM_AT_ACCEPTED) slm_fail(SLM_ERR_RESPONSE);
}
/* Применить ответ и выбрать следующий шаг таблицы запуска. */
static void slm_completed(void)
{
  uint8_t step = slm.attempt.status.step;
  const comm_modem_step_t *entry = &slm_steps[step - SLM_AT_READY];
  uint8_t action = entry->action & 15u;
  slm.attempt.done = 0; slm.attempt.next = slm.attempt.now;
  /* MODEM_ANY: ответ модема есть (OK, ERROR, CME) - шаг пройден; срок или сбой потока - нет. */
  if (slm.attempt.result != COMM_AT_OK && !((action == MODEM_ANY || action == MODEM_APN_QUERY) && slm.attempt.result < COMM_AT_GOT_PROMPT)) {
    if (action == MODEM_PDP && slm.attempt.result <= COMM_AT_TIMEOUT
        && !slm.apn.locked && slm.apn.source < SLM_APN_EMPTY) {
      ++slm.apn.source;
      comm_modem_record(slm.apn.source == SLM_APN_NETWORK ? SLM_NET_APN
        : slm.apn.source == SLM_APN_MNC ? SLM_IMSI : SLM_APN, 0, slm.attempt.status.modem_code);
      return;
    }
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
  case MODEM_APN_QUERY:
    slm.attempt.answer = slm.attempt.result == COMM_AT_OK && slm.apn.network[0];
    if (!slm.attempt.answer) { slm.apn.network[0] = 0; slm.apn.source = SLM_APN_MNC; }
    break;
  case MODEM_PDP: slm.apn.locked = 1; break;
  case MODEM_NETWORK:
    if (!slm.attempt.answer) { slm_fail(SLM_ERR_RESPONSE); return; }
    if (slm.attempt.query == 0) { slm.attempt.query = 1; return; }
    if (!slm.attempt.net_registered) {
      slm.attempt.query = 0; slm.attempt.next = slm.attempt.now + comm_timeouts[COMM_T_NETWORK_POLL]; return;
    }
    if (!slm.apn.locked && slm.apn.source <= SLM_APN_NETWORK)
      slm.apn.source = slm_text(SLM_CFG_APN)[0] ? SLM_APN_MANUAL : SLM_APN_NETWORK;
    break;
  case MODEM_MODEL: case MODEM_IMEI: case MODEM_IMSI: case MODEM_RAT: case MODEM_PORT:
    if (!slm.attempt.answer) { slm_fail(action == MODEM_MODEL ? SLM_ERR_TYPE : SLM_ERR_RESPONSE); return; }
    if (action == MODEM_IMSI && !slm_apn()[0]) slm.apn.source = SLM_APN_EMPTY;
    break;
  case MODEM_SIM:
    if (slm.attempt.answer != 1) {
      slm_fail(slm.attempt.answer == 2 ? SLM_ERR_PIN : slm.attempt.answer == 3 ? SLM_ERR_PUK : SLM_ERR_RESPONSE); return;
    }
    break;
  }
  uint8_t alternate = action == MODEM_READY ? slm.settings.number(SLM_CFG_TYPE) == MODEM_TYPE_SLM336
    : action == MODEM_NETWORK ? slm.apn.locked || slm.apn.source != SLM_APN_NETWORK : slm.attempt.answer == 1;
  if (action == MODEM_MODEL) slm.type_found(MODEM_TYPE_SLM336);
  uint8_t next = alternate ? entry->alternate : entry->next;
  comm_modem_record(next, 0, slm.attempt.status.modem_code);
  if (next == SLM_STEP_COUNT) slm.attempt.status.state = SLM_READY;
}
/* Принять состояние питания из SysTick, продвинуть AT и попытку запуска. */
void comm_modem_poll(uint32_t now_ms)
{
  if (slm.attempt.status.state == SLM_IDLE) return;
  slm.attempt.now = now_ms;
  comm_modem_record(SLM_STEP_COUNT + 1u, 0, slm.attempt.status.modem_code);
  /* При снятом/нарастающем питании UART может ловить FE и шум загрузки.
     Это не ответ AT: начинаем чистый поток после выдержки POWER_BOOT. */
  if (slm.attempt.status.state == SLM_STARTING && slm.attempt.status.step <= SLM_POWER_BOOT) {
    uint8_t p = comm_power_v5_state();
    if (p != slm.attempt.status.step + 1u) {
      if (p == COMM_PWR_READY) {
        comm_uart_init(115200);
        /* USB 112: законченный срок питания, а не опоздание главного цикла. */
        slm.diag.phases[SLM_PHASE_POWER] = comm_power_v5_ready_at() - slm.attempt.phase_started;
        slm.attempt.status.completed_phases |= (uint8_t)(1u << SLM_PHASE_POWER);
      }
      comm_modem_record(p == COMM_PWR_READY ? SLM_AT_READY : (uint8_t)(p - 1u), 0, slm.attempt.status.modem_code);
    }
    return;
  }
  comm_at_poll(now_ms); /* После ошибки шага завершаем ресинхронизацию. */
  slm_side_issue();
  if (slm.attempt.status.state != SLM_STARTING) return;
  if (slm.attempt.done) slm_completed();
  if (slm.attempt.status.state != SLM_STARTING) return;
  if (slm.call_active && slm_side_allowed()) return;
  if (slm.attempt.status.step == SLM_AT_READY && now_ms - slm.attempt.step_started >= comm_timeouts[COMM_T_AT_READY]) {
    slm_fail(SLM_ERR_TIMEOUT); return;
  }
  if (slm.attempt.status.step == SLM_NETWORK && now_ms - slm.attempt.step_started >= comm_timeouts[COMM_T_NETWORK]) {
    slm_fail(SLM_ERR_TIMEOUT); return;
  }
  if (!comm_at_busy() && (int32_t)(now_ms - slm.attempt.next) >= 0) {
    slm.attempt.answer = 0;
    slm_send();
  }
}
