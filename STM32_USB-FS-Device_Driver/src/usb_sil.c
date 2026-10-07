// Доработка: Кряжев А., 2026
// Запуск USB и чтение пакетов OUT.
// Дескрипторы и порядок событий сохраняют протокол CDC.
// Общие файлы также используются загрузчиком; память PMA требует особого доступа.

#include "usb_lib.h"

// Запустить питание USB и установить рабочую маску прерываний.
void USB_SIL_Init(void)
{

  _SetISTR(0);
  wInterrupt_Mask = IMR_MSK;

  _SetCNTR(wInterrupt_Mask);
}

// Прочитать принятый пакет конечной точки в буфер владельца.
uint32_t USB_SIL_Read(uint8_t bEpAddr, uint8_t* pBufferPointer)
{
  uint32_t DataLength = 0;

  DataLength = GetEPRxCount(bEpAddr & 0x7F);

  PMAToUserBufferCopy(pBufferPointer, GetEPRxAddr(bEpAddr & 0x7F), DataLength);

  return DataLength;
}
