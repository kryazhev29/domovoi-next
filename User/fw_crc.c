#include "includes.h"
#include "fw_crc.h"

uint32_t fw_crc_begin(void)
{
  CRC->CR = 1u;
  return 0xffffffffu;
}

uint32_t fw_crc_next(uint32_t previous, uint32_t word)
{
  (void)previous;
  CRC->DR = word;
  return CRC->DR;
}
