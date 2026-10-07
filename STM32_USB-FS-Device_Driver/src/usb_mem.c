// Доработка: Кряжев А., 2026
// Копирование между обычной памятью и разреженной PMA.
// Дескрипторы и порядок событий сохраняют протокол CDC.
// Общие файлы также используются загрузчиком; память PMA требует особого доступа.

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
