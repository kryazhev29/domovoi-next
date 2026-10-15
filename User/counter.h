// Изменяемые байты ID поверх базы; сервер получает прежнюю логическую ячейку.
#ifndef DOMOVOI_COUNTER_H
#define DOMOVOI_COUNTER_H
#include "includes.h"
bool counter_boot(void);
bool counter_overlay(uint32_t address,uint8_t *data,uint16_t size);
int counter_begin(uint32_t address,const cellID_s *cell);
uint8_t counter_poll(uint32_t now);
// Перед другим писателем: материализация сектора и гашение прежних записей слоя.
uint8_t counter_prepare(uint32_t sector,uint32_t now);
uint16_t counter_crc_delta(void);
uint32_t counter_turns(void);
uint32_t counter_fallbacks(void);
#endif
