/* Доработка: Кряжев А., 2026. Входящий вызов без ответа: номер, доступ, сброс. */
#include "comm_call.h"
#include "comm_modem.h"
#include "comm_timeouts.h"
#include <string.h>
/* WAIT_NUMBER - был RING, ждём CLIP; NUMBER - номер есть, проверить;
   DONE - сброшен, повторные RING/CLIP того же вызова не открывают. */
enum { CALL_IDLE, CALL_WAIT_NUMBER, CALL_NUMBER, CALL_HANGUP, CALL_HANGING, CALL_DONE };
static struct {
  void (*caller)(const char *);
  uint32_t now, since, last;
  char number[21];
  uint8_t state;
} call;
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
/* Из разбора AT: только отметки, проверка и AT - в comm_call_poll. */
static void call_urc(const uint8_t *p, uint16_t n)
{
  char number[sizeof(call.number)];
  if (comm_at_equal(p, n, "RING")) {
    call.last = call.now;
    if (call.state == CALL_IDLE) { call.state = CALL_WAIT_NUMBER; call.since = call.now; }
    else if (call.state == CALL_DONE) call.state = CALL_HANGUP; /* ещё звонит: сбросить снова */
    return;
  }
  if (!comm_at_prefix(p, n, "+CLIP:") || !call_number(p, n, number)) return;
  call.last = call.now;
  /* Новый номер - новый вызов; тот же номер в пределах вызова не открывает второй раз. */
  if (call.state != CALL_NUMBER && (call.state <= CALL_WAIT_NUMBER || strcmp(number, call.number))) {
    memcpy(call.number, number, sizeof(number)); call.state = CALL_NUMBER;
  } else if (call.state == CALL_DONE) call.state = CALL_HANGUP;
}
void comm_call_init(void (*caller)(const char *))
{
  memset(&call, 0, sizeof(call));
  call.caller = caller;
  comm_modem_listen(call_urc);
}
bool comm_call_active(void) { return call.state != CALL_IDLE; }
void comm_call_poll(uint32_t now_ms)
{
  call.now = now_ms;
  switch (call.state) {
  case CALL_WAIT_NUMBER: /* номер скрыт или CLIP потерян: сброс без открытия */
    if (now_ms - call.since >= comm_timeouts[COMM_T_CALL_NUMBER]) call.state = CALL_HANGUP;
    break;
  case CALL_NUMBER: /* сначала открыть, сброс - следующим шагом */
    call.state = CALL_HANGUP;
    if (call.caller) call.caller(call.number);
    break;
  case CALL_HANGUP:
    if (comm_modem_side("AT+CHUP", NULL, COMM_T_CALL_HANGUP)) call.state = CALL_HANGING;
    else if (now_ms - call.last >= comm_timeouts[COMM_T_CALL_QUIET]) call.state = CALL_DONE;
    break;
  case CALL_HANGING: /* любой исход: если вызов жив, следующий RING сбросит его снова */
    if (comm_modem_side_result() != UINT8_MAX) call.state = CALL_DONE;
    break;
  case CALL_DONE:
    if (now_ms - call.last >= comm_timeouts[COMM_T_CALL_QUIET]) { call.state = CALL_IDLE; call.number[0] = 0; }
    break;
  }
}
