// Две страницы изменений байтов 21..28 базы ID.
#ifndef DOMOVOI_COUNTER_H
#define DOMOVOI_COUNTER_H
#include "includes.h"
bool counter_boot(void);
bool counter_overlay(uint32_t address,uint8_t *data,uint16_t size);
bool counter_flush(void);
int counter_write(uint32_t address,const cellID_s *cell);
#endif
