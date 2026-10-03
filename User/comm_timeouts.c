/* Доработка: Кряжев А., 2026. Сроки только реализованных шагов связи. */
#include "comm_timeouts.h"
/* Мс. Питание v5: разряд 2 с, 100/900/2000/2000; одна ступень за poll.
   AT 300, resync 2000, SIM 5000, сеть 60000; QIACT/QIOPEN 150000.
   Сервер: 5 с, статус 15 с, unacked 10 с; паузы 30/60/120/300 с.
   Ping два по 3 с, DNS 15 с; поздний DNS ограждён на 60 с. */
const uint32_t comm_timeouts[COMM_T_COUNT] = {
  2000, /* COMM_T_DISCHARGE_V5 */
  100, /* COMM_T_POWER_LEAD_V5 */
  900, /* COMM_T_POWER_RAIL_V5 */
  2000, /* COMM_T_POWER_KEY_V5 */
  2000, /* COMM_T_POWER_BOOT_V5 */
  2000, /* COMM_T_AT_RESYNC */
  300, /* COMM_T_AT_SIMPLE */
  10000, /* COMM_T_AT_READY */
  100, /* COMM_T_AT_RETRY */
  5000, /* COMM_T_SIM */
  60000, /* COMM_T_NETWORK */
  1000, /* COMM_T_NETWORK_POLL */
  150000, /* COMM_T_QIACT */
  40000, /* COMM_T_QIDEACT: Quectel до 40 с */
  150000, /* COMM_T_QIOPEN */
  1000, /* COMM_T_QIRD */
  5000, /* COMM_T_SERVER_REPLY */
  15000, /* COMM_T_STATUS */
  10000, /* COMM_T_UNACKED */
  30000, /* COMM_T_SERVER_RETRY_1 */
  60000, /* COMM_T_SERVER_RETRY_2 */
  120000, /* COMM_T_SERVER_RETRY_3 */
  300000, /* COMM_T_SERVER_RETRY_MAX */
  600000, /* COMM_T_STABLE */
  1000, /* COMM_T_SEND_PROMPT */
  3000, /* COMM_T_SEND_RESULT */
  10000, /* COMM_T_TCP_CLOSE */
  6000, /* COMM_T_PING */
  15000, /* COMM_T_DNS */
  1000, /* COMM_T_ACK_POLL */
  60000, /* COMM_T_DNS_FENCE */
  /* Восстановление (этап 0, Q4/Q6): модем без питания 10/30/60 мин, без снятия
     питания 10/30/60 с, дальше по последней ступени; окна раннего перезапуска
     10 мин (3 таймаута AT) и 1 ч (5 неудач TCP); сутки работы без часов. */
  10000, /* COMM_T_SOFT_1 */
  30000, /* COMM_T_SOFT_2 */
  60000, /* COMM_T_SOFT_3 */
  600000, /* COMM_T_POWER_1 */
  1800000, /* COMM_T_POWER_2 */
  3600000, /* COMM_T_POWER_3 */
  600000, /* COMM_T_EARLY_AT */
  3600000, /* COMM_T_EARLY_TCP */
  86400000, /* COMM_T_DAY */
  /* Звонок без ответа: номер после RING до 1 с; сброс AT+CHUP до 5 с;
     вызов окончен, если 8 с нет RING/CLIP (RING идёт раз в ~5 с). */
  1000, /* COMM_T_CALL_NUMBER */
  5000, /* COMM_T_CALL_HANGUP */
  8000, /* COMM_T_CALL_QUIET */
};
