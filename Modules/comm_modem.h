/* Доработка: Кряжев А., 2026.
   Запускает единственный SLM336 и распределяет его ответы.
   Шаги питания и настройки заданы постоянной таблицей.
   Один владелец AT; служебные команды ждут свободного слота.
*/
#ifndef COMM_MODEM_H
#define COMM_MODEM_H
#include "modem_slm336.h"
enum { MODEM_OK, MODEM_MODEL, MODEM_SIM, MODEM_PORT, MODEM_RAT,
       MODEM_NETWORK, MODEM_IMSI, MODEM_PDP, MODEM_READY, MODEM_ANY, MODEM_IMEI, MODEM_APN_QUERY };
/* Фаза в старшей тетраде, правило ответа/ошибки — в младшей.
   READY повторяет AT, NETWORK опрашивает сеть, остальные ошибки завершают
   попытку; PDP/TCP/SIM уточняют причину. next/alternate — успешные переходы. */
typedef struct {
  uint8_t timeout, next, alternate, action;
} comm_modem_step_t;
/* Таблица команд и переходов единственного модема платы v5. */
extern const char slm_tcp_command[], slm_read_command[], slm_commands[];
extern const comm_modem_step_t slm_steps[];
/* Перезапуск по сохранённым настройкам: power - со снятием питания, иначе с AT. */
bool comm_modem_restart(bool power, uint32_t now_ms);
/* Выдать назначенные геттеры настроек. */
const slm_settings_t *comm_modem_settings(void);
/* Проверить новый внешний набор настроек до сохранения команды 12. */
bool comm_modem_valid_settings(void);
/* Команда по шаблону профиля; возвращает длину, >= size — не поместилась. */
uint16_t comm_modem_build(char *cmd, uint16_t size, const char *tpl, const char *host, uint16_t value);
#endif
