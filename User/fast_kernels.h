// Доработка: Кряжев А., 2026
// Контракты быстрых ядер для обычной памяти, без указателей на регистры.
// ASM включается в гибриде; переносимые функции нужны эталону и проверкам на модели.
// Длина и выравнивание определены у каждой функции.
#ifndef FAST_KERNELS_H
#define FAST_KERNELS_H
#include <stdint.h>
#include <stdbool.h>

#ifdef ASM_FAST_KERNELS
// Посчитать FNV-1a ровно по десяти байтам с любым выравниванием.
uint32_t fast_fnv10(const uint8_t *p);
// Применить 16 выровненных столбцов uint16_t к 16-битному вектору.
uint16_t fast_gf2_apply(const uint16_t *m, uint16_t x);
// Проверить только [p,p+n) на FF; пустой буфер возвращает 1 без чтения.
bool fast_all_ff(const uint8_t *p, uint16_t n);
// Найти первый SLDN или хвост S/SL/SLD; без совпадения вернуть n.
uint32_t fast_sldn(const uint8_t *p, uint16_t n);
#else
// Переносимые эталоны для тестов и сборки C.
// Посчитать FNV-1a по десяти байтам.
static inline uint32_t fast_fnv10(const uint8_t *p) {
  uint32_t h = 2166136261u;
  uint8_t n = 10;
  while (n--) { h ^= *p++; h *= 16777619u; }
  return h;
}
// Проверить, что все байты области равны FF.
static inline bool fast_all_ff(const uint8_t *p, uint16_t n) {
  while (n--) if (*p++ != 255u) return 0;
  return 1;
}
// Найти сигнатуру или сохранить её незавершённый хвост.
static inline uint32_t fast_sldn(const uint8_t *p, uint16_t n) {
  uint16_t i;
  for (i = 0; i < n; i++) {
    if (p[i] != 'S') continue;
    if (n-i == 1) break;
    if (p[i+1] != 'L') continue;
    if (n-i == 2) break;
    if (p[i+2] != 'D') continue;
    if (n-i == 3 || p[i+3] == 'N') break;
  }
  return i;
}
#endif
#endif
