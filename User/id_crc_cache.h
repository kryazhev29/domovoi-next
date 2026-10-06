#ifndef DOMOVOI_ID_CRC_CACHE_H
#define DOMOVOI_ID_CRC_CACHE_H
#include <stdint.h>
#include <stdbool.h>
bool idcrc_io_start(uint32_t,const uint8_t *,uint16_t,uint32_t);
uint8_t idcrc_io_poll(uint32_t);
bool idcrc_io_active(void);
bool idcrc_write(uint32_t,uint8_t *,uint16_t);
bool idcrc_erase(uint32_t);
void idcrc_owner_released(void);
bool idcrc_background(void);
bool idcrc_get(uint32_t,uint32_t,uint16_t *);
#endif
