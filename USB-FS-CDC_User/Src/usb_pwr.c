// Доработка: Кряжев А., 2026
// Питание USB и ограниченное ожидание RESET.
// Порядок событий и двоичные дескрипторы сохраняют протокол CDC.
// Общие файлы также используются загрузчиком.

/**
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
