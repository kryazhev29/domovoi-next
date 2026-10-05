/* Замер времени по счётчику тактов ядра (DWT CYCCNT). Доработка: Кряжев А., 2026 */
#include "includes.h"
#include "prof.h"

#define PROF_COMMAND 113u
#define PROF_SUM_SHIFT 8u                // сумма в единицах 256 тактов: uint32 хватает на 4 ч
#define PROF_STALE (72000000u * 2u)      // карта без реле дольше 2 с - отказ, не считать

typedef struct { uint32_t max, count, sum; } prof_s;
static prof_s prof[PROF_N];
uint32_t prof_card_mark;

void prof_init(void)
{
  /* В CMSIS проекта нет DWT - адреса из ARMv7-M: DEMCR.TRCENA, DWT_CYCCNT, DWT_CTRL.CYCCNTENA */
  *(volatile uint32_t*)0xE000EDFCu |= 1u << 24;
  *(volatile uint32_t*)0xE0001004u = 0;
  *(volatile uint32_t*)0xE0001000u |= 1u;
}

void prof_add(uint8_t i, uint32_t cycles)
{
  prof_s *p = &prof[i];
  if (cycles > p->max) p->max = cycles;
  p->count++;
  p->sum += cycles >> PROF_SUM_SHIFT;
}

void prof_relay_on(void)
{
  uint32_t mark = prof_card_mark;
  if (mark == 0) return;
  prof_card_mark = 0;
  uint32_t took = PROF_NOW() - mark;
  if (took < PROF_STALE) prof_add(PROF_CARD_RELAY, took);
}

/* Пакет 113: версия, затем на каждую точку max, count, sum (такты, штуки, такты/256).
 * После отправки счётчики обнуляются - каждый прогон отдельно. */
void prof_send(uint8_t uniqId)
{
  uint8_t b[8 + 3 + 4 + sizeof(prof)];
  uint16_t len = 3 + 4 + sizeof(prof), crc;
  uint32_t version = 1;

  b[0] = 'S'; b[1] = 'L'; b[2] = 'D'; b[3] = 'N';
  b[4] = len & 0xff; b[5] = len >> 8; b[6] = 0; b[7] = uniqId;
  b[8] = PROF_COMMAND;
  for (uint8_t k = 0; k < 4; k++) b[9 + k] = (uint8_t)(version >> (8 * k));
  const uint8_t *src = (const uint8_t*)prof;
  for (uint16_t k = 0; k < sizeof(prof); k++) b[13 + k] = src[k];
  crc = crc16_calc(&b[4], len + 2);
  b[len + 6] = crc & 0xff;
  b[len + 7] = crc >> 8;
  usb_send(b, len + 8);
  for (uint16_t k = 0; k < sizeof(prof); k++) ((uint8_t*)prof)[k] = 0;
}
