// Доработка: Кряжев А., 2026
// Включение, отключение и ограниченное ожидание RESET.
// Дескрипторы и порядок событий сохраняют протокол CDC.
// Общие файлы также используются загрузчиком; память PMA требует особого доступа.

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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
