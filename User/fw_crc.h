/* STM32 CRC block is exclusively owned by the current FW Flash job. */
#ifndef FW_CRC_H
#define FW_CRC_H
#include <stdint.h>
uint32_t fw_crc_begin(void);
uint32_t fw_crc_next(uint32_t previous, uint32_t word);
#endif
