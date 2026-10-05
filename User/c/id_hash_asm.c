/* Доработка: Кряжев А., 2026. Exact 12-byte FNV-1a contract. */
#include <stdint.h>
uint32_t idindex_hash(uint16_t index, const uint8_t *id)
{
  uint32_t h = (2166136261u ^ (uint8_t)index) * 16777619u;
  h = (h ^ (index >> 8)) * 16777619u;
  #define F(i) h = (h ^ id[i]) * 16777619u
  F(0); F(1); F(2); F(3); F(4); F(5); F(6); F(7); F(8); F(9);
  #undef F
  return h;
}
