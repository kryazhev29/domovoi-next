/* Доработка: Кряжев А., 2026. Стык нового канала; main подключается после этапа 6. */
#ifndef USER_COMM_H
#define USER_COMM_H
#include "../Modules/modem_slm336.h"
bool user_comm_start(const slm_settings_t *settings, const slm_handlers_t *handlers, uint32_t now_ms);
void user_comm_boot(void);
void user_comm_poll(uint32_t now_ms);
uint16_t user_comm_rx_capacity(void);
uint16_t user_comm_rx(const uint8_t *data, uint16_t count);
void user_comm_rx_done(uint16_t count, uint8_t reason);
void user_comm_session(bool open);
/* Для восстановления связи (user_fun.c): местное время, с (UINT32_MAX - часы не
   установлены) и занятость обменом, который нельзя рвать перезапуском модема. */
uint32_t user_comm_local_time(void);
/* SMS на номер; false - не принято к отправке. */
bool user_comm_sms(const char *number, const char *text);
bool user_comm_busy(void);
const char *user_comm_ussd(void);
uint8_t user_comm_volte(void);
/* Баланс в рублях (минус - долг): статус 2 и событие 37. */
void user_comm_balance(int32_t rubles);
/* Журнал связи: события 11, 12, 14-19 (comm_server.h), 38 - сигнал ниже 15. */
bool user_comm_link(uint8_t kind, uint8_t address);
void user_comm_low_signal(uint8_t csq);
/* Номер сервера по адресу канала: 0 - сервер 1, 1 - сервер 2. */
uint8_t user_comm_server(uint8_t address);
#endif
