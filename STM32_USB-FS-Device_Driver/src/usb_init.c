// Доработка: Кряжев А., 2026
// Связь таблиц класса CDC с ядром USB.
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

#include "usb_lib.h"

DEVICE_INFO *pInformation;

const DEVICE_PROP *pProperty;
uint16_t  wInterrupt_Mask;
DEVICE_INFO	Device_Info;
const USER_STANDARD_REQUESTS *pUser_Standard_Requests;

// Связать таблицы устройства и вызвать инициализацию класса.
void USB_Init(void)
{
  pInformation = &Device_Info;
  pInformation->ControlState = IN_DATA;
  pProperty = &Device_Property;
  pUser_Standard_Requests = &User_Standard_Requests;

  pProperty->Init();
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
