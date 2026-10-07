// Доработка: Кряжев А., 2026
// Питание USB и ограниченное ожидание RESET.
// Порядок событий и двоичные дескрипторы сохраняют протокол CDC.
// Общие файлы также используются загрузчиком.

#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_pwr.h"
#include "hw_config.h"
// Ограниченно ждать аппаратный RESET; вызывающий код выбирает восстановление и диагностику.
uint8_t USB_WaitReset(void)
{
  uint32_t budget = 32000u;
  while (budget != 0 && (_GetISTR() & ISTR_RESET) == 0) --budget;
  return budget != 0;
}

__IO uint32_t bDeviceState = UNCONNECTED; /* Текущее состояние устройства USB. */

// Включить USB с аппаратным порядком сброса и начальной маской событий.
RESULT PowerOn(void)
{
  uint16_t wRegVal;

  wRegVal = CNTR_FRES;
  _SetCNTR(wRegVal);

  wInterrupt_Mask = 0;
  _SetCNTR(wInterrupt_Mask);

  _SetISTR(0);

  wInterrupt_Mask = CNTR_RESETM | CNTR_SUSPM | CNTR_WKUPM;
  _SetCNTR(wInterrupt_Mask);

  return USB_SUCCESS;
}

// Выключить USB перед повторным подключением загрузчика.
RESULT PowerOff()
{

  _SetCNTR(CNTR_FRES);

  _SetISTR(0);

  _SetCNTR(CNTR_FRES + CNTR_PDWN);

  return USB_SUCCESS;
}

// Ввести USB в низкое потребление с сохранением прежнего состояния.

// Восстановить USB после приостановки.

// Выполнить согласованную фазу восстановления USB.
