// USB-журнал монтажника: без рабочей памяти и ожидания передачи.
#ifndef DOMOVOI_USB_LOG_H
#define DOMOVOI_USB_LOG_H
#include "includes.h"
extern device_s device;
#define usb_log_on(bit) (device.logBits & (bit))
void usb_log(uint8_t kind, const uint8_t *body, uint8_t count);
void usb_log_value(uint8_t kind, uint8_t code, uint32_t value);
void usb_log_session(void);
void usb_log_enable(uint8_t bits);
#endif
