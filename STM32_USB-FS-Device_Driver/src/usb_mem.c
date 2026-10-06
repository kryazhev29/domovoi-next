// Доработка: Кряжев А., 2026
// Копирование между обычной памятью и разреженной PMA.
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

// Скопировать ровно заданное число байтов в разреженную память USB.
void UserToPMABufferCopy(uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes)
{
  uint16_t *pma = (uint16_t *)(wPMABufAddr * 2 + PMAAddr);
  // нечётный хвост не читает следующий байт пользовательского буфера
  while (wNBytes >= 2) {
    *pma = (uint16_t)pbUsrBuf[0] | ((uint16_t)pbUsrBuf[1] << 8);
    pma += 2; pbUsrBuf += 2; wNBytes -= 2;
  }
  if (wNBytes) *pma = *pbUsrBuf;
}

// Скопировать ровно заданное число байтов из разреженной памяти USB.
void PMAToUserBufferCopy(uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes)
{
  uint32_t *pma = (uint32_t *)(wPMABufAddr * 2 + PMAAddr);
  // записываем только запрошенные байты, в том числе при нечётной длине
  while (wNBytes >= 2) {
    uint16_t word = (uint16_t)*pma++;
    *pbUsrBuf++ = (uint8_t)word;
    *pbUsrBuf++ = (uint8_t)(word >> 8); wNBytes -= 2;
  }
  if (wNBytes) *pbUsrBuf = (uint8_t)*pma;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
