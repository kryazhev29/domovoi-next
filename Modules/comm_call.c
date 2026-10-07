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
/* Действие и ответ независимы от очереди TCP; DONE ограждает повторы 8 с. */
enum { CALL_IDLE, CALL_WAIT_NUMBER, CALL_NUMBER, CALL_ANSWER, CALL_QUEUED,
       CALL_ANSWERING, CALL_HOLD, CALL_HANGUP, CALL_HANGING, CALL_ABORT, CALL_DONE };
/* VoLTE модема: неизвестно до запроса, не поддержан - больше не спрашивать в этом запуске. */
enum { VOLTE_UNKNOWN = 0xFF, VOLTE_UNSUPPORTED = 0xFE };
static struct {
  bool (*caller)(const char *);
  uint8_t (*volte)(void);
  const comm_side_t *volte_op;
  uint32_t now, since, last, answered;
  char number[21];
  uint8_t state, session, volte_modem, volte_read, pending;
} call;
static const comm_side_t call_answer = {"ATA", NULL, NULL, 0, COMM_AT_CALL, COMM_T_CALL_ANSWER};
static const comm_side_t call_hangup = {"ATH", NULL, NULL, 0, COMM_AT_CALL, COMM_T_CALL_ANSWER};
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
  call.state = CALL_WAIT_NUMBER; call.since = call.now; call.pending = 0;
  call.number[0] = 0; comm_channel_call(true); comm_modem_call(true);
}
/* Принять звонок, номер или потерю голосового соединения. */
static void call_urc(const uint8_t *p, uint16_t n)
{
  char number[sizeof(call.number)];
  if (comm_at_equal(p,n,"NO CARRIER")) {
    if (call.state >= CALL_QUEUED && call.state <= CALL_HANGING) call.state = CALL_ABORT;
    return;
  }
  if (call.state == CALL_DONE && call.now - call.last >= comm_timeouts[COMM_T_CALL_QUIET]) {
    call.state = CALL_IDLE; call.number[0] = 0;
  }
  if (comm_at_equal(p, n, "RING")) {
    if (call.state == CALL_IDLE) call_begin();
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
      || (st->state != SLM_READY && !(st->state == SLM_STARTING && st->step > SLM_SIM))) return;
  uint8_t want = call.volte() ? 1u : 0u;
  const comm_side_t *op = call.volte_modem == VOLTE_UNKNOWN ? &volte_query
                        : call.volte_modem != want ? &volte_set[want] : NULL;
  call.volte_read = VOLTE_UNKNOWN;
  if (op && comm_modem_side(op)) call.volte_op = op;
}
/* Новый номер во время ответа обслуживается сразу; следующий ATA ждёт отбоя. */
static void call_finished(void)
{
  if (call.pending) { call.state = CALL_NUMBER; return; }
  call.state = CALL_DONE; comm_channel_call(false);
}
/* Проверить доступ, ответить и выдержать сроки отбоя. */
void comm_call_poll(uint32_t now_ms, bool access_ready)
{
  call.now = now_ms;
  if (comm_modem_session() != call.session && call.state != CALL_IDLE) {
    comm_modem_side_result(&call_answer); comm_modem_side_result(&call_hangup);
    call.pending = 0; call_finished();
  }
  call_volte();
  /* Флеш может отложить проверку номера, но уже снятая трубка ждёт только таймер. */
  if (call.pending == 1u && access_ready) {
    if (call.caller(call.number)) call.pending = 2;
  }
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
    else if (r == COMM_AT_TIMEOUT) call_finished(); /* AT уже отправил CHUP перед CMEE */
    else if (r != UINT8_MAX) call.state = CALL_ABORT;
    break;
  }
  case CALL_HOLD:
    if (now_ms - call.answered >= comm_timeouts[COMM_T_CALL_HOLD]) call.state = CALL_HANGUP;
    /* Продолжаем в следующем состоянии без лишнего тика. */
  case CALL_HANGUP:
    if (now_ms - call.since >= comm_timeouts[COMM_T_CALL_LIMIT]) call.state = CALL_ABORT;
    else if (call.state == CALL_HANGUP && comm_modem_call_side(&call_hangup)) call.state = CALL_HANGING;
    break;
  case CALL_HANGING: {
    uint8_t r = comm_modem_side_result(&call_hangup);
    if (r == COMM_AT_OK || r == COMM_AT_TIMEOUT) call_finished();
    else if (r != UINT8_MAX || now_ms - call.since >= comm_timeouts[COMM_T_CALL_LIMIT]) call.state = CALL_ABORT;
    break;
  }
  case CALL_DONE:
    if (now_ms - call.last >= comm_timeouts[COMM_T_CALL_QUIET]) { call.state = CALL_IDLE; call.number[0] = 0; }
    break;
  }
  /* ERROR/NO CARRIER и предел не получают лишний тик главного цикла. */
  if (call.state == CALL_ABORT && comm_modem_call_abort(now_ms)) {
    comm_modem_side_result(&call_answer); comm_modem_side_result(&call_hangup); call_finished();
  }
}
