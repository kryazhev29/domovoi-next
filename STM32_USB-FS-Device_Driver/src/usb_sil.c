// Доработка: Кряжев А., 2026
// Запуск USB и чтение пакетов OUT.
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
