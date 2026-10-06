// Доработка: Кряжев А., 2026
// USB CDC платы v5: usb_desc.
// Модуль сохраняет дескрипторы, порядок событий и двоичный протокол приложения.
// Диагностические счётчики отделены от обязательных таймаутов восстановления.
/**
  ******************************************************************************
  * @file    usb_desc.c
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   Descriptors for Virtual Com Port Demo
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


#include "usb_lib.h"
#include "usb_desc.h"

/* Дескриптор устройства USB: значения формата остаются неизменными. */
const uint8_t Virtual_Com_Port_DeviceDescriptor[] =
  {
    0x12,
    USB_DEVICE_DESCRIPTOR_TYPE,
    0x00,
    0x02,
    0x02,
    0x00,
    0x00,
    0x40,
    0x83,
    0x04,
    0x40,
    0x57,
    0x00,
    0x02,
    1,
    2,
    3,
    0x01
  };

const uint8_t Virtual_Com_Port_ConfigDescriptor[] =
  {
    /* Дескриптор конфигурации CDC. */
    0x09,
    USB_CONFIGURATION_DESCRIPTOR_TYPE,
    VIRTUAL_COM_PORT_SIZ_CONFIG_DESC,
    0x00,
    0x02,
    0x01,
    0x00,
    0xC0,
    0x32,
    /* Дескриптор интерфейса CDC. */
    0x09,
    USB_INTERFACE_DESCRIPTOR_TYPE,

    0x00,
    0x00,
    0x01,
    0x02,
    0x02,
    0x01,
    0x00,

    0x05,
    0x24,
    0x00,
    0x10,
    0x01,

    0x05,
    0x24,
    0x01,
    0x00,
    0x01,

    0x04,
    0x24,
    0x02,
    0x02,

    0x05,
    0x24,
    0x06,
    0x00,
    0x01,

    0x07,
    USB_ENDPOINT_DESCRIPTOR_TYPE,
    0x82,
    0x03,
    VIRTUAL_COM_PORT_INT_SIZE,
    0x00,
    0xFF,

    0x09,
    USB_INTERFACE_DESCRIPTOR_TYPE,
    0x01,
    0x00,
    0x02,
    0x0A,
    0x00,
    0x00,
    0x00,

    0x07,
    USB_ENDPOINT_DESCRIPTOR_TYPE,
    0x03,
    0x02,
    VIRTUAL_COM_PORT_DATA_SIZE,
    0x00,
    0x00,

    0x07,
    USB_ENDPOINT_DESCRIPTOR_TYPE,
    0x81,
    0x02,
    VIRTUAL_COM_PORT_DATA_SIZE,
    0x00,
    0x00
  };


const uint8_t Virtual_Com_Port_StringLangID[VIRTUAL_COM_PORT_SIZ_STRING_LANGID] =
  {
    VIRTUAL_COM_PORT_SIZ_STRING_LANGID,
    USB_STRING_DESCRIPTOR_TYPE,
    0x09,
    0x04
  };

const uint8_t Virtual_Com_Port_StringVendor[VIRTUAL_COM_PORT_SIZ_STRING_VENDOR] =
  {
    VIRTUAL_COM_PORT_SIZ_STRING_VENDOR,
    USB_STRING_DESCRIPTOR_TYPE,

    'S', 0, 'T', 0, 'M', 0, 'i', 0, 'c', 0, 'r', 0, 'o', 0, 'e', 0,
    'l', 0, 'e', 0, 'c', 0, 't', 0, 'r', 0, 'o', 0, 'n', 0, 'i', 0,
    'c', 0, 's', 0
  };

const uint8_t Virtual_Com_Port_StringProduct[VIRTUAL_COM_PORT_SIZ_STRING_PRODUCT] =
  {
    VIRTUAL_COM_PORT_SIZ_STRING_PRODUCT,
    USB_STRING_DESCRIPTOR_TYPE,

    'S', 0, 'T', 0, 'M', 0, '3', 0, '2', 0, ' ', 0, 'V', 0, 'i', 0,
    'r', 0, 't', 0, 'u', 0, 'a', 0, 'l', 0, ' ', 0, 'C', 0, 'O', 0,
    'M', 0, ' ', 0, 'P', 0, 'o', 0, 'r', 0, 't', 0, ' ', 0, ' ', 0
  };

uint8_t Virtual_Com_Port_StringSerial[VIRTUAL_COM_PORT_SIZ_STRING_SERIAL] =
  {
    VIRTUAL_COM_PORT_SIZ_STRING_SERIAL,
    USB_STRING_DESCRIPTOR_TYPE,
    'S', 0, 'T', 0, 'M', 0, '3', 0, '2', 0
  };

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
