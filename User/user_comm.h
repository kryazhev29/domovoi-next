/* Доработка: Кряжев А., 2026. Стык нового канала; main подключается после этапа 6. */
#ifndef USER_COMM_H
#define USER_COMM_H
#include "../Modules/modem_slm336.h"
#ifdef COMM_NEW_CORE
bool user_comm_start(const slm_settings_t *settings, comm_power_gpio_t gpio, void (*type_found)(uint8_t), uint32_t now_ms);
void user_comm_poll(uint32_t now_ms);
uint16_t user_comm_rx_capacity(void);
uint16_t user_comm_rx(const uint8_t *data, uint16_t count);
void user_comm_rx_done(uint16_t count, uint8_t reason);
void user_comm_session(bool open);
#endif
#endif
