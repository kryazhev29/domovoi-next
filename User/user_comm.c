/* Доработка: Кряжев А., 2026. Прежние SLDN/обработчики, новое владение транспортом. */
#include "user_fun.h"
#include "user_comm.h"
#include <stddef.h>
#include <string.h>
#include "../Modules/comm_server.h"
#include "../Modules/comm_channel.h"
#include "../Modules/comm_recover.h"
#include "../Modules/comm_info.h"
#include "../Modules/comm_call.h"
#include "../Modules/comm_sms.h"
void server_rx_process(void);
static void user_comm_registration(void) { user_send_registrationData(CH_SERVER); }
static void user_comm_status(void) { user_send_statusPack(CH_SERVER); }
static bool user_comm_no_traffic(void)
{
  /* Таблица заказчика: событие 13 «Нет трафика», аргументы нулевые. */
  return log_addSystemEvent((logSystem_e)13);
}
static const comm_server_handlers_t handlers = {
  user_comm_registration, user_comm_status, user_comm_session,
  server_rx_process, user_comm_no_traffic
};
/* После перезапуска модема сервер и канал - с начала. */
static void user_comm_reset_session(void)
{
  comm_server_init(&handlers);
  comm_channel_receive(user_comm_rx, user_comm_rx_done);
}
/* Номер из CLIP - прежней проверке доступа; строка вызова только для чтения. */
static void user_comm_call(const char *number)
{
  char num[24]; /* номер CLIP короче 21 */
  strncpy(num, number, sizeof(num) - 1u); num[sizeof(num) - 1u] = 0;
  user_incomingcall(num, CT_PHONE);
}
bool user_comm_start(const slm_settings_t *settings, const slm_handlers_t *handlers, uint32_t now_ms)
{
  static const comm_recover_hooks_t hooks = {
    user_comm_reset_session, user_comm_local_time, user_comm_busy
  };
  if (!modem_slm336_start(settings, handlers, now_ms)) return false;
  user_comm_reset_session();
  comm_server_no_internet(comm_recover_no_internet);
  comm_recover_init(&hooks, now_ms);
  comm_call_init(user_comm_call);
  comm_sms_init(user_smsreceived);
  return true;
}
/* Одна SMS в очереди: занято или неверный номер - false, событие отправки не пишется. */
bool user_comm_sms(const char *number, const char *text) { return comm_sms_send(number, text); }
void user_comm_poll(uint32_t now_ms)
{
  comm_channel_rx_capacity(user_comm_rx_capacity());
  comm_server_poll(now_ms);
  comm_recover_poll(now_ms);
  comm_info_poll(now_ms);
  comm_call_poll(now_ms);
  comm_sms_poll(now_ms);
}
