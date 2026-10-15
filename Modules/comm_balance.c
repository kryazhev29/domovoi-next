/* Доработка: Кряжев А., 2026.
   Запрашивает баланс SIM через USSD и разбирает ответ оператора.
   Запрос идёт раз в сутки, отказ повторяется через заданную паузу.
   Меню оператора закрывается после получения результата.
*/
#include "comm_balance.h"
#include "comm_modem.h"
#include "comm_timeouts.h"
#include <string.h>
/* ASK - команда у модема, WAIT - ждём +CUSD (обычно после OK), CANCEL - закрыть меню. */
enum { BAL_IDLE, BAL_ASK, BAL_WAIT, BAL_CANCEL };
static const comm_side_t bal_cancel = {"AT+CUSD=2", NULL, NULL, 0, COMM_AT_TEXT, COMM_T_AT_SIMPLE};
static struct {
  const char *(*code)(void);
  void (*result)(int32_t);
  uint32_t now, since, next;
  comm_side_t req;
  char cmd[24];
  char text[64];
  uint8_t state, got, menu;
} bal;
/* +CUSD: <m>[,"<str>"[,<dcs>]] (27.007 §7.15); dcs 72 - UCS2 hex, иначе текст.
   Строка может быть обрезана движком AT (длинная реклама): без закрывающей кавычки. */
static void bal_answer(const uint8_t *p, uint16_t n)
{
  uint16_t pos = sizeof("+CUSD:") - 1u, start, end, len = 0;
  uint32_t m, dcs = 15;
  if ((bal.state != BAL_ASK && bal.state != BAL_WAIT) || !comm_at_prefix(p, n, "+CUSD:")
      || !comm_at_scan(p, n, &pos, 5u, &m)) return;
  bal.got = 1; bal.menu = m == 1; bal.text[0] = 0;
  if (m > 1 || pos + 1u >= n || p[pos] != ',' || p[pos + 1u] != '"') return; /* 2/4/5 - отказ сети */
  start = end = (uint16_t)(pos + 2u);
  while (end < n && p[end] != '"') ++end;
  if (end + 1u < n && p[end + 1u] == ',') { pos = (uint16_t)(end + 2u); if (!comm_at_scan(p, n, &pos, 255u, &dcs)) dcs = 15; }
  if (end == n) { /* обрезана: кодировка не дошла - UCS2, если только hex четвёрками */
    uint16_t i = start;
    while (i < end && comm_at_hex(p[i]) < 16u) ++i;
    if (i == end && end - start >= 4u) dcs = 72;
  }
  if (dcs == 72u) {
    for (uint16_t i = start; i + 3u < end && len + 1u < sizeof(bal.text); i = (uint16_t)(i + 4u)) {
      uint16_t u = 0;
      for (uint8_t k = 0; k < 4u; ++k) { uint8_t v = comm_at_hex(p[i + k]); if (v > 15u) { bal.text[len] = 0; return; } u = (uint16_t)(u << 4 | v); }
      bal.text[len++] = comm_at_cp1251(u);
    }
  } else for (uint16_t i = start; i < end && len + 1u < sizeof(bal.text); ++i) bal.text[len++] = (char)p[i];
  bal.text[len] = 0;
}
/* Передать строку ответа текущего запроса разборщику USSD. */
static void bal_line(const uint8_t *p, uint16_t n, uint16_t detail) { (void)detail; bal_answer(p, n); }
/* Сумма после "баланс"/"balance" (иначе первое число): "-12,30", "123.45 р". */
bool comm_balance_parse(const char *t, int32_t *rubles)
{
  static const char *const words[] = {"\xE0\xEB\xE0\xED\xF1", "alance"}; /* [Бб]аланс, [Bb]alance */
  const char *s = NULL;
  for (uint8_t w = 0; w < 2u && !s; ++w) {
    for (const char *f = t; *f; ++f) {
      uint8_t i = 0;
      while (words[w][i] && f[i] == words[w][i]) ++i;
      if (!words[w][i]) { s = f; break; }
    }
  }
  if (!s) s = t;
  while (*s && (*s < '0' || *s > '9')) ++s;
  if (!*s) return false;
  bool minus = s > t && s[-1] == '-';
  int32_t v = 0;
  while (*s >= '0' && *s <= '9') { if (v < 1000000) v = v * 10 + (*s - '0'); ++s; }
  *rubles = minus ? -v : v;
  return true;
}
/* Выдать сумму и назначить следующий срок запроса. */
static void bal_finish(bool ok)
{
  int32_t v;
  bal.state = BAL_IDLE;
  if (ok && comm_balance_parse(bal.text, &v)) {
    bal.next = bal.now + comm_timeouts[COMM_T_DAY];
    bal.result(v);
  } else bal.next = bal.now + comm_timeouts[COMM_T_USSD_RETRY];
  if (bal.menu && comm_modem_side(&bal_cancel)) bal.state = BAL_CANCEL; /* меню оператора - закрыть */
}
/* Подключить настройки, результат и уведомления USSD. */
void comm_balance_init(const char *(*code)(void), void (*result)(int32_t))
{
  memset(&bal, 0, sizeof(bal));
  bal.code = code; bal.result = result;
  bal.req.mode = COMM_AT_TEXT; bal.req.timeout = COMM_T_USSD; bal.req.line = bal_line; bal.req.cmd = bal.cmd;
  comm_modem_listen(COMM_URC_BALANCE, bal_answer);
}
/* Продвинуть запрос баланса, ожидание ответа или закрытие меню. */
void comm_balance_poll(uint32_t now_ms)
{
  uint8_t r;
  bal.now = now_ms;
  switch (bal.state) {
  case BAL_IDLE: {
    if (!comm_modem_ready() || (int32_t)(now_ms - bal.next) < 0) return;
    const char *c = bal.code();
    if (!*c) return;
    uint8_t i = 0;
    while (c[i] && i < 8u && ((c[i] >= '0' && c[i] <= '9') || c[i] == '*' || c[i] == '#')) ++i;
    if (c[i]) { bal.next = now_ms + comm_timeouts[COMM_T_DAY]; return; } /* не USSD - не спрашиваем */
    uint16_t k = comm_at_copy(bal.cmd, sizeof(bal.cmd), 0, "AT+CUSD=1,\"");
    k = comm_at_copy(bal.cmd, sizeof(bal.cmd), k, c);
    comm_at_copy(bal.cmd, sizeof(bal.cmd), k, "\",15");
    bal.got = bal.menu = 0;
    if (comm_modem_side(&bal.req)) { bal.state = BAL_ASK; bal.since = now_ms; }
    break;
  }
  case BAL_ASK:
    if ((r = comm_modem_side_result(&bal.req)) == UINT8_MAX) break;
    if (r != COMM_AT_OK) bal_finish(false);
    else bal.state = BAL_WAIT;
    break;
  case BAL_WAIT:
    if (bal.got) bal_finish(true);
    else if (now_ms - bal.since >= comm_timeouts[COMM_T_USSD]) bal_finish(false);
    break;
  case BAL_CANCEL:
    if (comm_modem_side_result(&bal_cancel) != UINT8_MAX) bal.state = BAL_IDLE;
    break;
  }
}
