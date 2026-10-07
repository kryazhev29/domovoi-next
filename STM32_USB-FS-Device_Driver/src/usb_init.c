// Доработка: Кряжев А., 2026
// Связь таблиц класса CDC с ядром USB.
// Дескрипторы и порядок событий сохраняют протокол CDC.
// Общие файлы также используются загрузчиком; память PMA требует особого доступа.

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
