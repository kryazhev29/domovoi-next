/* Доработка: Кряжев А., 2026. Same nibble polynomials as crc_asm.s. */
#include <stdint.h>
#include "crc.h"
static const uint16_t crc16_nibble[16] = {
  0x0000,0xcc01,0xd801,0x1400,0xf001,0x3c00,0x2800,0xe401,
  0xa001,0x6c00,0x7800,0xb401,0x5000,0x9c01,0x8801,0x4400
};
static const uint32_t crc32_nibble[16] = {
  0x00000000,0x1db71064,0x3b6e20c8,0x26d930ac,0x76dc4190,0x6b6b51f4,0x4db26158,0x5005713c,
  0xedb88320,0xf00f9344,0xd6d6a3e8,0xcb61b38c,0x9b64c2b0,0x86d3d2d4,0xa00ae278,0xbdbdf21c
};
static const uint32_t stm_nibble[16] = {
  0x00000000,0x04c11db7,0x09823b6e,0x0d4326d9,0x130476dc,0x17c56b6b,0x1a864db2,0x1e475005,
  0x2608edb8,0x22c9f00f,0x2f8ad6d6,0x2b4bcb61,0x350c9b64,0x31cd86d3,0x3c8ea00a,0x384fbdbd
};
unsigned short crc16_calcInit(unsigned char *buf, unsigned short size, unsigned short crc)
{
  while (size--) {
    crc ^= *buf++;
    crc = (crc >> 4) ^ crc16_nibble[crc & 15u];
    crc = (crc >> 4) ^ crc16_nibble[crc & 15u];
  }
  return crc;
}
unsigned short crc16_calc(unsigned char *buf, unsigned short size)
{ return crc16_calcInit(buf, size, 0xffffu); }
uint32_t crc32_update(uint32_t crc, const uint8_t *p, uint16_t size)
{
  while (size--) {
    crc ^= *p++;
    crc = (crc >> 4) ^ crc32_nibble[crc & 15u];
    crc = (crc >> 4) ^ crc32_nibble[crc & 15u];
  }
  return crc;
}
uint32_t CRC32(uint32_t crc, uint32_t word)
{
  crc ^= word;
  #define STEP() crc = (crc << 4) ^ stm_nibble[crc >> 28]
  STEP(); STEP(); STEP(); STEP(); STEP(); STEP(); STEP(); STEP();
  #undef STEP
  return crc;
}
