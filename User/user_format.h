/* Доработка: Кряжев А., 2026. Fixed wire formats, no printf dependency. */
#ifndef USER_FORMAT_H
#define USER_FORMAT_H
#include <stdint.h>
/* The ignored second argument preserves the original ASM sprintf call ABI. */
uint32_t format_id(char *dst, const char *ignored, uint32_t value);
uint32_t format_u32(char *dst, const char *ignored, uint32_t value);
uint32_t format_i32(char *dst, const char *ignored, int32_t value);
uint32_t format_status(char *dst, const char *ignored, uint32_t signal,
                      const char *network, uint32_t connected, uint32_t server,
                      uint32_t firmware, uint32_t t1, uint32_t t2, uint32_t t3, uint32_t t4);
#endif
