/* Доработка: Кряжев А., 2026. Сигнал, регистрация и тип сети. Устройство - в comm_info.h. */
#include "comm_info.h"
#include "comm_channel.h"
#include "comm_at.h"
#include "modem_slm336.h"
#include <string.h>

#define INFO_PERIOD_MS 30000u

/* CREG - голосовая сеть (звонки, SMS); CGREG/CEREG - данные 2G-3G/LTE: зарегистрирован в любой - cgreg. */
static const char *const info_cmd[] = {"AT+CSQ", "AT+CREG?", "AT+CGREG?", "AT+CEREG?", "AT+QNWINFO"};
static struct {
  comm_info_t v;
  uint32_t next;
  uint8_t step, ps; /* ps: бит 0 - CGREG, бит 1 - CEREG */
} info = {{99, 0, 0, 0}, 0, 0, 0};

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
    /* Первое поле - технология: "LTE"/"FDD LTE"/"TDD LTE", "GSM"/"EDGE"/"GPRS", "WCDMA"/"HSPA"... */
    info.v.net = info_has(p, n, "LTE") ? 4u
      : info_has(p, n, "GSM") || info_has(p, n, "EDGE") || info_has(p, n, "GPRS") ? 1u
      : info_has(p, n, "No Service") || info_has(p, n, "NONE") ? 0u : 3u;
  }
}

void comm_info_poll(uint32_t now_ms)
{
  /* Модем не на связи - цифры прошлого сеанса не показываем, опрос заново с начала. */
  if (!modem_slm336_ready()) {
    info.v.csq = 99; info.v.creg = info.v.cgreg = info.v.net = info.ps = 0;
    info.step = 0; info.next = now_ms; return;
  }
  if ((int32_t)(now_ms - info.next) < 0) return;
  if (!comm_channel_query(info_cmd[info.step], info_line)) return;
  info.step = (uint8_t)((info.step + 1u) % (sizeof(info_cmd) / sizeof(info_cmd[0])));
  if (!info.step) info.next = now_ms + INFO_PERIOD_MS;
}

const comm_info_t *comm_info(void) { return &info.v; }
