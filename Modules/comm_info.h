/* Доработка: Кряжев А., 2026.
   Опрашивает сигнал, регистрацию и технологию сети.
   Опрос имеет низкий приоритет и не мешает данным сервера.
   Без связи прошлые значения очищаются, слабый сигнал отмечается раз в сутки.
*/
/* Доработка: Кряжев А., 2026. Сигнал, регистрация и тип сети для статусов.
   Раз в 30 с, когда канал свободен: AT+CSQ, AT+CREG?, AT+CGREG?, AT+CEREG?, AT+QNWINFO, AT+QENG.
   Значения как у прежней связи: сигнал - число CSQ (0-31, 99 неизвестно),
   регистрация - 1/0, тип сети: 0 неизвестно, 1 GSM/EDGE, 3 3G, 4 LTE. */
#ifndef COMM_INFO_H
#define COMM_INFO_H
#include <stdint.h>
#include <stdbool.h>

typedef struct {
  uint8_t csq, creg, cgreg, net;
  char operator_code[7];
  uint8_t rsrp, rsrq, sinr; /* Модули отрицательных RSRP/RSRQ, сырой SINR 0..250; 255 неизвестно. */
} comm_info_t;

/* Поставить очередной фоновый запрос сведений. */
void comm_info_poll(uint32_t now_ms);
/* Выдать текущий снимок сигнала и регистрации. */
const comm_info_t *comm_info(void);
/* Событие 38 (сигнал ниже 15): handler(csq) после ближайшего полного опроса, дальше раз в сутки. */
void comm_info_low_signal(void (*handler)(uint8_t csq));

#endif
