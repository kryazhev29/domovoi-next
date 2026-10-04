/* Доработка: Кряжев А., 2026. Сигнал, регистрация и тип сети для статусов.
   Раз в 30 с, когда канал свободен: AT+CSQ, AT+CREG?, AT+CEREG?, AT+QNWINFO.
   Значения как у прежней связи: сигнал - число CSQ (0-31, 99 неизвестно),
   регистрация - 1/0, тип сети: 0 неизвестно, 1 GSM/EDGE, 3 3G, 4 LTE. */
#ifndef COMM_INFO_H
#define COMM_INFO_H
#include <stdint.h>
#include <stdbool.h>

typedef struct {
  uint8_t csq, creg, cgreg, net;
} comm_info_t;

void comm_info_poll(uint32_t now_ms);
const comm_info_t *comm_info(void);
/* Событие 38 (сигнал ниже 15): handler(csq) после ближайшего полного опроса, дальше раз в сутки. */
void comm_info_low_signal(void (*handler)(uint8_t csq));

#endif
