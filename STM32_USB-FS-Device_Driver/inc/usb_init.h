// Доработка: Кряжев А., 2026
// Связь таблиц класса CDC с ядром USB.
// Дескрипторы и порядок событий сохраняют протокол CDC.
// Общие файлы также используются загрузчиком; память PMA требует особого доступа.

#ifndef __USB_INIT_H
#define __USB_INIT_H

// Связать таблицы устройства и вызвать инициализацию класса.
void USB_Init(void);

extern DEVICE_INFO*	pInformation;

extern const DEVICE_PROP *	pProperty;

extern const USER_STANDARD_REQUESTS *pUser_Standard_Requests;
extern uint16_t wInterrupt_Mask;

#endif
