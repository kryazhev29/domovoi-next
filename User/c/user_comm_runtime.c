/* Доработка: Кряжев А., 2026.
   Соединяет связь с кадрами, журналом и проверкой доступа приложения.
   Главный цикл передаёт каждому модулю время и доступность флеш.
   Перезапуск назначает новый сеанс канала и его обязательные обработчики.
*/
#include "user_fun.h"
#include "user_comm.h"
#include "photo.h"
#include <string.h>
#include "../../Modules/comm_server.h"
#include "../../Modules/comm_channel.h"
#include "../../Modules/comm_recover.h"
#include "../../Modules/comm_info.h"
#include "../../Modules/comm_call.h"
#include "../../Modules/comm_sms.h"
extern device_s device;
/* Обработать уже проверенные кадры сервера вне приёма AT. */
void server_rx_process(void);
/* Начать ограниченный проход приёма кадров сервера. */
void server_rx_begin_pass(void);
/* Собрать прежний кадр регистрации сервера. */
static void user_comm_registration(void) { user_send_registrationData(CH_SERVER); }
/* Собрать прежний кадр состояния устройства. */
static void user_comm_status(void) { user_send_statusPack(CH_SERVER); }
/* Записать событие отсутствия трафика в журнал. */
static bool user_comm_no_traffic(void)
{
  /* Таблица заказчика: событие 13 «Нет трафика», аргументы нулевые. */
  return log_addSystemEvent((logSystem_e)13);
}
static const comm_server_handlers_t server_handlers = {
  user_comm_registration, user_comm_status, user_comm_session,
  server_rx_process, user_comm_no_traffic
};
/* После перезапуска модема сервер и канал - с начала. */
static void user_comm_reset_session(void)
{
  comm_server_init(&server_handlers);
  comm_channel_receive(user_comm_rx, user_comm_rx_done);
}
/* NULL проверяет только допуск голоса; номер из CLIP передаётся проверке доступа. */
static bool user_comm_call(const char *number)
{
  if (device.status.readyToUpdate) return false;
  if (!number) return true;
  return user_incomingcall(number, CT_PHONE);
}
/* Запустить связь и назначить её обязательных владельцев. */
bool user_comm_start(const slm_settings_t *settings, const slm_handlers_t *handlers, uint32_t now_ms)
{
  static const comm_recover_hooks_t hooks = {
    user_comm_reset_session, user_comm_local_time, user_comm_busy
  };
  if (!modem_slm336_start(settings, handlers, now_ms)) return false;
  user_comm_reset_session();
  comm_server_no_internet(comm_recover_no_internet);
  comm_server_link(user_comm_link);
  comm_info_low_signal(user_comm_low_signal);
  comm_recover_init(&hooks, now_ms);
  comm_call_init(user_comm_call, user_comm_volte);
  comm_sms_init(user_smsreceived);
  return true;
}
/* Одна SMS в очереди: занято или неверный номер - false, событие отправки не пишется. */
bool user_comm_sms(const char *number, const char *text) { return comm_sms_send(number, text); }
/* Продвинуть все модули связи с общей отметкой времени. */
void user_comm_poll(uint32_t now_ms)
{
  server_rx_begin_pass();
  comm_channel_rx_capacity(user_comm_rx_capacity());
  comm_server_poll(now_ms);
  comm_recover_poll(now_ms);
  comm_info_poll(now_ms);
  comm_call_poll(now_ms, flash_exec_read_allowed());
  /* Обработчик вызова мог сменить владельца флеша: для SMS нужен свежий снимок. */
  if (flash_exec_read_allowed()) comm_sms_poll(now_ms);
}
