// Доработка: Кряжев А., 2026
// Хеш записи ID: два младших байта индекса и десять байт идентификатора.
// FNV-1a работает в uint32_t с естественным переполнением.
// Десять байт ID считает развёрнутое ядро fast_fnv10_seed.
#include <stdint.h>
#include "../fast_kernels.h"

// Вычислить FNV-1a по точному 12-байтовому ключу индекса.
uint32_t idindex_hash(uint16_t index, const uint8_t *id)
{
  uint32_t h = (2166136261u ^ (uint8_t)index) * 16777619u;
  return fast_fnv10_seed((h ^ (index >> 8)) * 16777619u, id);
}
