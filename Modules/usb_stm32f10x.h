
#ifndef _USB_H
#define _USB_H

#include "main.h"

void usb_init(void);
void usb_datareceived (uint8_t* ptr, uint16_t len);
void usb_send (uint8_t* ptr, uint16_t len);
void usb_sendstr (char* str);
void usb_timer1ms (void);
void usb_check_data (void);
uint8_t usb_get_state (void);
void usb_clear_buffer (void);
void usb_data_reading(uint8_t* ptr, uint16_t len);

#endif