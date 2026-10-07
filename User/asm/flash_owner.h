/* Коды владельцев байта flash_owner.
   0 — свободно. Бит 7 запрещает чтение таблиц на время операции.
   Общие константы для ассемблера IAR и C.
   Захват и освобождение владельца используют один и тот же код. */
#ifndef FLASH_OWNER_H
#define FLASH_OWNER_H

#define FO_READ_BLOCK   0x80
#define FO_FWSERVICE    0x83
#define FO_UPLOAD       0x84
#define FO_TABLE        0x87
#define FO_PHOTO        0x08
#define FO_FWBLOCK      0x09
#define FO_LOG          0x0A

#endif
