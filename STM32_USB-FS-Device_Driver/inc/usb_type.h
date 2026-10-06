// Доработка: Кряжев А., 2026
// Типы библиотеки USB для приложения и загрузчика.
// Дескрипторы и порядок событий сохраняют протокол CDC.
// Общие файлы также используются загрузчиком; память PMA требует особого доступа.

/**
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
