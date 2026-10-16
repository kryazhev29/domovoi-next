// Модуль расчета контрольной суммы

#ifndef CRC_H
#define CRC_H

// Расчет CRC-16 ModBus
unsigned short crc16_calc(unsigned char* buf, unsigned short size);
// Расчет CRC-16 ModBus с инициализацией начального значения
unsigned short crc16_calcInit(unsigned char* buf, unsigned short size, unsigned short init);

// CRC-16 ModBus после count байт 0xFF, начиная с init (без чтения байт)
unsigned short crc16_ffInit(unsigned long count, unsigned short init);

#endif