/* Доработка: Кряжев А., 2026. Explicit cooperative workspace ownership. */
#ifndef WORK_MEMORY_H
#define WORK_MEMORY_H
#include <stdint.h>
#include <stddef.h>
#include "work_slots.h"
/* Main-loop only. IRQ handlers never acquire or release this storage. */
void *work_acquire(uint32_t size, uint32_t slot);
void work_release(void *pointer, uint32_t slot);
uint16_t work_owned(void);
uint32_t work_capacity(uint32_t slot);
uint8_t work_upload_ack(uint8_t command, uint8_t channel, void *context);
#endif
