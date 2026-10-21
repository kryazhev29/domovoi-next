/* Доработка: Кряжев А., 2026.
   Ведёт регистрацию, статусы и надзор за ответами сервера.
   Канал владеет передачей, обработчики приложения собирают прежние кадры.
   Отсутствие ответа запускает восстановление; журнал фиксирует смену связи.
*/
#include "usb_log.h"
#include "domovoi_diag.h"
#include "comm_server.h"
#include "comm_channel.h"
#include "comm_modem.h"
#include "comm_recover.h"
#include "comm_timeouts.h"
#include <string.h>
static struct {
  comm_server_handlers_t handlers;
  uint32_t now, registration_since, status_since, periodic_since, stable_since, silent_since;
  uint8_t state, registrations, misses, status_id, last_status_id, silent_step, address;
  struct { uint8_t status_wait:1, status_due:1, traffic_event:1, stable:1, first_status:1, rejected:1, probe_failed:1; } flags;
} sv;
enum { SV_OFF, SV_REG, SV_REG_WAIT, SV_ONLINE, SV_PING, SV_DNS, SV_SILENT };
/* Вне sv: переживают comm_server_init при перезапуске модема. */
static void (*sv_no_internet)(void);
static bool (*sv_link)(uint8_t, uint8_t);
static uint8_t sv_noted; /* записано до исправления: биты 0/1 - ошибка адреса 0/1, 2 - нет сети, 3 - нет интернета */
// Подключить восстановление при отсутствии интернета.
void comm_server_no_internet(void (*handler)(void)) { sv_no_internet = handler; }
/* Подключить запись изменений связи в журнал. */
void comm_server_link(bool (*handler)(uint8_t, uint8_t)) { sv_link = handler; }
/* Выдать событие связи с подавлением подтверждённых повторов. */
static void server_link(uint8_t kind, uint8_t address)
{
  uint8_t bit = kind == COMM_LINK_FAIL ? (uint8_t)(1u << address) : kind >= COMM_LINK_NO_NET ? (uint8_t)(1u << (kind - 1u)) : 0u;
  if ((sv_noted & bit)) return;
  usb_log_value(17, kind, address);
  if (sv_link(kind, address)) sv_noted |= bit;
}
/* Передать событие сети или интернета от восстановления. */
void comm_server_note(uint8_t kind) { server_link(kind, 0); }
/* Выдать адрес текущего серверного соединения. */
uint8_t comm_server_address(void) { return sv.address; }
/* Проверить интернет перед новой попыткой сервера; отсчёт между попытками, не после ping. */
/* Проверено на плате №1 12.10.2026 (ee0979b): сервер выключен 4 мин - ping ya.ru, событие 13 одно,
   основной -> сразу резервный, модем не перезапускался, возврат 4 мин 23 с.
   Изменено 2026-10-21 (35a4a72):
   повторы моста ограничивает канал без второй серверной паузы — на плате не проверено. */
static void server_check(void)
{
  sv.flags.status_wait = sv.flags.status_due = sv.flags.stable = sv.flags.probe_failed = 0;
  if (sv.state == SV_ONLINE) sv.silent_since = sv.now;
  /* Отказ основного до регистрации: резерв сразу, проверка интернета после него. */
  if (comm_channel_bridge() || (sv.state != SV_ONLINE && !sv.address && comm_modem_reserve())) {
    comm_channel_reopen(0); sv.state = SV_OFF;
  } else sv.state = SV_PING;
}
/* Применить открытие, потерю или результат проверки канала. */
static void server_channel_event(uint8_t kind, uint8_t value)
{
  if (kind == COMM_CH_OPENING) {
    comm_recover_data(COMM_DATA_BEGIN);
    /* Начало пары или первая попытка на прежний резервный после звонка. */
    if (!value || sv.address) sv.silent_since = sv.now;
    return;
  }
  if (kind == COMM_CH_OPENED) {
    sv.state = SV_REG; sv.registrations = sv.misses = 0; sv.address = value;
    usb_log_value(17, 5, value);
  } else if (kind == COMM_CH_CLOSED || kind == COMM_CH_FAULT) {
    usb_log_value(17, 6, sv.address);
    if (sv.state != SV_OFF) server_link((sv.state >= SV_ONLINE && !sv.registrations) ? COMM_LINK_LOST : COMM_LINK_FAIL, sv.address);
    sv.state = SV_OFF; sv.flags.stable = 0;
  } else {
    if (kind == COMM_CH_OPEN_FAILED) {
      comm_recover_data(COMM_DATA_FAILED);
      sv.address = value; server_link(COMM_LINK_FAIL, value); server_check(); return;
    }
    if (kind == COMM_CH_PROBE_DONE && (sv.state == SV_PING || sv.state == SV_DNS)) {
      if (value) sv.state = SV_SILENT;
      else if (sv.state == SV_PING) sv.state = SV_DNS;
      else {
        sv.flags.probe_failed = 1;
        comm_modem_failure(SLM_ERR_PROBE, 0);
        sv_no_internet();
      }
    }
    return;
  }
  sv.flags.status_wait = sv.flags.status_due = 0;
  sv.handlers.session(kind == COMM_CH_OPENED);
}
/* Начать регистрацию нового сеанса канала. */
void comm_server_init(const comm_server_handlers_t *handlers)
{
  if (sv.state >= SV_ONLINE && !sv.registrations && comm_channel_ready()) server_link(COMM_LINK_LOST, sv.address); /* перезапуск модема на связи */
  memset(&sv, 0, sizeof(sv));
  sv.handlers = *handlers;
  comm_channel_init(server_channel_event);
}
/* Сообщить, принял ли сервер регистрацию. */
bool comm_server_online(void) { return sv.state == SV_ONLINE; }
/* Срок принятой регистрации не сбрасывается периодическим ACK статуса. */
bool comm_server_settled(void)
{ return comm_server_online() && !(sv.flags.status_wait | sv.misses) &&
    sv.now - sv.registration_since >= comm_timeouts[COMM_T_USSD_STABLE]; }
uint8_t comm_server_condition(void)
{
  if (sv.flags.rejected) return LINK_IMEI_REJECTED;
  if (sv.state == SV_ONLINE) return LINK_ONLINE;
  if (sv.flags.probe_failed) return LINK_NO_INTERNET;
  return sv.state == SV_REG || sv.state == SV_REG_WAIT ? LINK_CONNECTING : LINK_NO_SERVER;
}
#if DOMOVOI_DIAG
/* Собрать состояние надзора для стендовой команды 111. */
uint32_t comm_server_diag(void)
{ return sv.state | (uint32_t)sv.misses << 8 | (uint32_t)sv.registrations << 16; }
#endif
/* Принять ответ регистрации и сбросить счёт попыток соединения. */
void comm_server_registered(uint8_t success)
{
  if (sv.state != SV_REG_WAIT) return;
  if (!success) {
    sv.flags.rejected = 1;
    usb_log_value(17, 7, sv.address);
    comm_modem_failure(SLM_ERR_IMEI, 0);
    server_link(COMM_LINK_FAIL, sv.address);
    server_check();
    return;
  }
  sv.flags.rejected = 0;
  sv.flags.probe_failed = 0;
  comm_channel_registered(); sv.state = SV_ONLINE; sv.flags.traffic_event = sv.misses = sv.registrations = sv.flags.status_wait = 0;
  sv.flags.stable = 1; sv.stable_since = sv.periodic_since = sv.now;
  sv.registration_since = sv.now;
  sv.flags.first_status = 1;
  server_link(COMM_LINK_UP, sv.address); sv_noted = 0;
}
/* Начать срок ожидания подтверждения отправленного статуса. */
void comm_server_status_sent(uint8_t unique_id)
{
  sv.last_status_id = unique_id;
  /* Внеочередной статус не продлевает уже идущий срок ответа. */
  if (!sv.flags.status_wait) {
    sv.status_id = unique_id; sv.flags.status_wait = 1; sv.status_since = sv.now;
  }
}
/* Принять подходящее подтверждение статуса. */
void comm_server_status_ack(uint8_t unique_id)
{
  if (!comm_server_online() || !sv.flags.status_wait || (unique_id != sv.status_id && unique_id != sv.last_status_id)) return;
  comm_channel_alive();
  comm_recover_data(COMM_DATA_OK);
  sv.flags.status_wait = sv.misses = sv.flags.traffic_event = 0;
  sv.state = SV_ONLINE;
  if (!sv.flags.stable) { sv.flags.stable = 1; sv.stable_since = sv.now; }
}
/* Продвинуть регистрацию, статусы и надзор ответов сервера. */
/* Проверено на плате №1 12.10.2026 (ee0979b): сервер выключен 4 мин - ping ya.ru, событие 13 одно,
   основной -> сразу резервный, модем не перезапускался, возврат 4 мин 23 с.
   Изменено 14.10.2026 (1a4b571): съём USB-журнала; на плате №1 14.10 видны TCP и регистрация, коды 8/9 не вызывались.
   Изменено 2026-10-21 (task/link-fixes): первая регистрация после старта без ответа через модем - питание модема сразу,
   без второго захода и резервного адреса — на плате не проверено.
   Изменено 2026-10-21 (908887a): учёт LTE-сеанса без ответа — на плате не проверено.
   Изменено 2026-10-21 (task/wifi-one): общий фильтр USB-журнала вместо повторных проверок — на плате не проверено. */
void comm_server_poll(uint32_t now_ms)
{
  sv.now = now_ms;
  if (comm_channel_call_pending() && comm_server_online()) sv.status_since = now_ms;
  if (comm_server_online() && sv.flags.status_wait &&
      now_ms - sv.status_since >= comm_timeouts[COMM_T_STATUS]) {
    if (comm_channel_expected()) comm_channel_reopen(0);
    else if (!sv.flags.status_due && sv.misses < 3) ++sv.misses;
    sv.flags.status_wait = 0;
  }
  /* Первый периодический статус — через 5 с регистрации, следующие — через 15 с.
     У внеочередного статуса свой срок ACK: он не сдвигает периодический запрос. */
  if (comm_server_online() && now_ms - sv.periodic_since >=
      comm_timeouts[sv.flags.first_status ? COMM_T_SERVER_REPLY : COMM_T_STATUS]) sv.flags.status_due = 1;
  comm_channel_priority(sv.flags.status_due || sv.state == SV_REG);
  comm_channel_poll(now_ms);
  if (sv.state == SV_OFF) return;
  if (sv.state == SV_REG_WAIT && now_ms - sv.registration_since >= comm_timeouts[COMM_T_SERVER_REPLY]) {
    usb_log_value(17, 8, sv.address);
    comm_modem_failure(SLM_ERR_SERVER, 0);
    comm_recover_data(COMM_DATA_FAILED);
    /* Через модем первая регистрация после старта без ответа - питание модема сразу (мост тут ни при чём). */
    if (!comm_channel_bridge() && comm_recover_first_silence()) { server_link(COMM_LINK_FAIL, sv.address); sv.state = SV_OFF; }
    else if (sv.registrations < 2) sv.state = SV_REG;
    else { server_link(COMM_LINK_FAIL, sv.address); server_check(); }
  }
  if (sv.state == SV_REG && !comm_tx_busy() && !comm_channel_call_pending()) {
    comm_channel_priority(false);
    sv.handlers.registration();
    if (comm_tx_busy()) { ++sv.registrations; sv.registration_since = now_ms; sv.state = SV_REG_WAIT; }
  }
  if (sv.flags.status_due && !comm_tx_busy() && !comm_channel_call_pending()) {
    comm_channel_priority(false);
    sv.handlers.status();
    if (comm_tx_busy()) { sv.flags.status_due = sv.flags.first_status = 0; sv.periodic_since = now_ms; }
  }
  comm_channel_priority(sv.flags.status_due || sv.state == SV_REG);
  /* Обработка входящих подтверждений также ждёт освобождения слота. */
  if (!comm_tx_busy() && !sv.flags.status_due) sv.handlers.process();
  if (sv.flags.stable && now_ms - sv.stable_since >= comm_timeouts[COMM_T_STABLE]) { sv.silent_step = 0; sv.flags.stable = 0; comm_channel_stable(); }
  if (comm_channel_call_pending()) return;
  if (sv.state == SV_ONLINE && sv.misses >= 3) {
    usb_log_value(17, 9, sv.address);
    comm_modem_failure(SLM_ERR_SERVER, 0);
    server_check();
  }
  /* Канал хранит принятую пробу сам; после отмены переоткрытием запрос повторяется. */
  if (sv.state == SV_PING || sv.state == SV_DNS)
    comm_channel_probe(sv.state == SV_PING ? COMM_CH_PING : COMM_CH_DNS);
  if (sv.state == SV_SILENT) {
    if (!sv.flags.traffic_event) sv.flags.traffic_event = sv.handlers.no_traffic();
    uint8_t t = sv.silent_step < 3 ? (uint8_t)(COMM_T_SERVER_RETRY_1 + sv.silent_step) : COMM_T_SERVER_RETRY_MAX;
    if (now_ms - sv.silent_since >= comm_timeouts[t]) {
      if (sv.silent_step < 3) ++sv.silent_step;
      // Пауза уже выдержана сервером: внутренний откат канала не прибавляем.
      comm_channel_stable();
      comm_channel_reopen(0);
      sv.state = SV_OFF; // Канал сообщит результат; таймер не прерывает идущий QIOPEN.
    }
  }
}
