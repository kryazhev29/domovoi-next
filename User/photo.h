// Фото: внешние команды, камера RS485 и границы области хранения.
// Номер и размер на проводе остаются по 4 байта; ACK хранится одним байтом.
// Незавершённая запись не имеет метки формата и не попадает в очередь.
// Доработка: Кряжев А., 2026
#ifndef PHOTO_H
#define PHOTO_H
#include "includes.h"
#define PHOTO_FORMAT 0x32504850u /* PHP2: метка готового фото. */
#define PHOTO_RECORD_HEAD (sizeof(photoParam_s) + 4u + 1u)
cmdCode_e photo_start(uint8_t address);
bool photo_empty_cleanup(void);
void photo_poll(void);
void photo_connection_lost(void);
uint32_t rs485_baud(void);
void rs485_init(void);
void rs485_set_config(const uint8_t *data);
uint32_t photo_storage_end(void);
#endif
