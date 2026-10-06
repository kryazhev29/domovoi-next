// Доработка: Кряжев А., 2026
// Обработчики событий USB и восстановление по ESOF.
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

#ifndef __USB_ISTR_H
#define __USB_ISTR_H

#include "usb_conf.h"

 void USB_Istr(void);

// Подтвердить пакет IN и передать следующую часть ответа.
void EP1_IN_Callback(void);

// Принять пакет OUT и снова разрешить приём точки.
void EP3_OUT_Callback(void);

// Отметить кадр USB и с заданным интервалом продвинуть очередь IN.
void SOF_Callback(void);

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
