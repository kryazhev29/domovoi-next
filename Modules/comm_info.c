/* Доработка: Кряжев А., 2026.
   Опрашивает сигнал, регистрацию и технологию сети.
   Опрос имеет низкий приоритет и не мешает данным сервера.
   Без связи прошлые значения очищаются, слабый сигнал отмечается раз в сутки.
*/
#include "comm_info.h"
#include "comm_channel.h"
#include "comm_at.h"
#include "modem_slm336.h"
#include "comm_timeouts.h"
#include <string.h>

#define INFO_PERIOD_MS 30000u

/* CREG - голосовая сеть (звонки, SMS); CGREG/CEREG - данные 2G-3G/LTE: зарегистрирован в любой - cgreg. */
static const char *const info_cmd[] = {"AT+CSQ", "AT+CREG?", "AT+CGREG?", "AT+CEREG?", "AT+QNWINFO"};
static struct {
  comm_info_t v;
  uint32_t next, day; /* day - срок суточной проверки сигнала */
  uint8_t step, ps, daily; /* ps: бит 0 - CGREG, бит 1 - CEREG; daily - срок day назначен */
} info = {{99, 0, 0, 0, {0}}, 0, 0, 0, 0, 0};
static void (*info_low)(uint8_t);
/* Подключить запись события слабого сигнала. */
void comm_info_low_signal(void (*handler)(uint8_t csq)) { info_low = handler; info.daily = 0; }

/* Есть ли в строке подстрока (короткие строки ответа, без кучи). */
static bool info_has(const uint8_t *p, uint16_t n, const char *s)
{
  uint16_t k = (uint16_t)strlen(s);
  for (uint16_t i = 0; i + k <= n; i++)
    if (!memcmp(p + i, s, k)) return true;
  return false;
}

/* Строка ответа текущего запроса; регистрация - статус 1 (дом) или 5 (роуминг). */
static void info_line(const uint8_t *p, uint16_t n)
{
  uint32_t v[2];
  if (comm_at_parse(p, n, "+CSQ: \001,\001", v)) info.v.csq = v[0] <= 99u ? (uint8_t)v[0] : 99u;
  else if (comm_at_parse(p, n, "+CREG: \001,\001", v)) info.v.creg = v[1] == 1u || v[1] == 5u;
  else if (comm_at_parse(p, n, "+CGREG: \001,\001", v) || comm_at_parse(p, n, "+CEREG: \001,\001", v)) {
    uint8_t bit = p[2] == 'G' ? 1u : 2u;
    info.ps = (uint8_t)((v[1] == 1u || v[1] == 5u) ? info.ps | bit : info.ps & ~bit);
    info.v.cgreg = info.ps != 0;
  }
  else if (comm_at_prefix(p, n, "+QNWINFO:")) {
    memset(info.v.operator_code, 0, sizeof(info.v.operator_code));
    const uint8_t *field = memchr(p, ',', n);
    if (field) {
      uint16_t pos = comm_at_spaces(p, n, (uint16_t)(field - p + 1u));
      uint8_t digits = 0;
      if (pos < n && p[pos++] == '"') {
        while (pos < n && digits < 6u && p[pos] >= '0' && p[pos] <= '9')
          info.v.operator_code[digits++] = (char)p[pos++];
        if (pos >= n || p[pos] != '"' || digits < 5u) memset(info.v.operator_code, 0, sizeof(info.v.operator_code));
      }
    }
    /* Первое поле - технология: "LTE"/"FDD LTE"/"TDD LTE", "GSM"/"EDGE"/"GPRS", "WCDMA"/"HSPA"... */
    info.v.net = info_has(p, n, "LTE") ? 4u
      : info_has(p, n, "GSM") || info_has(p, n, "EDGE") || info_has(p, n, "GPRS") ? 1u
      : info_has(p, n, "No Service") || info_has(p, n, "NONE") ? 0u : 3u;
  }
}

/* Поставить очередной фоновый запрос сведений. */
void comm_info_poll(uint32_t now_ms)
{
  /* Модем не на связи - цифры прошлого сеанса не показываем, опрос заново с начала. */
  if (!comm_modem_ready()) {
    info.v.csq = 99; info.v.creg = info.v.cgreg = info.v.net = info.ps = 0;
    memset(info.v.operator_code, 0, sizeof(info.v.operator_code));
    info.step = 0; info.next = now_ms; return;
  }
  if ((int32_t)(now_ms - info.next) < 0) return;
  if (!comm_channel_query(info_cmd[info.step], info_line)) return;
  info.step = (uint8_t)((info.step + 1u) % (sizeof(info_cmd) / sizeof(info_cmd[0])));
  if (info.step) return;
  info.next = now_ms + INFO_PERIOD_MS;
  /* Событие 38: сигнал ниже 15 - после первого полного опроса, дальше раз в сутки. */
  if (!info.daily || (int32_t)(now_ms - info.day) >= 0) {
    info.daily = 1; info.day = now_ms + comm_timeouts[COMM_T_DAY];
    if (info.v.csq < 15u) info_low(info.v.csq);
  }
}

/* Выдать текущий снимок сигнала и регистрации. */
const comm_info_t *comm_info(void) { return &info.v; }
