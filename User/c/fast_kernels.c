/* Доработка: Кряжев А., 2026. Переносимые C-ядра без ассемблерных вставок. */
#include "fast_kernels.h"
#include <string.h>
uint32_t fast_fnv10(const uint8_t *p)
{
  uint32_t h = 2166136261u;
  #define F(i) h = (h ^ p[i]) * 16777619u
  F(0); F(1); F(2); F(3); F(4); F(5); F(6); F(7); F(8); F(9);
  #undef F
  return h;
}
uint16_t fast_gf2_apply(const uint16_t *m, uint16_t x)
{
  uint16_t result = 0;
  for (unsigned i = 0; x; ++i, x >>= 1)
    if (x & 1u) result ^= m[i];
  return result;
}
bool fast_all_ff(const uint8_t *p, uint16_t n)
{

  if (n) do {
    if (*p++ != 255u) return false;
    if (!--n) return true;
  } while ((uintptr_t)p & 3u);
  while (n >= 4) {
    uint32_t word;
    memcpy(&word, p, sizeof word);
    if (word != 0xffffffffu) return false;
    p += 4; n -= 4;
  }
  while (n--) if (*p++ != 255u) return false;
  return true;
}
uint32_t fast_sldn(const uint8_t *p, uint16_t n)
{
  uint16_t i;
  for (i = 0; i != n; ++i) {
    if (p[i] != 'S') continue;
    if (n-i == 1) break;
    if (p[i+1] != 'L') continue;
    if (n-i == 2) break;
    if (p[i+2] != 'D') continue;
    if (n-i == 3 || p[i+3] == 'N') break;
  }
  return i;
}
