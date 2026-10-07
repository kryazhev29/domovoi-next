// Доработка: Кряжев А., 2026
// Копирование между обычной памятью и разреженной PMA.
// Дескрипторы и порядок событий сохраняют протокол CDC.
// Общие файлы также используются загрузчиком; память PMA требует особого доступа.

#ifndef __USB_MEM_H
#define __USB_MEM_H

// Скопировать ровно заданное число байтов в разреженную память USB.
void UserToPMABufferCopy(uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes);
// Скопировать ровно заданное число байтов из разреженной памяти USB.
void PMAToUserBufferCopy(uint8_t *pbUsrBuf, uint16_t wPMABufAddr, uint16_t wNBytes);

#endif
