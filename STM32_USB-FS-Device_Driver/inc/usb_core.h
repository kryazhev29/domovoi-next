// Доработка: Кряжев А., 2026
// Автомат управляющей точки ep0 и стандартные запросы хоста.
// Дескрипторы и порядок событий сохраняют протокол CDC.
// Общие файлы также используются загрузчиком; память PMA требует особого доступа.

#ifndef __USB_CORE_H
#define __USB_CORE_H

typedef enum _CONTROL_STATE
{
  WAIT_SETUP,
  SETTING_UP,
  IN_DATA,
  OUT_DATA,
  LAST_IN_DATA,
  LAST_OUT_DATA,
  WAIT_STATUS_IN,
  WAIT_STATUS_OUT,
  STALLED
} CONTROL_STATE;

typedef struct OneDescriptor
{
  uint8_t *Descriptor;
  uint16_t Descriptor_Size;
}
ONE_DESCRIPTOR;

typedef enum _RESULT
{
  USB_SUCCESS = 0,
  USB_ERROR,
  USB_UNSUPPORT
} RESULT;

typedef struct _ENDPOINT_INFO
{

  uint16_t  Usb_wLength;
  uint16_t  Usb_wOffset;
  uint16_t  PacketSize;
  uint8_t   *(*CopyData)(uint16_t Length);
}ENDPOINT_INFO;

typedef struct _DEVICE
{
  uint8_t Total_Endpoint;
  uint8_t Total_Configuration;
}
DEVICE;

typedef union
{
  uint16_t w;
  struct BW
  {
    uint8_t bb1;
    uint8_t bb0;
  }
  bw;
} uint16_t_uint8_t;

typedef struct _DEVICE_INFO
{
  uint8_t USBbmRequestType;
  uint8_t USBbRequest;
  uint16_t_uint8_t USBwValues;
  uint16_t_uint8_t USBwIndexs;
  uint16_t_uint8_t USBwLengths;

  uint8_t ControlState;
  uint8_t Current_Feature;
  uint8_t Current_Configuration;
  uint8_t Current_Interface;
  uint8_t Current_AlternateSetting;

  ENDPOINT_INFO Ctrl_Info;
}DEVICE_INFO;

typedef struct _DEVICE_PROP
{
  void (*Init)(void);
  void (*Reset)(void);

  void (*Process_Status_IN)(void);
  void (*Process_Status_OUT)(void);

  RESULT (*Class_Data_Setup)(uint8_t RequestNo);

  RESULT (*Class_NoData_Setup)(uint8_t RequestNo);

  RESULT  (*Class_Get_Interface_Setting)(uint8_t Interface, uint8_t AlternateSetting);

  uint8_t* (*GetDeviceDescriptor)(uint16_t Length);
  uint8_t* (*GetConfigDescriptor)(uint16_t Length);
  uint8_t* (*GetStringDescriptor)(uint16_t Length);

  uint8_t MaxPacketSize;

}DEVICE_PROP;

typedef struct _USER_STANDARD_REQUESTS
{
  void (*User_GetConfiguration)(void);
  void (*User_SetConfiguration)(void);
  void (*User_GetInterface)(void);
  void (*User_SetInterface)(void);
  void (*User_GetStatus)(void);
  void (*User_ClearFeature)(void);
  void (*User_SetEndPointFeature)(void);
  void (*User_SetDeviceFeature)(void);
  void (*User_SetDeviceAddress)(void);
}
USER_STANDARD_REQUESTS;

#define Type_Recipient (pInformation->USBbmRequestType & (REQUEST_TYPE | RECIPIENT))

#define Usb_rLength Usb_wLength
#define Usb_rOffset Usb_wOffset

#define USBwValue USBwValues.w
#define USBwValue0 USBwValues.bw.bb0
#define USBwValue1 USBwValues.bw.bb1
#define USBwIndex USBwIndexs.w
#define USBwIndex0 USBwIndexs.bw.bb0
#define USBwIndex1 USBwIndexs.bw.bb1
#define USBwLength USBwLengths.w

// Снять восьмибайтовый SETUP из PMA и запустить разбор EP0.
void Setup0_Process(void);
// Подготовить EP0 к следующему пакету или отклонить запрос.
void Post0_Process(void);
// Обработать данные либо статус OUT управляющей точки.
void Out0_Process(void);
// Завершить передачу EP0, применяя новый адрес после ответа хосту.
void In0_Process(void);

// Заблокировать проверенную конечную точку по запросу SET_FEATURE.
RESULT Standard_SetEndPointFeature(void);
// Запомнить разрешение удалённого пробуждения в статусе USB.
RESULT Standard_SetDeviceFeature(void);

// Вернуть текущую конфигурацию для стандартного запроса хоста.
uint8_t *Standard_GetConfiguration(uint16_t Length);
// Проверить SET_CONFIGURATION и применить выбранную конфигурацию.
RESULT Standard_SetConfiguration(void);
// Вернуть текущее альтернативное состояние интерфейса.
uint8_t *Standard_GetInterface(uint16_t Length);
// Проверить SET_INTERFACE и уведомить обработчик класса.
RESULT Standard_SetInterface(void);
// Вернуть размер или очередной фрагмент выбранного дескриптора.
uint8_t *Standard_GetDescriptorData(uint16_t Length, const ONE_DESCRIPTOR *pDesc);

// Собрать двухбайтовый статус устройства, интерфейса или конечной точки.
uint8_t *Standard_GetStatus(uint16_t Length);
// Снять разрешение пробуждения или блокировку проверенной конечной точки.
RESULT Standard_ClearFeature(void);
// Назначить адрес устройства и адреса его конечных точек.
void SetDeviceAddress(uint8_t);
// Обслужить допустимое событие, которому не нужно действие класса.
void NOP_Process(void);

extern const DEVICE_PROP Device_Property;
extern const USER_STANDARD_REQUESTS User_Standard_Requests;
extern const DEVICE Device_Table;
extern DEVICE_INFO Device_Info;

extern __IO uint16_t SaveRState;
extern __IO uint16_t SaveTState;

#endif
