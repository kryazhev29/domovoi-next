// Доработка: Кряжев А., 2026
// Точные двоичные дескрипторы CDC и строки устройства.
// Дескрипторы и порядок событий сохраняют протокол CDC.
// Общие файлы также используются загрузчиком; память PMA требует особого доступа.

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
