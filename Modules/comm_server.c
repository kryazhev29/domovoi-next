/* Доработка: Кряжев А., 2026.
   Ведёт регистрацию, статусы и надзор за ответами сервера.
   Канал владеет передачей, обработчики приложения собирают прежние кадры.
   Отсутствие ответа запускает восстановление; журнал фиксирует смену связи.
*/
#include "domovoi_diag.h"
#include "comm_server.h"
#include "comm_channel.h"
#include "comm_timeouts.h"
#include <string.h>
static struct {
  comm_server_handlers_t handlers;
  uint32_t now, registration_since, status_since, stable_since, silent_since;
  uint8_t state, registrations, misses, status_id, last_status_id, silent_step, address;
  struct { uint8_t status_wait:1, status_due:1, traffic_event:1, stable:1, check_pending:1; } flags;
} sv;
enum { SV_OFF, SV_REG, SV_REG_WAIT, SV_ONLINE, SV_PING, SV_DNS, SV_SILENT };
/* Вне sv: переживают comm_server_init при перезапуске модема. */
static void (*sv_no_internet)(void);
static bool (*sv_link)(uint8_t, uint8_t);
static uint8_t sv_noted; /* записано до исправления: биты 0/1 - ошибка адреса 0/1, 2 - нет сети, 3 - нет интернета */
void comm_server_no_internet(void (*handler)(void)) { sv_no_internet = handler; }
/* Подключить запись изменений связи в журнал. */
void comm_server_link(bool (*handler)(uint8_t, uint8_t)) { sv_link = handler; }
/* Выдать событие связи с подавлением подтверждённых повторов. */
static void server_link(uint8_t kind, uint8_t address)
{
  uint8_t bit = kind == COMM_LINK_FAIL ? (uint8_t)(1u << address) : kind >= COMM_LINK_NO_NET ? (uint8_t)(1u << (kind - 1u)) : 0u;
  if ((sv_noted & bit)) return;
  if (sv_link(kind, address)) sv_noted |= bit;
}
/* Передать событие сети или интернета от восстановления. */
void comm_server_note(uint8_t kind) { server_link(kind, 0); }
/* Выдать адрес текущего серверного соединения. */
uint8_t comm_server_address(void) { return sv.address; }
/* Применить открытие, потерю или результат проверки канала. */
static void server_channel_event(uint8_t kind, uint8_t value)
{
  if (kind == COMM_CH_OPENED) {
    sv.state = SV_REG; sv.registrations = sv.misses = 0; sv.address = value;
  } else if (kind == COMM_CH_CLOSED || kind == COMM_CH_FAULT) {
    if (sv.state != SV_OFF) server_link(sv.state >= SV_ONLINE ? COMM_LINK_LOST : COMM_LINK_FAIL, sv.address);
    sv.state = SV_OFF; sv.flags.stable = 0;
  } else {
    if (kind == COMM_CH_OPEN_FAILED) { server_link(COMM_LINK_FAIL, value); return; }
    if (kind == COMM_CH_PROBE_DONE && (sv.state == SV_PING || sv.state == SV_DNS)) {
      sv.flags.check_pending = 0;
      if (value) { sv.state = SV_SILENT; sv.silent_since = sv.now; }
      else if (sv.state == SV_PING) sv.state = SV_DNS;
      else sv_no_internet(); /* ping и DNS не прошли: решает восстановление */
    }
    return;
  }
  sv.flags.status_wait = sv.flags.status_due = sv.flags.check_pending = 0;
  sv.handlers.session(kind == COMM_CH_OPENED);
}
/* Начать регистрацию нового сеанса канала. */
void comm_server_init(const comm_server_handlers_t *handlers)
{
  if (sv.state >= SV_ONLINE) server_link(COMM_LINK_LOST, sv.address); /* перезапуск модема на связи */
  memset(&sv, 0, sizeof(sv));
  sv.handlers = *handlers;
  comm_channel_init(server_channel_event);
}
/* Сообщить, принял ли сервер регистрацию. */
bool comm_server_online(void) { return sv.state >= SV_ONLINE; }
#if DOMOVOI_DIAG
/* Собрать состояние надзора для стендовой команды 112. */
uint32_t comm_server_diag(void)
{ return sv.state | (uint32_t)sv.misses << 8 | (uint32_t)sv.registrations << 16; }
#endif
/* Принять ответ регистрации и сбросить счёт попыток соединения. */
void comm_server_registered(uint8_t success)
{
  if (sv.state != SV_REG_WAIT) return;
  if (!success) { comm_channel_reopen(0); return; }
  comm_channel_registered(); sv.state = SV_ONLINE; sv.flags.traffic_event = sv.misses = sv.flags.status_wait = 0;
  sv.flags.stable = 1; sv.stable_since = sv.status_since = sv.now;
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
  sv.flags.status_wait = sv.misses = sv.flags.traffic_event = sv.flags.check_pending = 0;
  sv.state = SV_ONLINE;
  if (!sv.flags.stable) { sv.flags.stable = 1; sv.stable_since = sv.now; }
}
/* Продвинуть регистрацию, статусы и надзор ответов сервера. */
void comm_server_poll(uint32_t now_ms)
{
  sv.now = now_ms;
  if (comm_channel_call_pending() && comm_server_online()) sv.status_since = now_ms;
  if (comm_server_online() && now_ms - sv.status_since >= comm_timeouts[COMM_T_STATUS]) {
    if (sv.flags.status_wait && comm_channel_expected()) comm_channel_reopen(0);
    else if (!sv.flags.status_due && sv.flags.status_wait && sv.misses < 3) ++sv.misses;
    sv.flags.status_wait = 0; sv.flags.status_due = 1;
  }
  comm_channel_priority(sv.flags.status_due || sv.state == SV_REG);
  comm_channel_poll(now_ms);
  if (sv.state == SV_OFF) return;
  if (sv.state == SV_REG_WAIT && now_ms - sv.registration_since >= comm_timeouts[COMM_T_SERVER_REPLY]) {
    if (sv.registrations < 2) sv.state = SV_REG;
    else { comm_channel_reopen(0); return; }
  }
  if (sv.state == SV_REG && !comm_tx_busy()) {
    comm_channel_priority(false);
    sv.handlers.registration();
    if (comm_tx_busy()) { ++sv.registrations; sv.registration_since = now_ms; sv.state = SV_REG_WAIT; }
  }
  if (sv.flags.status_due && !comm_tx_busy()) {
    comm_channel_priority(false);
    sv.handlers.status();
    if (comm_tx_busy()) { sv.flags.status_due = 0; sv.status_since = now_ms; }
  }
  comm_channel_priority(sv.flags.status_due || sv.state == SV_REG);
  /* Обработка входящих подтверждений также ждёт освобождения слота. */
  if (!comm_tx_busy() && !sv.flags.status_due) sv.handlers.process();
  if (sv.flags.stable && now_ms - sv.stable_since >= comm_timeouts[COMM_T_STABLE]) { sv.silent_step = 0; sv.flags.stable = 0; comm_channel_stable(); }
  if (comm_channel_call_pending()) return;
  if (sv.state == SV_ONLINE && sv.misses >= 3) { sv.state = SV_PING; sv.flags.stable = 0; }
  if ((sv.state == SV_PING || sv.state == SV_DNS) && !sv.flags.check_pending)
    sv.flags.check_pending = comm_channel_probe(sv.state == SV_PING ? COMM_CH_PING : COMM_CH_DNS);
  if (sv.state == SV_SILENT) {
    if (!sv.flags.traffic_event) sv.flags.traffic_event = sv.handlers.no_traffic();
    uint8_t t = sv.silent_step < 3 ? (uint8_t)(COMM_T_SERVER_RETRY_1 + sv.silent_step) : COMM_T_SERVER_RETRY_MAX;
    if (now_ms - sv.silent_since >= comm_timeouts[t]) {
      if (sv.silent_step < 3) ++sv.silent_step;
      comm_channel_reopen(0);
    }
  }
}
