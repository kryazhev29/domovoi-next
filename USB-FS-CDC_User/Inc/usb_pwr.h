// Доработка: Кряжев А., 2026
// USB CDC платы v5: usb_pwr.
// Модуль сохраняет дескрипторы, порядок событий и двоичный протокол приложения.
// Диагностические счётчики отделены от обязательных таймаутов восстановления.
/**
  ******************************************************************************
  * @file    usb_pwr.h
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   Connection/disconnection & power management header
  ******************************************************************************
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


typedef enum _RESUME_STATE
{
  RESUME_EXTERNAL,
  RESUME_INTERNAL,
  RESUME_LATER,
  RESUME_WAIT,
  RESUME_START,
  RESUME_ON,
  RESUME_OFF,
  RESUME_ESOF
} RESUME_STATE;

typedef enum _DEVICE_STATE
{
  UNCONNECTED,
  ATTACHED,
  POWERED,
  SUSPENDED,
  ADDRESSED,
  CONFIGURED
} DEVICE_STATE;

uint8_t USB_WaitReset(void);
void Suspend(void);
void Resume_Init(void);
void Resume(RESUME_STATE eResumeSetVal);
RESULT PowerOn(void);
RESULT PowerOff(void);

extern  __IO uint32_t bDeviceState; /* Текущее состояние устройства USB. */
#ifdef BOOTLOADER
extern __IO bool fSuspendEnabled;
#else
extern __IO usb_bool_t fSuspendEnabled;
#endif  /* Разрешение аппаратной приостановки USB. */

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
