/* Доработка: Кряжев А., 2026. Прежние SLDN/обработчики, новое владение транспортом. */
#ifdef COMM_NEW_CORE
#include "user_fun.h"
#include "user_comm.h"
#include <stddef.h>
#include "../Modules/comm_server.h"
void server_rx_process(void);
static void user_comm_registration(void) { user_send_registrationData(CH_SERVER); }
static void user_comm_status(void) { user_send_statusPack(CH_SERVER); }
static bool user_comm_no_traffic(void)
{
  /* Таблица заказчика: событие 13 «Нет трафика», аргументы нулевые. */
  return log_addSystemEvent((logSystem_e)13);
}
bool user_comm_start(const slm_settings_t *settings, comm_power_gpio_t gpio, void (*type_found)(uint8_t), uint32_t now_ms)
{
  static const comm_server_handlers_t handlers = {
    user_comm_registration, user_comm_status, user_comm_session,
    server_rx_process, user_comm_no_traffic
  };
  slm_handlers_t modem_handlers = {gpio, user_comm_rx, user_comm_rx_done, type_found};
  if (!modem_slm336_start(settings, &modem_handlers, now_ms)) return false;
  comm_server_init(&handlers);
  return true;
}
void user_comm_poll(uint32_t now_ms)
{
  modem_slm336_rx_capacity(user_comm_rx_capacity());
  comm_server_poll(now_ms);
}
#endif
