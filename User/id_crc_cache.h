// Доработка: Кряжев А., 2026
// Кэш CRC базы ID и согласование всех её писателей с W25.
// Кэш хранится только в ОЗУ и пересчитывается после сброса или ошибки.
// Источник записи остаётся неизменным до окончания проверки W25.
#ifndef DOMOVOI_ID_CRC_CACHE_H
#define DOMOVOI_ID_CRC_CACHE_H
#include <stdint.h>
#include <stdbool.h>
// Начинает запись с предварительным чтением прежних байтов ID.
bool idcrc_io_start(uint32_t,const uint8_t *,uint16_t,uint32_t);
// Продвигает чтение прежних байтов и проверку W25.
uint8_t idcrc_io_poll(uint32_t);
// Сообщает о незавершённой операции кэша либо W25.
bool idcrc_io_active(void);
// Сбрасывает готовность CRC после отмены или неопределённой записи.
void idcrc_unknown(void);
// Учитывает синхронную запись ID в готовой CRC.
bool idcrc_write(uint32_t,uint8_t *,uint16_t);
// Учитывает стирание сектора ID в готовой CRC.
bool idcrc_erase(uint32_t);
// Учитывает недоступность базы после возврата владельца флеша.
uint8_t idcrc_release_tail(void);
#define idcrc_owner_released() ((void)idcrc_release_tail())
// Продвигает построение CRC, освобождая флеш в том же проходе.
bool idcrc_background(void);
// Возвращает готовую CRC всей базы ID.
bool idcrc_get(uint16_t *crc);
#endif
