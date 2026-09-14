// Модуль расчета контрольной суммы

#ifndef CRC_H
#define CRC_H

// Расчет CRC-16 ModBus
unsigned short crc16_calc(unsigned char* buf, unsigned short size);
// Расчет CRC-16 ModBus с инициализацией начального значения
unsigned short crc16_calcInit(unsigned char* buf, unsigned short size, unsigned short init);

// Вычисление контрольной суммы CRC Dallas
// in - входной буфер, num - число байт
unsigned char crc8_calc (unsigned char* in, unsigned char num);

#endif