// Доработка: Кряжев А., 2026
// Регистры USB, поля конечных точек и аппаратная таблица буферов.
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

// Задать выровненный адрес таблицы буферов в памяти USB.
void SetBTABLE(uint16_t wRegValue)
{
  _SetBTABLE(wRegValue);
}

// Задать тип конечной точки, сохранив аппаратные флаги.
void SetEPType(uint8_t bEpNum, uint16_t wType)
{
  _SetEPType(bEpNum, wType);
}

// Изменить статус IN через аппаратные переключающие биты.
void SetEPTxStatus(uint8_t bEpNum, uint16_t wState)
{
  _SetEPTxStatus(bEpNum, wState);
}

// Изменить статус OUT через аппаратные переключающие биты.
void SetEPRxStatus(uint8_t bEpNum, uint16_t wState)
{
  _SetEPRxStatus(bEpNum, wState);
}

// Разрешить передачу уже подготовленного пакета IN.
void SetEPTxValid(uint8_t bEpNum)
{
  _SetEPTxStatus(bEpNum, EP_TX_VALID);
}

// Разрешить приём следующего пакета OUT.
void SetEPRxValid(uint8_t bEpNum)
{
  _SetEPRxStatus(bEpNum, EP_RX_VALID);
}

// Снять признак ожидаемого статуса OUT управляющей точки.
void Clear_Status_Out(uint8_t bEpNum)
{
  _ClearEP_KIND(bEpNum);
}

// Вернуть приём конечной точки к последовательности DATA0.
void ClearDTOG_RX(uint8_t bEpNum)
{
  _ClearDTOG_RX(bEpNum);
}

// Вернуть передачу конечной точки к последовательности DATA0.
void ClearDTOG_TX(uint8_t bEpNum)
{
  _ClearDTOG_TX(bEpNum);
}

// Задать выровненное положение буфера IN в памяти USB.
void SetEPTxAddr(uint8_t bEpNum, uint16_t wAddr)
{
  _SetEPTxAddr(bEpNum, wAddr);
}

// Задать выровненное положение буфера OUT в памяти USB.
void SetEPRxAddr(uint8_t bEpNum, uint16_t wAddr)
{
  _SetEPRxAddr(bEpNum, wAddr);
}

// Прочитать положение буфера IN конечной точки.
uint16_t GetEPTxAddr(uint8_t bEpNum)
{
  return(_GetEPTxAddr(bEpNum));
}

// Прочитать положение буфера OUT конечной точки.
uint16_t GetEPRxAddr(uint8_t bEpNum)
{
  return(_GetEPRxAddr(bEpNum));
}

// Задать размер подготовленного пакета IN.
void SetEPTxCount(uint8_t bEpNum, uint16_t wCount)
{
  _SetEPTxCount(bEpNum, wCount);
}

// Закодировать ёмкость буфера OUT аппаратными блоками 2 или 32 байта.
void SetEPRxCount(uint8_t bEpNum, uint16_t wCount)
{
  _SetEPRxCount(bEpNum, wCount);
}

// Прочитать фактическую длину принятого пакета OUT.
uint16_t GetEPRxCount(uint8_t bEpNum)
{
  return(_GetEPRxCount(bEpNum));
}

// Поменять местами байты 16-битного поля USB.
uint16_t ByteSwap(uint16_t wSwW)
{
  uint8_t bTemp;
  uint16_t wRet;
  bTemp = (uint8_t)(wSwW & 0xff);
  wRet =  (wSwW >> 8) | ((uint16_t)bTemp << 8);
  return(wRet);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
