// Доработка: Кряжев А., 2026
// USB CDC платы v5: usb_bool.
// Модуль сохраняет дескрипторы, порядок событий и двоичный протокол приложения.
// Диагностические счётчики отделены от обязательных таймаутов восстановления.
/* Перечисление USB сохраняет ABI ST, не занимая стандартное имя bool языка C. */
#ifndef DOMOVOI_USB_BOOL_H
#define DOMOVOI_USB_BOOL_H
typedef enum { USB_FALSE = 0, USB_TRUE = 1 } usb_bool_t;
#ifndef FALSE
#define FALSE USB_FALSE
#define TRUE USB_TRUE
#endif
#endif
