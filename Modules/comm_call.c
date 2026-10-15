/* Доработка: Кряжев А., 2026.
   Обрабатывает входящий номер и короткий голосовой ответ.
   Проверка доступа предшествует ATA; таймеры отбоя не зависят от занятости флеш.
   Голос ждёт свободного AT, а настройка VoLTE выполняется вне вызова.
*/
#include "comm_call.h"
#include "comm_modem.h"
#include "comm_channel.h"
#include "comm_timeouts.h"
#include <string.h>
/* Действие и ответ независимы от TCP; DONE ограждает поздний CLIP до нового RING. */
enum { CALL_IDLE, CALL_WAIT_NUMBER, CALL_NUMBER, CALL_ANSWER, CALL_QUEUED,
       CALL_ANSWERING, CALL_HOLD, CALL_HANGUP, CALL_HANGING, CALL_CHECK, CALL_CHECKING, CALL_ABORT, CALL_DONE };
/* VoLTE модема: неизвестно до запроса, не поддержан - больше не спрашивать в этом запуске. */
enum { VOLTE_UNKNOWN = 0xFF, VOLTE_UNSUPPORTED = 0xFE };
static struct {
  bool (*caller)(const char *);
  uint8_t (*volte)(void);
  const comm_side_t *volte_op;
  uint32_t now, since, last, answered, born;
  char number[21];
  uint8_t state, session, volte_modem, volte_read, pending, seen, stopped;
} call;
static const comm_side_t call_answer = {"ATA", NULL, NULL, 0, COMM_AT_CALL, COMM_T_CALL_ANSWER};
static const comm_side_t call_hangup = {"AT+CHUP", NULL, NULL, 0, COMM_AT_CALL, COMM_T_CALL_ANSWER};
/* MeiG Common V2.24, 7.6: только OK без строк CLCC подтверждает отсутствие вызовов. */
static void call_line(const uint8_t *p, uint16_t n, uint16_t detail)
{
  (void)detail;
  if (comm_at_prefix(p, n, "+CLCC:")) call.seen = 1;
}
static const comm_side_t call_check[2] = {
  {"AT+CLCC", NULL, call_line, 0, COMM_AT_CALL, COMM_T_AT_SIMPLE},
  {"AT+CLCC", NULL, call_line, 0, COMM_AT_TEXT, COMM_T_AT_SIMPLE}};
/* Ответ настройки +SETVOLTE: <0|1>, руководство MeiG, раздел 11.11. */
static void volte_line(const uint8_t *p, uint16_t n, uint16_t detail)
{
  uint16_t pos = sizeof("+SETVOLTE:") - 1u; uint32_t v;
  (void)detail;
  if (comm_at_prefix(p, n, "+SETVOLTE:") && comm_at_scan(p, n, &pos, 1u, &v) && pos == n) call.volte_read = (uint8_t)v;
}
static const comm_side_t volte_query = {"AT+SETVOLTE?", NULL, volte_line, 0, COMM_AT_TEXT, COMM_T_VOLTE};
static const comm_side_t volte_set[2] = {
  {"AT+SETVOLTE=0", NULL, NULL, 0, COMM_AT_TEXT, COMM_T_VOLTE},
  {"AT+SETVOLTE=1", NULL, NULL, 0, COMM_AT_TEXT, COMM_T_VOLTE}};
/* +CLIP: "<номер>",<тип>,... (27.007 §7.18); номер - цифры и '+', может быть пустым. */
static bool call_number(const uint8_t *p, uint16_t n, char *out)
{
  uint16_t pos = comm_at_spaces(p, n, sizeof("+CLIP:") - 1u), len = 0;
  if (pos >= n || p[pos++] != '"') return false;
  for (; pos < n && p[pos] != '"'; ++pos) {
    if (len + 1u >= sizeof(call.number) || !((p[pos] >= '0' && p[pos] <= '9') || p[pos] == '+')) return false;
    out[len++] = (char)p[pos];
  }
  out[len] = 0;
  return pos < n;
}
/* URC только отмечают вызов: действие и команды выполняет главный цикл. */
static void call_begin(void)
{
  call.state = CALL_WAIT_NUMBER; call.since = call.now; call.pending = call.stopped = 0;
  call.born = call.now;
  call.number[0] = 0; comm_channel_call(true); comm_modem_call(true);
}
/* До физического ATA нужен обычный CHUP; аварийный отбой требует режима CALL. */
static void call_stop(void)
{
  call.pending = 0;
  if (call.stopped || call.state < CALL_WAIT_NUMBER || call.state > CALL_CHECKING) return;
  if (call.state <= CALL_QUEUED &&
      (call.state != CALL_QUEUED || !comm_modem_side_started(&call_answer, &call.since))) {
    comm_modem_side_result(&call_answer);
    call.since = call.now; call.answered = call.now - comm_timeouts[COMM_T_AT_SIMPLE];
    call.state = CALL_HANGUP;
  } else call.state = CALL_ABORT;
}
/* Принять звонок, номер или потерю голосового соединения. */
static void call_urc(const uint8_t *p, uint16_t n)
{
  char number[sizeof(call.number)];
  if (comm_at_equal(p,n,"NO CARRIER")) {
    call_stop();
    return;
  }
  if (call.state == CALL_DONE && call.now - call.last >= comm_timeouts[COMM_T_CALL_QUIET]) {
    call.state = CALL_IDLE; call.number[0] = 0;
  }
  if (comm_at_equal(p, n, "RING")) {
    if (call.state == CALL_IDLE || call.state == CALL_DONE) call_begin();
    call.last = call.now; return;
  }
  if (!comm_at_prefix(p, n, "+CLIP:") || !call_number(p, n, number)) return;
  call.last = call.now;
  if (call.state != CALL_IDLE && call.state != CALL_WAIT_NUMBER && !strcmp(number, call.number)) return;
  if (call.state == CALL_IDLE || call.state == CALL_DONE) call_begin();
  memcpy(call.number, number, strlen(number) + 1u); call.pending = 1;
  if (call.state == CALL_WAIT_NUMBER) call.state = CALL_NUMBER;
}
/* Назначить обработчик доступа и настройку VoLTE. */
void comm_call_init(bool (*caller)(const char *), uint8_t (*volte)(void))
{
  memset(&call, 0, sizeof(call));
  call.caller = caller; call.volte = volte;
  call.session = (uint8_t)(comm_modem_session() - 1u);
  comm_modem_listen(COMM_URC_CALL, call_urc);
}
/* Сообщить, продолжается ли вызов или его тихое завершение. */
bool comm_call_active(void) { return call.state != CALL_IDLE; }
/* Настройка VoLTE: в каждом запуске модема после SIM, вне вызова; меняется только при расхождении. */
static void call_volte(void)
{
  uint8_t session = comm_modem_session();
  if (session != call.session) { call.session = session; call.volte_modem = VOLTE_UNKNOWN; }
  if (call.volte_op) {
    uint8_t r = comm_modem_side_result(call.volte_op);
    if (r == UINT8_MAX) return;
    if (r != COMM_AT_OK || (call.volte_op == &volte_query && call.volte_read > 1u)) call.volte_modem = VOLTE_UNSUPPORTED;
    else call.volte_modem = call.volte_op == &volte_query ? call.volte_read : (uint8_t)(call.volte_op - volte_set);
    call.volte_op = NULL;
  }
  const slm_status_t *st = comm_modem_status();
  if (call.state != CALL_IDLE || call.volte_modem == VOLTE_UNSUPPORTED
      || (st->state != SLM_READY && !(st->state >= SLM_STARTING && st->step > SLM_SIM))) return;
  uint8_t want = call.volte() ? 1u : 0u;
  const comm_side_t *op = call.volte_modem == VOLTE_UNKNOWN ? &volte_query
                        : call.volte_modem != want ? &volte_set[want] : NULL;
  call.volte_read = VOLTE_UNKNOWN;
  if (op && comm_modem_side(op)) call.volte_op = op;
}
/* Новый номер во время ответа обслуживается сразу; следующий ATA ждёт отбоя. */
static void call_finished(void)
{
  // Пустой CLCC не разрешает выполнять ещё не проверенный номер.
  if (call.pending == 1u) call.pending = 0;
  if (call.pending) {
    call.state = CALL_NUMBER; call.stopped = 0;
    comm_channel_call(true); comm_modem_call(true); return;
  }
  call.state = CALL_DONE; comm_channel_call(false);
}
/* Аварийный CHUP уже ушёл. Отпустить данные, до пустого CLCC оградить повторы вызова.
   Фоновая проверка раз в секунду не повторяет CHUP за пределом CALL_LIMIT. */
static void call_stopped(void)
{
  call.stopped = 1; call.answered = call.now; call.state = CALL_CHECK;
  comm_channel_call(false);
}
/* Проверить доступ, ответить и выдержать сроки отбоя. */
void comm_call_poll(uint32_t now_ms, bool access_ready)
{
  call.now = now_ms;
  if (comm_modem_session() != call.session && call.state != CALL_IDLE) {
    comm_modem_side_result(&call_answer); comm_modem_side_result(&call_hangup);
    comm_modem_side_result(&call_check[call.stopped]);
    call.pending = 0; call_finished();
  }
  call_volte();
  // 8 с от первого RING/CLIP: повторы URC не продлевают ожидающий доступ.
  // Срок физического отбоя после ATA по-прежнему отсчитывается от since.
  if (!call.stopped && call.state >= CALL_WAIT_NUMBER && call.state <= CALL_CHECKING &&
      (call.state <= CALL_QUEUED || call.pending == 1u) &&
      now_ms - call.born >= comm_timeouts[COMM_T_CALL_QUIET]) {
    call_stop();
  }
  /* Запрет приложения действует уже на RING, без номера и ожидания флеш. */
  if (call.state != CALL_IDLE && call.state != CALL_DONE && !call.caller(NULL)) {
    call.pending = 0;
    if (call.state <= CALL_QUEUED) call_stop();
  }
  /* Флеш может отложить проверку номера, но уже снятая трубка ждёт только таймер. */
  if (call.pending == 1u && access_ready) {
    if (call.caller(call.number)) call.pending = 2;
  }
  if (!call.stopped && call.state >= CALL_HOLD && call.state <= CALL_CHECKING
      && now_ms - call.since >= comm_timeouts[COMM_T_CALL_LIMIT]) call.state = CALL_ABORT;
  switch (call.state) {
  case CALL_WAIT_NUMBER:
    if (now_ms - call.since >= comm_timeouts[COMM_T_CALL_NUMBER]) call.state = CALL_ANSWER;
    break;
  case CALL_NUMBER:
    if (call.pending == 2u) { call.pending = 0; call.state = CALL_ANSWER; }
    break;
  case CALL_ANSWER:
    if (comm_modem_call_side(&call_answer)) call.state = CALL_QUEUED;
    break;
  case CALL_QUEUED:
    if (!comm_modem_side_started(&call_answer, &call.since)) break;
    call.state = CALL_ANSWERING;
    /* Продолжаем в следующем состоянии без лишнего тика. */
  case CALL_ANSWERING: {
    uint8_t r = comm_modem_side_result(&call_answer);
    if (r == COMM_AT_OK) { call.answered = now_ms; call.state = CALL_HOLD; }
    else if (r == COMM_AT_TIMEOUT) call_stopped(); /* AT уже отправил CHUP перед CMEE */
    else if (r != UINT8_MAX) call.state = CALL_ABORT;
    break;
  }
  case CALL_HOLD:
    if (now_ms - call.answered >= comm_timeouts[COMM_T_CALL_HOLD]) call.state = CALL_HANGUP;
    /* Продолжаем в следующем состоянии без лишнего тика. */
  case CALL_HANGUP:
    if (call.state == CALL_HANGUP && (now_ms - call.answered >= comm_timeouts[COMM_T_AT_SIMPLE])
        && comm_modem_call_side(&call_hangup)) call.state = CALL_HANGING;
    break;
  case CALL_HANGING: {
    uint8_t r = comm_modem_side_result(&call_hangup);
    if (r == COMM_AT_OK) call.state = CALL_CHECK;
    else if (r == COMM_AT_TIMEOUT) call_stopped();
    else if (r != UINT8_MAX) call.state = CALL_ABORT;
    break;
  }
  case CALL_CHECK:
    if (call.stopped && now_ms - call.answered < comm_timeouts[COMM_T_CALL_HOLD]) break;
    call.seen = 0;
    if (call.stopped ? comm_modem_side(&call_check[1]) : comm_modem_call_side(&call_check[0])) call.state = CALL_CHECKING;
    break;
  case CALL_CHECKING: {
    uint8_t r = comm_modem_side_result(&call_check[call.stopped]);
    if (r == UINT8_MAX) break;
    if (r == COMM_AT_OK && !call.seen) call_finished();
    else if (call.stopped || r == COMM_AT_TIMEOUT) call_stopped();
    else { call.answered = now_ms; call.state = CALL_HANGUP; }
    break;
  }
  case CALL_DONE:
    if (now_ms - call.last >= comm_timeouts[COMM_T_CALL_QUIET]) { call.state = CALL_IDLE; call.number[0] = 0; }
    break;
  }
  /* ERROR/NO CARRIER и предел не получают лишний тик главного цикла. */
  if (call.state == CALL_ABORT && comm_modem_call_abort(now_ms)) {
    comm_modem_side_result(&call_answer); comm_modem_side_result(&call_hangup);
    comm_modem_side_result(&call_check[0]); call_stopped();
  }
}
