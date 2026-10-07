// Доработка: Кряжев А., 2026
// Автомат управляющей точки ep0 и стандартные запросы хоста.
// Дескрипторы и порядок событий сохраняют протокол CDC.
// Общие файлы также используются загрузчиком; память PMA требует особого доступа.

#include "usb_lib.h"

uint16_t_uint8_t StatusInfo;

#ifdef BOOTLOADER
bool Data_Mul_MaxPacketSize = FALSE;
#else
usb_bool_t Data_Mul_MaxPacketSize = FALSE;
#endif

static void DataStageOut(void);
static void DataStageIn(void);
static void NoData_Setup0(void);
static void Data_Setup0(void);

// Выдать пустой пакет статуса IN; аппаратный статус EP0 применится после обработки IRQ.
static void Send0LengthData(void)
{
  _SetEPTxCount(ENDP0, 0);
  SaveTState = EP_TX_VALID;
}

// Вернуть текущую конфигурацию для стандартного запроса хоста.
uint8_t *Standard_GetConfiguration(uint16_t Length)
{
  if (Length == 0)
  {
    pInformation->Ctrl_Info.Usb_wLength =
      sizeof(pInformation->Current_Configuration);
    return 0;
  }
  pUser_Standard_Requests->User_GetConfiguration();
  return (uint8_t *)&pInformation->Current_Configuration;
}

// Проверить SET_CONFIGURATION и применить выбранную конфигурацию.
RESULT Standard_SetConfiguration(void)
{

  if ((pInformation->USBwValue0 <=
      Device_Table.Total_Configuration) && (pInformation->USBwValue1 == 0)
      && (pInformation->USBwIndex == 0))
  {
    pInformation->Current_Configuration = pInformation->USBwValue0;
    pUser_Standard_Requests->User_SetConfiguration();
    return USB_SUCCESS;
  }
  else
  {
    return USB_UNSUPPORT;
  }
}

// Вернуть текущее альтернативное состояние интерфейса.
uint8_t *Standard_GetInterface(uint16_t Length)
{
  if (Length == 0)
  {
    pInformation->Ctrl_Info.Usb_wLength =
      sizeof(pInformation->Current_AlternateSetting);
    return 0;
  }
  pUser_Standard_Requests->User_GetInterface();
  return (uint8_t *)&pInformation->Current_AlternateSetting;
}

// Проверить SET_INTERFACE и уведомить обработчик класса.
RESULT Standard_SetInterface(void)
{
  RESULT Re;

  Re = (*pProperty->Class_Get_Interface_Setting)(pInformation->USBwIndex0, pInformation->USBwValue0);

  if (pInformation->Current_Configuration != 0)
  {
    if ((Re != USB_SUCCESS) || (pInformation->USBwIndex1 != 0)
        || (pInformation->USBwValue1 != 0))
    {
      return  USB_UNSUPPORT;
    }
    else
    {
      pUser_Standard_Requests->User_SetInterface();
      pInformation->Current_Interface = pInformation->USBwIndex0;
      pInformation->Current_AlternateSetting = pInformation->USBwValue0;
      return USB_SUCCESS;
    }

  }

  return USB_UNSUPPORT;
}

// Собрать двухбайтовый статус устройства, интерфейса или конечной точки.
uint8_t *Standard_GetStatus(uint16_t Length)
{
  if (Length == 0)
  {
    pInformation->Ctrl_Info.Usb_wLength = 2;
    return 0;
  }

  StatusInfo.w = 0;

  if (Type_Recipient == (STANDARD_REQUEST | DEVICE_RECIPIENT))
  {

    uint8_t Feature = pInformation->Current_Feature;

    if ((Feature & (1u << 5)))
    {
      StatusInfo.bw.bb1 |= (1u << 1);
    }
    else
    {
      StatusInfo.bw.bb1 &= ~(1u << 1);
    }

    if ((Feature & (1u << 6)))
    {
      StatusInfo.bw.bb1 |= (1u << 0);
    }
    else
    {
      StatusInfo.bw.bb1 &= ~(1u << 0);
    }
  }

  else if (Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT))
  {
    return (uint8_t *)&StatusInfo;
  }

  else if (Type_Recipient == (STANDARD_REQUEST | ENDPOINT_RECIPIENT))
  {
    uint8_t Related_Endpoint;
    uint8_t wIndex0 = pInformation->USBwIndex0;

    Related_Endpoint = (wIndex0 & 0x0f);
    if (pInformation->USBwIndex1 != 0 || (wIndex0 & 0x70) != 0 ||
        Related_Endpoint >= 8u || Related_Endpoint >= Device_Table.Total_Endpoint) return NULL;
    if ((wIndex0 & (1u << 7)))
    {

      if (_GetTxStallStatus(Related_Endpoint))
      {
        StatusInfo.bw.bb1 |= (1u << 0);
      }
    }
    else
    {

      if (_GetRxStallStatus(Related_Endpoint))
      {
        StatusInfo.bw.bb1 |= (1u << 0);
      }
    }

  }
  else
  {
    return NULL;
  }
  pUser_Standard_Requests->User_GetStatus();
  return (uint8_t *)&StatusInfo;
}

// Снять разрешение пробуждения или блокировку проверенной конечной точки.
RESULT Standard_ClearFeature(void)
{
  uint32_t     Type_Rec = Type_Recipient;
  uint32_t     Status;

  if (Type_Rec == (STANDARD_REQUEST | DEVICE_RECIPIENT))
  {
    pInformation->Current_Feature &= ~(1u << 5);
    return USB_SUCCESS;
  }
  else if (Type_Rec == (STANDARD_REQUEST | ENDPOINT_RECIPIENT))
  {
    uint32_t Related_Endpoint;
    uint32_t wIndex0;
    uint32_t rEP;

    if ((pInformation->USBwValue != ENDPOINT_STALL)
        || (pInformation->USBwIndex1 != 0))
    {
      return USB_UNSUPPORT;
    }

    wIndex0 = pInformation->USBwIndex0;
    rEP = wIndex0 & ~0x80;
    Related_Endpoint = ENDP0 + rEP;
    // номер проверяем до обращения к регистрам USB
    if (rEP >= 8u ||
        rEP >= Device_Table.Total_Endpoint) return USB_UNSUPPORT;

    if ((pInformation->USBwIndex0 & (1u << 7)))
    {

      Status = _GetEPTxStatus(Related_Endpoint);
    }
    else
    {
      Status = _GetEPRxStatus(Related_Endpoint);
    }

    if ((Status == 0)
        || (pInformation->Current_Configuration == 0))
    {
      return USB_UNSUPPORT;
    }

    if (wIndex0 & 0x80)
    {

      if (_GetTxStallStatus(Related_Endpoint ))
      {
        ClearDTOG_TX(Related_Endpoint);
        SetEPTxStatus(Related_Endpoint, EP_TX_VALID);
      }
    }
    else
    {

      if (_GetRxStallStatus(Related_Endpoint))
      {
        if (Related_Endpoint == ENDP0)
        {

          SetEPRxCount(Related_Endpoint, Device_Property.MaxPacketSize);
          _SetEPRxStatus(Related_Endpoint, EP_RX_VALID);
        }
        else
        {
          ClearDTOG_RX(Related_Endpoint);
          _SetEPRxStatus(Related_Endpoint, EP_RX_VALID);
        }
      }
    }
    pUser_Standard_Requests->User_ClearFeature();
    return USB_SUCCESS;
  }

  return USB_UNSUPPORT;
}

// Заблокировать проверенную конечную точку по запросу SET_FEATURE.
RESULT Standard_SetEndPointFeature(void)
{
  uint32_t    wIndex0;
  uint32_t    Related_Endpoint;
  uint32_t    rEP;
  uint32_t    Status;

  wIndex0 = pInformation->USBwIndex0;
  rEP = wIndex0 & ~0x80;
  Related_Endpoint = ENDP0 + rEP;
  // номер проверяем до обращения к регистрам USB
  if (pInformation->USBwIndex1 != 0 || rEP >= Device_Table.Total_Endpoint ||
      pInformation->USBwValue != ENDPOINT_STALL ||
      pInformation->Current_Configuration == 0) return USB_UNSUPPORT;

  if ((pInformation->USBwIndex0 & (1u << 7)))
  {

    Status = _GetEPTxStatus(Related_Endpoint);
  }
  else
  {
    Status = _GetEPRxStatus(Related_Endpoint);
  }

  if (Status == 0)
  {
    return USB_UNSUPPORT;
  }
  else
  {
    if (wIndex0 & 0x80)
    {

      _SetEPTxStatus(Related_Endpoint, EP_TX_STALL);
    }

    else
    {

      _SetEPRxStatus(Related_Endpoint, EP_RX_STALL);
    }
  }
  pUser_Standard_Requests->User_SetEndPointFeature();
  return USB_SUCCESS;
}

// Запомнить разрешение удалённого пробуждения в статусе USB.
RESULT Standard_SetDeviceFeature(void)
{
  pInformation->Current_Feature |= (1u << 5);
  pUser_Standard_Requests->User_SetDeviceFeature();
  return USB_SUCCESS;
}

// Вернуть размер или очередной фрагмент выбранного дескриптора.
uint8_t *Standard_GetDescriptorData(uint16_t Length, const ONE_DESCRIPTOR *pDesc)
{
  uint32_t  wOffset;

  wOffset = pInformation->Ctrl_Info.Usb_wOffset;
  if (Length == 0)
  {
    pInformation->Ctrl_Info.Usb_wLength = pDesc->Descriptor_Size - wOffset;
    return 0;
  }

  return pDesc->Descriptor + wOffset;
}

// Принять очередную порцию данных управляющего запроса EP0.
void DataStageOut(void)
{
  ENDPOINT_INFO *pEPinfo = &pInformation->Ctrl_Info;
  uint32_t save_rLength;

  save_rLength = pEPinfo->Usb_rLength;

  if (pEPinfo->CopyData && save_rLength)
  {
    uint8_t *Buffer;
    uint32_t Length;

    Length = pEPinfo->PacketSize;
    if (Length > save_rLength)
    {
      Length = save_rLength;
    }

    Buffer = (*pEPinfo->CopyData)(Length);
    pEPinfo->Usb_rLength -= Length;
    pEPinfo->Usb_rOffset += Length;
    PMAToUserBufferCopy(Buffer, GetEPRxAddr(ENDP0), Length);

  }

  if (pEPinfo->Usb_rLength != 0)
  {
    SaveRState = EP_RX_VALID;
    SetEPTxCount(ENDP0, 0);
    SaveTState = EP_TX_VALID;
  }

  if (pEPinfo->Usb_rLength >= pEPinfo->PacketSize)
  {
    pInformation->ControlState = OUT_DATA;
  }
  else
  {
    if (pEPinfo->Usb_rLength > 0)
    {
      pInformation->ControlState = LAST_OUT_DATA;
    }
    else
    {
      pInformation->ControlState = WAIT_STATUS_IN;
      Send0LengthData();
    }
  }
}

// Передать очередную порцию EP0 и нужный завершающий пустой пакет.
void DataStageIn(void)
{
  ENDPOINT_INFO *pEPinfo = &pInformation->Ctrl_Info;
  uint32_t save_wLength = pEPinfo->Usb_wLength;
  uint32_t ControlState = pInformation->ControlState;

  uint8_t *DataBuffer;
  uint32_t Length;

  if ((save_wLength == 0) && (ControlState == LAST_IN_DATA))
  {
    if(Data_Mul_MaxPacketSize == TRUE)
    {

      Send0LengthData();
      ControlState = LAST_IN_DATA;
      Data_Mul_MaxPacketSize = FALSE;
    }
    else
    {

      ControlState = WAIT_STATUS_OUT;
      SaveTState = EP_TX_STALL;

    }

    goto Expect_Status_Out;
  }

  Length = pEPinfo->PacketSize;
  ControlState = (save_wLength <= Length) ? LAST_IN_DATA : IN_DATA;

  if (Length > save_wLength)
  {
    Length = save_wLength;
  }

  DataBuffer = (*pEPinfo->CopyData)(Length);

  UserToPMABufferCopy(DataBuffer, GetEPTxAddr(ENDP0), Length);

  SetEPTxCount(ENDP0, Length);

  pEPinfo->Usb_wLength -= Length;
  pEPinfo->Usb_wOffset += Length;
  SaveTState = EP_TX_VALID;

  SaveRState = EP_RX_VALID;

Expect_Status_Out:
  pInformation->ControlState = ControlState;
}

// Обработать SETUP без данных и выбрать ответ либо STALL.
void NoData_Setup0(void)
{
  RESULT Result = USB_UNSUPPORT;
  uint32_t RequestNo = pInformation->USBbRequest;
  uint32_t ControlState;

  if (Type_Recipient == (STANDARD_REQUEST | DEVICE_RECIPIENT))
  {

    if (RequestNo == SET_CONFIGURATION)
    {
      Result = Standard_SetConfiguration();
    }

    else if (RequestNo == SET_ADDRESS)
    {
      if ((pInformation->USBwValue0 > 127) || (pInformation->USBwValue1 != 0)
          || (pInformation->USBwIndex != 0)
          || (pInformation->Current_Configuration != 0))

      {
        ControlState = STALLED;
        goto exit_NoData_Setup0;
      }
      else
      {
        Result = USB_SUCCESS;
      }
    }

    else if (RequestNo == SET_FEATURE)
    {
      if ((pInformation->USBwValue0 == DEVICE_REMOTE_WAKEUP) \
          && (pInformation->USBwIndex == 0))
      {
        Result = Standard_SetDeviceFeature();
      }
      else
      {
        Result = USB_UNSUPPORT;
      }
    }

    else if (RequestNo == CLEAR_FEATURE)
    {
      if (pInformation->USBwValue0 == DEVICE_REMOTE_WAKEUP
          && pInformation->USBwIndex == 0
          && (pInformation->Current_Feature & (1u << 5)))
      {
        Result = Standard_ClearFeature();
      }
      else
      {
        Result = USB_UNSUPPORT;
      }
    }

  }

  else if (Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT))
  {

    if (RequestNo == SET_INTERFACE)
    {
      Result = Standard_SetInterface();
    }
  }

  else if (Type_Recipient == (STANDARD_REQUEST | ENDPOINT_RECIPIENT))
  {

    if (RequestNo == CLEAR_FEATURE)
    {
      Result = Standard_ClearFeature();
    }

    else if (RequestNo == SET_FEATURE)
    {
      Result = Standard_SetEndPointFeature();
    }
  }
  else
  {
    Result = USB_UNSUPPORT;
  }

  if (Result != USB_SUCCESS)
  {
    Result = (*pProperty->Class_NoData_Setup)(RequestNo);
  }

  if (Result != USB_SUCCESS)
  {
    ControlState = STALLED;
    goto exit_NoData_Setup0;
  }

  ControlState = WAIT_STATUS_IN;

  Send0LengthData();

exit_NoData_Setup0:
  pInformation->ControlState = ControlState;
  return;
}

// Проверить SETUP с данными и назначить источник или приёмник EP0.
void Data_Setup0(void)
{
  uint8_t *(*CopyRoutine)(uint16_t);
  RESULT Result;
  uint32_t Request_No = pInformation->USBbRequest;

  uint32_t Related_Endpoint, Reserved;
  uint32_t wOffset, Status;

  CopyRoutine = NULL;
  wOffset = 0;

  if (Request_No == GET_DESCRIPTOR)
  {
    if (Type_Recipient == (STANDARD_REQUEST | DEVICE_RECIPIENT))
    {
      uint8_t wValue1 = pInformation->USBwValue1;
      if (wValue1 == DEVICE_DESCRIPTOR)
      {
        CopyRoutine = pProperty->GetDeviceDescriptor;
      }
      else if (wValue1 == CONFIG_DESCRIPTOR)
      {
        CopyRoutine = pProperty->GetConfigDescriptor;
      }
      else if (wValue1 == STRING_DESCRIPTOR)
      {
        CopyRoutine = pProperty->GetStringDescriptor;
      }
    }
  }

  else if ((Request_No == GET_STATUS) && (pInformation->USBwValue == 0)
           && (pInformation->USBwLength == 0x0002)
           && (pInformation->USBwIndex1 == 0))
  {

    if ((Type_Recipient == (STANDARD_REQUEST | DEVICE_RECIPIENT))
        && (pInformation->USBwIndex == 0))
    {
      CopyRoutine = Standard_GetStatus;
    }

    else if (Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT))
    {
      if (((*pProperty->Class_Get_Interface_Setting)(pInformation->USBwIndex0, 0) == USB_SUCCESS)
          && (pInformation->Current_Configuration != 0))
      {
        CopyRoutine = Standard_GetStatus;
      }
    }

    else if (Type_Recipient == (STANDARD_REQUEST | ENDPOINT_RECIPIENT))
    {
      Related_Endpoint = (pInformation->USBwIndex0 & 0x0f);
      Reserved = pInformation->USBwIndex0 & 0x70;

      // недопустимый SETUP не должен читать даже неиспользуемую конечную точку
      if (Related_Endpoint >= 8u || Related_Endpoint >= Device_Table.Total_Endpoint || Reserved != 0)
      {
        Status = 0;
      }
      else if ((pInformation->USBwIndex0 & (1u << 7)))
      {

        Status = _GetEPTxStatus(Related_Endpoint);
      }
      else
      {
        Status = _GetEPRxStatus(Related_Endpoint);
      }

      if ((Related_Endpoint < Device_Table.Total_Endpoint) && (Reserved == 0)
          && (Status != 0))
      {
        CopyRoutine = Standard_GetStatus;
      }
    }

  }

  else if (Request_No == GET_CONFIGURATION)
  {
    if (Type_Recipient == (STANDARD_REQUEST | DEVICE_RECIPIENT))
    {
      CopyRoutine = Standard_GetConfiguration;
    }
  }

  else if (Request_No == GET_INTERFACE)
  {
    if ((Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT))
        && (pInformation->Current_Configuration != 0) && (pInformation->USBwValue == 0)
        && (pInformation->USBwIndex1 == 0) && (pInformation->USBwLength == 0x0001)
        && ((*pProperty->Class_Get_Interface_Setting)(pInformation->USBwIndex0, 0) == USB_SUCCESS))
    {
      CopyRoutine = Standard_GetInterface;
    }

  }

  if (CopyRoutine)
  {
    pInformation->Ctrl_Info.Usb_wOffset = wOffset;
    pInformation->Ctrl_Info.CopyData = CopyRoutine;

    (*CopyRoutine)(0);
    Result = USB_SUCCESS;
  }
  else
  {
    Result = (*pProperty->Class_Data_Setup)(pInformation->USBbRequest);
  }

  if ((Result == USB_UNSUPPORT) || (pInformation->Ctrl_Info.Usb_wLength == 0))
  {

    pInformation->ControlState = STALLED;
    return;
  }

  if ((pInformation->USBbmRequestType & (1u << 7)))
  {

    __IO uint32_t wLength = pInformation->USBwLength;

    if (pInformation->Ctrl_Info.Usb_wLength > wLength)
    {
      pInformation->Ctrl_Info.Usb_wLength = wLength;
    }

    else if (pInformation->Ctrl_Info.Usb_wLength < pInformation->USBwLength)
    {
      if (pInformation->Ctrl_Info.Usb_wLength < pProperty->MaxPacketSize)
      {
        Data_Mul_MaxPacketSize = FALSE;
      }
      else if ((pInformation->Ctrl_Info.Usb_wLength % pProperty->MaxPacketSize) == 0)
      {
        Data_Mul_MaxPacketSize = TRUE;
      }
    }

    pInformation->Ctrl_Info.PacketSize = pProperty->MaxPacketSize;
    DataStageIn();
  }
  else
  {
    pInformation->ControlState = OUT_DATA;
    SaveRState = EP_RX_VALID;
  }

  return;
}

// Снять восьмибайтовый SETUP из PMA и запустить разбор EP0.
void Setup0_Process(void)
{

  union
  {
    uint8_t* b;
    uint16_t* w;
  } pBuf;
  uint16_t offset = 1;

  pBuf.b = PMAAddr + (uint8_t *)(_GetEPRxAddr(ENDP0) * 2);

  pInformation->USBbmRequestType = *pBuf.b++;
  pInformation->USBbRequest = *pBuf.b++;
  pBuf.w += offset;
  pInformation->USBwValue = ByteSwap(*pBuf.w++);
  pBuf.w += offset;
  pInformation->USBwIndex  = ByteSwap(*pBuf.w++);
  pBuf.w += offset;
  pInformation->USBwLength = *pBuf.w;

  pInformation->ControlState = SETTING_UP;
  if (pInformation->USBwLength == 0)
  {

    NoData_Setup0();
  }
  else
  {

    Data_Setup0();
  }
  Post0_Process();
}

// Завершить передачу EP0, применяя новый адрес после ответа хосту.
void In0_Process(void)
{
  uint32_t ControlState = pInformation->ControlState;

  if ((ControlState == IN_DATA) || (ControlState == LAST_IN_DATA))
  {
    DataStageIn();

    ControlState = pInformation->ControlState;
  }

  else if (ControlState == WAIT_STATUS_IN)
  {
    if ((pInformation->USBbRequest == SET_ADDRESS) &&
        (Type_Recipient == (STANDARD_REQUEST | DEVICE_RECIPIENT)))
    {
      SetDeviceAddress(pInformation->USBwValue0);
      pUser_Standard_Requests->User_SetDeviceAddress();
    }
    (*pProperty->Process_Status_IN)();
    ControlState = STALLED;
  }

  else
  {
    ControlState = STALLED;
  }

  pInformation->ControlState = ControlState;

  Post0_Process();
}

// Обработать данные либо статус OUT управляющей точки.
void Out0_Process(void)
{
  uint32_t ControlState = pInformation->ControlState;

  if ((ControlState == IN_DATA) || (ControlState == LAST_IN_DATA))
  {

    ControlState = STALLED;
  }
  else if ((ControlState == OUT_DATA) || (ControlState == LAST_OUT_DATA))
  {
    DataStageOut();
    ControlState = pInformation->ControlState;
  }

  else if (ControlState == WAIT_STATUS_OUT)
  {
    (*pProperty->Process_Status_OUT)();
    ControlState = STALLED;
  }

  else
  {
    ControlState = STALLED;
  }

  pInformation->ControlState = ControlState;

  Post0_Process();
}

// Подготовить EP0 к следующему пакету или отклонить запрос.
void Post0_Process(void)
{

  SetEPRxCount(ENDP0, Device_Property.MaxPacketSize);

  if (pInformation->ControlState == STALLED)
  {
    SaveRState = EP_RX_STALL;
    SaveTState = EP_TX_STALL;
  }

}

// Назначить адрес устройства и адреса его конечных точек.
void SetDeviceAddress(uint8_t Val)
{
  uint32_t i;
  uint32_t nEP = Device_Table.Total_Endpoint;

  for (i = 0; i < nEP; i++)
  {
    _SetEPAddress((uint8_t)i, (uint8_t)i);
  }
  _SetDADDR(Val | DADDR_EF);
}

// Обслужить допустимое событие, которому не нужно действие класса.
void NOP_Process(void)
{
}
