// Доработка: Кряжев А., 2026
#ifndef PHOTO_H
#define PHOTO_H
#include "includes.h"
cmdCode_e photo_start(uint8_t address);
cmdCode_e photo_request(dataChanel_e channel, uint8_t id, const uint8_t *args, uint8_t size);
void photo_poll(void);
uint32_t rs485_baud(void);
void rs485_init(void);
void rs485_set_config(const uint8_t *data);
uint32_t photo_storage_end(void);
uint8_t photo_recover(void);
#endif
