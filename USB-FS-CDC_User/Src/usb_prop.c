// Доработка: Кряжев А., 2026
// Дескрипторы, управляющие запросы и параметры CDC.
// Порядок событий и двоичные дескрипторы сохраняют протокол CDC.
// Общие файлы также используются загрузчиком.

#include "../../User/domovoi_diag.h"
// Доработка: Кряжев А., 2026

#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "hw_config.h"

LINE_CODING linecoding =
  {
    115200,
    0x00,
    0x00,
    0x08
  };

const DEVICE Device_Table =
  {
    EP_NUM,
    1
  };

const DEVICE_PROP Device_Property =
  {
    Virtual_Com_Port_init,
    Virtual_Com_Port_Reset,
    NOP_Process,
    NOP_Process,
    Virtual_Com_Port_Data_Setup,
    Virtual_Com_Port_NoData_Setup,
    Virtual_Com_Port_Get_Interface_Setting,
    Virtual_Com_Port_GetDeviceDescriptor,
    Virtual_Com_Port_GetConfigDescriptor,
    Virtual_Com_Port_GetStringDescriptor,
    0x40
  };

const USER_STANDARD_REQUESTS User_Standard_Requests =
  {
    NOP_Process,
    Virtual_Com_Port_SetConfiguration,
    NOP_Process,
    NOP_Process,
    NOP_Process,
    NOP_Process,
    NOP_Process,
    NOP_Process,
    Virtual_Com_Port_SetDeviceAddress
  };

const ONE_DESCRIPTOR Device_Descriptor =
  {
    (uint8_t*)Virtual_Com_Port_DeviceDescriptor,
    VIRTUAL_COM_PORT_SIZ_DEVICE_DESC
  };

const ONE_DESCRIPTOR Config_Descriptor =
  {
    (uint8_t*)Virtual_Com_Port_ConfigDescriptor,
    VIRTUAL_COM_PORT_SIZ_CONFIG_DESC
  };

const ONE_DESCRIPTOR String_Descriptor[4] =
  {
    {(uint8_t*)Virtual_Com_Port_StringLangID, VIRTUAL_COM_PORT_SIZ_STRING_LANGID},
    {(uint8_t*)Virtual_Com_Port_StringVendor, VIRTUAL_COM_PORT_SIZ_STRING_VENDOR},
    {(uint8_t*)Virtual_Com_Port_StringProduct, VIRTUAL_COM_PORT_SIZ_STRING_PRODUCT},
    {(uint8_t*)Virtual_Com_Port_StringSerial, VIRTUAL_COM_PORT_SIZ_STRING_SERIAL}
  };

// Подготовить CDC и серийный номер перед подключением к хосту.
void Virtual_Com_Port_init(void)
{

  Get_SerialNum();

  pInformation->Current_Configuration = 0;

  PowerOn();

  USB_SIL_Init();

  bDeviceState = UNCONNECTED;
}

#ifndef BOOTLOADER
extern void usb_clear_buffer(void);
extern volatile uint32_t usb_bus_resets;
#endif
// Сбросить очередь и настроить точки EP0–EP3 для CDC.
void Virtual_Com_Port_Reset(void)
{
#ifndef BOOTLOADER
  usb_bus_resets++;
  USB_Tx_Reset();
  usb_clear_buffer();
#else
  USB_Tx_Reset();
#endif

  pInformation->Current_Configuration = 0;

  pInformation->Current_Feature = Virtual_Com_Port_ConfigDescriptor[7];

  pInformation->Current_Interface = 0;

  SetBTABLE(BTABLE_ADDRESS);

  SetEPType(ENDP0, EP_CONTROL);
  SetEPTxStatus(ENDP0, EP_TX_STALL);
  SetEPRxAddr(ENDP0, ENDP0_RXADDR);
  SetEPTxAddr(ENDP0, ENDP0_TXADDR);
  Clear_Status_Out(ENDP0);
  SetEPRxCount(ENDP0, Device_Property.MaxPacketSize);
  SetEPRxValid(ENDP0);

  SetEPType(ENDP1, EP_BULK);
  SetEPTxAddr(ENDP1, ENDP1_TXADDR);
  SetEPTxStatus(ENDP1, EP_TX_NAK);
  SetEPRxStatus(ENDP1, EP_RX_DIS);

  SetEPType(ENDP2, EP_INTERRUPT);
  SetEPTxAddr(ENDP2, ENDP2_TXADDR);
  SetEPRxStatus(ENDP2, EP_RX_DIS);
  SetEPTxStatus(ENDP2, EP_TX_NAK);

  SetEPType(ENDP3, EP_BULK);
  SetEPRxAddr(ENDP3, ENDP3_RXADDR);
  SetEPRxCount(ENDP3, VIRTUAL_COM_PORT_DATA_SIZE);
  SetEPRxStatus(ENDP3, EP_RX_VALID);
  SetEPTxStatus(ENDP3, EP_TX_DIS);

  SetDeviceAddress(0);

  bDeviceState = ATTACHED;
}

// Отметить выбранную хостом конфигурацию CDC.
void Virtual_Com_Port_SetConfiguration(void)
{
  if (pInformation->Current_Configuration != 0)
  {

    bDeviceState = CONFIGURED;
  }
}

// Отметить назначение адреса устройства.
void Virtual_Com_Port_SetDeviceAddress (void)
{
  bDeviceState = ADDRESSED;
}

// Назначить данные GET/SET_LINE_CODING управляющего запроса.
RESULT Virtual_Com_Port_Data_Setup(uint8_t RequestNo)
{
  uint8_t    *(*CopyRoutine)(uint16_t);

  CopyRoutine = NULL;

  if (RequestNo == GET_LINE_CODING)
  {
    if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
    {
      CopyRoutine = Virtual_Com_Port_GetLineCoding;
    }
  }
  else if (RequestNo == SET_LINE_CODING)
  {
    if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
    {
      CopyRoutine = Virtual_Com_Port_SetLineCoding;
    }
  }

  if (CopyRoutine == NULL)
  {
    return USB_UNSUPPORT;
  }

  pInformation->Ctrl_Info.CopyData = CopyRoutine;
  pInformation->Ctrl_Info.Usb_wOffset = 0;
  (*CopyRoutine)(0);
  return USB_SUCCESS;
}

// Принять поддерживаемый CDC-запрос без данных.
RESULT Virtual_Com_Port_NoData_Setup(uint8_t RequestNo)
{

  if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT))
  {
    if (RequestNo == SET_COMM_FEATURE)
    {
      return USB_SUCCESS;
    }
    else if (RequestNo == SET_CONTROL_LINE_STATE)
    {
      return USB_SUCCESS;
    }
  }

  return USB_UNSUPPORT;
}

// Выдать фрагмент дескриптора устройства.
uint8_t *Virtual_Com_Port_GetDeviceDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &Device_Descriptor);
}

// Выдать фрагмент конфигурации CDC.
uint8_t *Virtual_Com_Port_GetConfigDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &Config_Descriptor);
}

// Выдать существующую строку USB либо отклонить её индекс.
uint8_t *Virtual_Com_Port_GetStringDescriptor(uint16_t Length)
{
  uint8_t wValue0 = pInformation->USBwValue0;
  if (wValue0 >= sizeof(String_Descriptor) / sizeof(String_Descriptor[0]))
  {
    return NULL;
  }
  else
  {
    return Standard_GetDescriptorData(Length, &String_Descriptor[wValue0]);
  }
}

// Разрешить два интерфейса CDC с нулевой альтернативой.
RESULT Virtual_Com_Port_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting)
{
  if (AlternateSetting > 0)
  {
    return USB_UNSUPPORT;
  }
  else if (Interface > 1)
  {
    return USB_UNSUPPORT;
  }
  return USB_SUCCESS;
}

// Выдать текущие параметры виртуального порта.
uint8_t *Virtual_Com_Port_GetLineCoding(uint16_t Length)
{
  if (Length == 0)
  {
    pInformation->Ctrl_Info.Usb_wLength = sizeof(linecoding);
    return NULL;
  }
  return(uint8_t *)&linecoding;
}

// Принять параметры в семибайтовое поле CDC.
uint8_t *Virtual_Com_Port_SetLineCoding(uint16_t Length)
{
  if (Length == 0)
  {
    pInformation->Ctrl_Info.Usb_wLength = sizeof(linecoding);
    return NULL;
  }
  return(uint8_t *)&linecoding;
}
