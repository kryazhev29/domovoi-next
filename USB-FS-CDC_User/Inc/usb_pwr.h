// Доработка: Кряжев А., 2026
// Включение, отключение и ограниченное ожидание RESET.
// Дескрипторы и порядок событий сохраняют протокол CDC.
// Общие файлы также используются загрузчиком; память PMA требует особого доступа.

#ifndef __USB_PWR_H
#define __USB_PWR_H

typedef enum _DEVICE_STATE
{
  UNCONNECTED,
  ATTACHED,
  POWERED,
  SUSPENDED,
  ADDRESSED,
  CONFIGURED
} DEVICE_STATE;

// Ограниченно ждать RESET; вызывающий код обрабатывает отказ.
uint8_t USB_WaitReset(void);
// Включить USB с аппаратным порядком сброса и начальной маской событий.
RESULT PowerOn(void);
// Выключить USB перед повторным подключением загрузчика.
RESULT PowerOff(void);

extern  __IO uint32_t bDeviceState; /* Текущее состояние устройства USB. */

#endif
