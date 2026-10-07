// Доработка: Кряжев А., 2026
// Форматирование ID, чисел и статуса SMS без зависимости от printf.
// Размеры и текст ответов сохраняют прежний протокол; память выделяет вызывающий.
// Второй аргумент совместим с прежними вызовами sprintf из быстрых ASM-ядер.
#ifndef USER_FORMAT_H
#define USER_FORMAT_H
#include <stdint.h>

// Сформировать соответствующее поле протокольного ответа.
uint32_t format_id(char *dst, const char *ignored, uint32_t value);
// Сформировать соответствующее поле протокольного ответа.
uint32_t format_u32(char *dst, const char *ignored, uint32_t value);
// Сформировать соответствующее поле протокольного ответа.
uint32_t format_i32(char *dst, const char *ignored, int32_t value);
// Сформировать соответствующее поле протокольного ответа.
uint32_t format_status(char *dst, const char *ignored, uint32_t signal,
                      const char *network, uint32_t connected, uint32_t server,
                      uint32_t firmware, uint32_t t1, uint32_t t2, uint32_t t3, uint32_t t4);
#endif
