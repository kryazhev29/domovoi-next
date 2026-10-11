/* Доработка: Кряжев А., 2026.
   Проверяет входящий номер и отклоняет вызов без ответа.
   Таймер отбоя не зависит от занятости флеш.
   Голос ждёт свободного AT, а настройка VoLTE выполняется вне вызова.
*/
#include "comm_call.h"
#include "comm_modem.h"
#include "comm_channel.h"
#include "comm_timeouts.h"
#include <string.h>
/* DONE ограждает повторные RING/CLIP до 8 с тишины. */
enum { CALL_IDLE, CALL_WAIT_NUMBER, CALL_NUMBER, CALL_HANGUP,
       CALL_HANGING, CALL_ABORT, CALL_DONE };
/* VoLTE модема: неизвестно до запроса, не поддержан — больше не спрашивать в этом запуске. */
enum { VOLTE_UNKNOWN = 0xFF, VOLTE_UNSUPPORTED = 0xFE };
static struct {
  void (*caller)(const char *);
  uint8_t (*volte)(void);
  const comm_side_t *volte_op;
  uint32_t now, since, last;
  char number[21];
  uint8_t state, session, volte_modem, volte_read;
} call;
static const comm_side_t call_hangup = {"ATH", NULL, NULL, 0, COMM_AT_CALL, COMM_T_CALL_HANGUP};
/* Ответ настройки +SETVOLTE: <0|1>, руководство MeiG, раздел 11.11. */
static void volte_line(const uint8_t *p, uint16_t n, uint16_t detail)
{
  uint16_t pos = sizeof("+SETVOLTE:") - 1u;
  uint32_t v;
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
  call.state = CALL_WAIT_NUMBER;
  call.since = call.now;
  call.number[0] = 0;
  comm_channel_call(true);
  comm_modem_call(true);
}
static void call_urc(const uint8_t *p, uint16_t n)
{
  char number[sizeof(call.number)];
  if (call.state == CALL_DONE && call.now - call.last >= comm_timeouts[COMM_T_CALL_QUIET]) {
    call.state = CALL_IDLE;
  }
  if (comm_at_equal(p, n, "RING")) {
    if (call.state == CALL_IDLE) call_begin();
    call.last = call.now;
    return;
  }
  if (!comm_at_prefix(p, n, "+CLIP:") || !call_number(p, n, number)) return;
  call.last = call.now;
  if (call.state == CALL_DONE && call.number[0] && strcmp(number, call.number)) call_begin();
  if (call.state == CALL_IDLE) call_begin();
  if (call.state != CALL_WAIT_NUMBER) return;
  memcpy(call.number, number, strlen(number) + 1u);
  call.state = CALL_NUMBER;
}
/* Назначить обработчик доступа и настройку VoLTE. */
void comm_call_init(void (*caller)(const char *), uint8_t (*volte)(void))
{
  memset(&call, 0, sizeof(call));
  call.caller = caller;
  call.volte = volte;
  call.session = (uint8_t)(comm_modem_session() - 1u);
  comm_modem_listen(COMM_URC_CALL, call_urc);
}
/* Сообщить, продолжается ли вызов или его тихое завершение. */
bool comm_call_active(void) { return call.state != CALL_IDLE; }
/* Настройка VoLTE: в каждом запуске модема после SIM, вне вызова; меняется только при расхождении. */
static void call_volte(void)
{
  uint8_t session = comm_modem_session();
  if (session != call.session) {
    call.session = session;
    call.volte_modem = VOLTE_UNKNOWN;
  }
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
static void call_finished(void)
{
  call.state = CALL_DONE;
  comm_channel_call(false);
}
void comm_call_poll(uint32_t now_ms, bool access_ready)
{
  call.now = now_ms;
  if (comm_modem_session() != call.session && call.state != CALL_IDLE) {
    comm_modem_side_result(&call_hangup);
    call_finished();
  }
  call_volte();
  switch (call.state) {
  case CALL_WAIT_NUMBER:
    if (now_ms - call.since < comm_timeouts[COMM_T_CALL_NUMBER]) break;
    call.state = CALL_HANGUP;
    break;
  case CALL_NUMBER:
    if (!access_ready) break;
    call.caller(call.number);
    call.state = CALL_HANGUP;
    break;
  case CALL_HANGING: {
    uint8_t r = comm_modem_side_result(&call_hangup);
    /* При таймауте AT уже отправил CHUP перед CMEE. */
    if (r == COMM_AT_OK || r == COMM_AT_TIMEOUT) call_finished();
    else if (r != UINT8_MAX) call.state = CALL_ABORT;
    break;
  }
  case CALL_DONE:
    if (now_ms - call.last >= comm_timeouts[COMM_T_CALL_QUIET]) call.state = CALL_IDLE;
    break;
  }
  /* Проверка номера и постановка отбоя проходят в одном такте. */
  if (call.state == CALL_HANGUP && comm_modem_call_side(&call_hangup)) call.state = CALL_HANGING;
  if (call.state == CALL_ABORT && comm_modem_call_abort(now_ms)) {
    comm_modem_side_result(&call_hangup);
    call_finished();
  }
}
