// Доработка: Кряжев А., 2026
// Контракты быстрых ядер для обычной памяти, без указателей на регистры.
// Переносимые C-ядра; ручной ASM удалён после замера реальных операций.
// Длина и выравнивание определены у каждой функции.
#ifndef FAST_KERNELS_H
#define FAST_KERNELS_H
#include <stdint.h>
#include <stdbool.h>

// Посчитать FNV-1a ровно по десяти байтам с любым выравниванием.
uint32_t fast_fnv10(const uint8_t *p);
// FNV-1a десяти байт от заданного начального значения.
uint32_t fast_fnv10_seed(uint32_t h, const uint8_t *p);
// Применить 16 выровненных столбцов uint16_t к 16-битному вектору.
uint16_t fast_gf2_apply(const uint16_t *m, uint16_t x);
// Проверить только [p,p+n) на FF; пустой буфер возвращает 1 без чтения.
bool fast_all_ff(const uint8_t *p, uint16_t n);
// Найти первый SLDN или хвост S/SL/SLD; без совпадения вернуть n.
uint32_t fast_sldn(const uint8_t *p, uint16_t n);

#endif
