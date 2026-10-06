// Доработка: Кряжев А., 2026
// Константы восьмибайтового SETUP и дескрипторов USB.
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

#ifndef __USB_DEF_H
#define __USB_DEF_H

typedef enum _RECIPIENT_TYPE
{
  DEVICE_RECIPIENT,
  INTERFACE_RECIPIENT,
  ENDPOINT_RECIPIENT,
  OTHER_RECIPIENT
} RECIPIENT_TYPE;

typedef enum _STANDARD_REQUESTS
{
  GET_STATUS = 0,
  CLEAR_FEATURE,
  RESERVED1,
  SET_FEATURE,
  RESERVED2,
  SET_ADDRESS,
  GET_DESCRIPTOR,
  SET_DESCRIPTOR,
  GET_CONFIGURATION,
  SET_CONFIGURATION,
  GET_INTERFACE,
  SET_INTERFACE,
  TOTAL_sREQUEST,
  SYNCH_FRAME = 12
} STANDARD_REQUESTS;

typedef enum _DESCRIPTOR_TYPE
{
  DEVICE_DESCRIPTOR = 1,
  CONFIG_DESCRIPTOR,
  STRING_DESCRIPTOR,
  INTERFACE_DESCRIPTOR,
  ENDPOINT_DESCRIPTOR
} DESCRIPTOR_TYPE;

typedef enum _FEATURE_SELECTOR
{
  ENDPOINT_STALL,
  DEVICE_REMOTE_WAKEUP
} FEATURE_SELECTOR;

#define REQUEST_TYPE      0x60
#define STANDARD_REQUEST  0x00
#define CLASS_REQUEST     0x20

#define RECIPIENT         0x1F

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
