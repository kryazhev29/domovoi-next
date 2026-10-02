/* Доработка: Кряжев А., 2026. Общее ядро и данные профиля модема. */
#ifndef COMM_MODEM_H
#define COMM_MODEM_H
#include "modem_slm336.h"
enum { MODEM_OK, MODEM_MODEL, MODEM_SIM, MODEM_PORT, MODEM_RAT,
       MODEM_NETWORK, MODEM_IMSI, MODEM_PDP, MODEM_READY };
/* Фаза в старшей тетраде, правило ответа/ошибки — в младшей.
   READY повторяет AT, NETWORK опрашивает сеть, остальные ошибки завершают
   попытку; PDP/TCP/SIM уточняют причину. next/alternate — успешные переходы. */
typedef struct {
  uint8_t command;
  uint8_t timeout, next, alternate, action;
} comm_modem_step_t;
typedef struct {
  const char *commands;
  const comm_modem_step_t *steps;
  const char *model, *recv, *closed, *pdp, *open, *port, *port_ready, *rat;
  const char *network[2], *network_reply[2];
  uint8_t type, read_mode, tcp, read;
} comm_modem_profile_t;
/* selected — неизменная таблица во флеш на всё время сеанса.
   Порядок шагов, фазы, следующий/альтернативный шаг задаёт профиль. */
bool comm_modem_start(const comm_modem_profile_t *profile, const slm_settings_t *settings,
                      const slm_handlers_t *handlers, uint32_t now_ms);
const comm_modem_profile_t *comm_modem_profile(void);
const slm_settings_t *comm_modem_settings(void);
/* Команда по шаблону профиля; возвращает длину, >= size — не поместилась. */
uint16_t comm_modem_build(char *cmd, uint16_t size, const char *tpl, const char *host, uint16_t value);
#endif
