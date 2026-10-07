// Доработка: Кряжев А., 2026
// Типы библиотеки USB для приложения и загрузчика.
// Дескрипторы и порядок событий сохраняют протокол CDC.
// Общие файлы также используются загрузчиком; память PMA требует особого доступа.

#ifndef __USB_TYPE_H
#define __USB_TYPE_H

#include "usb_conf.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifdef BOOTLOADER
#ifndef FALSE
typedef enum
{
  FALSE = 0, TRUE  = !FALSE
}
bool;
#endif
#else
#include "usb_bool.h"
#endif

#endif
