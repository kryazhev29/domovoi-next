// Две страницы изменений байтов 21..28 базы ID.
#ifndef DOMOVOI_COUNTER_H
#define DOMOVOI_COUNTER_H
#include "includes.h"
bool counter_boot(void);
bool counter_overlay(uint32_t address,uint8_t *data,uint16_t size);
bool counter_replaced(uint32_t address);
bool counter_pending(void);
bool counter_busy(void);
void counter_poll(uint32_t now);
bool counter_base_read(uint32_t address,uint8_t *data,uint16_t size);
bool counter_drain_begin(void);
int counter_contains(uint32_t address);
int counter_write(uint32_t address,const cellID_s *cell);
#endif
