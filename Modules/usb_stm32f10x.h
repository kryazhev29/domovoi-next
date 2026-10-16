// Доработка: Кряжев А., 2026

#ifndef _USB_H
#define _USB_H

#include "main.h"

// Эпоха USB-сессии обязательна для отмены ответов владельцев флеш.
extern volatile uint32_t usb_bus_resets;
#if DOMOVOI_DIAG
extern volatile uint32_t usb_rx_errors;
extern volatile uint32_t usb_tx_errors;
extern volatile uint32_t usb_debug_dropped;
extern volatile uint32_t usb_recovery_count;
extern volatile uint32_t usb_esof_forced;
extern volatile uint32_t usb_esof_timeouts;
extern volatile uint32_t usb_recovery_reason;
#endif
// Отсоединить USB на 20 мс для восстановления.
void usb_request_recovery(void);
// Отметить принятие пакета IN хостом.
void usb_in_done(void);
// Отметить свежий SOF для диагностики.
void usb_sof_seen(void);
// Учесть ошибку шины в стендовой диагностике.
void usb_bus_error_seen(void);
#if DOMOVOI_DIAG
// Снять счётчики команды 111.
void usb_diag_fill(uint32_t *v);
// Снять текущие регистры и прежний снимок.
void usb_diag_fill_regs(uint32_t *v);
// Снять текущие и сохранённые ошибки шины.
void usb_diag_fill_errors(uint32_t *v);
// Снять текущие и сохранённые адрес/EP0.
void usb_diag_fill_addr(uint32_t *v);
#endif
// Настроить USB и подключить подтяжку.
void usb_init(void);
// Принять внешние байты OUT в защищённый буфер.
void usb_datareceived (uint8_t* ptr, uint16_t len);
// Принять двоичный ответ в очередь USB.
uint8_t usb_send (uint8_t* ptr, uint16_t len);
// Обслужить тишину кадра и таймауты USB.
void usb_timer1ms (void);
// Проверить один целый кадр и передать приложению.
void usb_check_data (void);
// Вернуть программное состояние подключения.
uint8_t usb_get_state (void);
// Освободить приём с учётом владения разборщика.
void usb_clear_buffer (void);

#endif
