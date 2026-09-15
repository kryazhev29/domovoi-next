
#ifndef _USB_H
#define _USB_H

#include "main.h"

extern volatile uint32_t usb_rx_errors;
extern volatile uint32_t usb_tx_errors;
extern volatile uint32_t usb_debug_dropped;
extern volatile uint32_t usb_recovery_count;
extern volatile uint32_t usb_esof_forced;
extern volatile uint32_t usb_esof_timeouts;
extern volatile uint32_t usb_bus_resets;
extern volatile uint32_t usb_recovery_reason;
void usb_request_recovery(void);
void usb_in_done(void);
void usb_sof_seen(void);
void usb_diag_fill(uint32_t *v);
void usb_diag_fill_regs(uint32_t *v);
void usb_init(void);
void usb_datareceived (uint8_t* ptr, uint16_t len);
uint8_t usb_send (uint8_t* ptr, uint16_t len);
void usb_sendstr (char* str);
void usb_timer1ms (void);
void usb_check_data (void);
uint8_t usb_get_state (void);
void usb_clear_buffer (void);
void usb_data_reading(uint8_t* ptr, uint16_t len);

#endif