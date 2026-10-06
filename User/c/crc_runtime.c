// Доработка: Кряжев А., 2026
// Эталон CRC-16/MODBUS и быстрое продвижение CRC по области FF.
// Гибрид использует ASM CRC16, а чистый C — тот же полином 0xA001.
// Таблица степеней FF заменяет чтение пустой флеш; степени повторяются через 15.
#include "crc.h"

#ifndef ASM_CRC
// Продолжить CRC16 с переданного начального значения.
unsigned short crc16_calcInit(unsigned char *buf, unsigned short size, unsigned short init)
{
  while (size--) {
    init ^= *buf++;
    for (unsigned char bit = 0; bit < 8; ++bit)
      init = (init & 1u) ? (init >> 1) ^ 0xA001u : init >> 1;
  }
  return init;
}

// Получить CRC16 кадра с начальным значением FFFF.
unsigned short crc16_calc(unsigned char *buf, unsigned short size)
{
  return crc16_calcInit(buf, size, 0xFFFFu);
}
#endif

#ifdef ASM_FAST_KERNELS
#include "fast_kernels.h"
#define gf2_apply fast_gf2_apply
#else
// Применить матрицу CRC к установленным битам начального значения.
static unsigned short gf2_apply(const unsigned short *m, unsigned short x)
{
  unsigned short r = 0;
  for (unsigned char j = 0; x; j++, x >>= 1) if (x & 1u) r ^= m[j];
  return r;
}
#endif

/* CRC-16 ModBus после count байт 0xFF без самих байт (стёртая флеш).
   Шаг по одному байту 0xFF аффинный: r -> T*r ^ c, где T - шаг по байту 0,
   c = T*0xFF. Степень отображения считается удвоением: log2(count) шагов
   по 16x16 бит вместо count*8 сдвигов. */
#include <stdint.h>
#include "service_ff.inc"
// Продолжить CRC16 по count байтам FF без чтения пустой флеш.
unsigned short crc16_ffInit(unsigned long count, unsigned short init)
{
  unsigned char power=0;
  while (count) {
    if (count & 1u) init=gf2_apply(sc_ff_powers[power],init)^sc_ff_powers[power][16];
    count>>=1;
    if (++power==15) power=0;
  }
  return init;
}
