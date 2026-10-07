// Доработка: Кряжев А., 2026
// Эталон CRC-16/MODBUS и быстрое продвижение CRC по области FF.
// CRC16 реализован измеренным ASM-ядром; здесь только продвижение по FF.
// Таблица степеней FF заменяет чтение пустой флеш; степени повторяются через 15.
#include "crc.h"


#include "fast_kernels.h"
#define gf2_apply fast_gf2_apply


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
