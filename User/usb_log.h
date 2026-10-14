// USB-журнал монтажника: без рабочей памяти и ожидания передачи.
#ifndef DOMOVOI_USB_LOG_H
#define DOMOVOI_USB_LOG_H
#include "includes.h"
extern device_s device;
#define usb_log_on(bit) (device.logBits & (bit))
// Поставить кадр 109 в очередь USB; тело длиннее 45 байт обрезается с пометкой.
void usb_log(uint8_t kind, const uint8_t *body, uint16_t count);
// Кадр с кодом и числом: данные, сервер, восстановление.
void usb_log_value(uint8_t kind, uint8_t code, uint32_t value);
// Новая USB-сессия выключает журнал.
void usb_log_session(void);
// Команда 108: включить категории 1/2/4.
void usb_log_enable(uint8_t bits);
#endif
