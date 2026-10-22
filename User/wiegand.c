// Доработка: Кряжев А., 2026
// Приём W26 по EXTI2/EXTI3, снимок кадра и проверка чётности.
// После тишины больше 25 мс карта передаётся за текущий проход.
// ISR владеет новым кадром; счётчик шума насыщается, исходный PRIMASK сохраняется.

#include "wiegand.h"
#include "prof.h"
#include "user_fun.h"
#include "user_format.h"

volatile uint32_t wgData = 0;
volatile uint32_t wgTime = 0;
volatile uint8_t wgBitCount = 0;
uint32_t wgID = 0;
static struct { uint32_t data; uint8_t count; } wg_frames[4];
static uint8_t wg_head, wg_count;
uint32_t wg_overflows;

// Принять бит D0 и очистить аппаратный флаг фронта.
void EXTI2_IRQHandler(void){
  wgData <<= 1;
  if (wgBitCount < 255) wgBitCount++; // Шум не должен снова стать кадром из 26 бит.
  wgTime = timers_getMillis();
  EXTI->PR = EXTI_Line2;
}

// Принять бит D1 и очистить аппаратный флаг фронта.
void EXTI3_IRQHandler(void){
  wgData = (wgData<<1) | 1;
  if (wgBitCount < 255) wgBitCount++; // Шум не должен снова стать кадром из 26 бит.
  wgTime = timers_getMillis();
  EXTI->PR = EXTI_Line3;
}

// Разрешить фронты D0/D1 и их прежние приоритеты.
void WG_init(void)
{
  const uint32_t lines = EXTI_IMR_MR2 | EXTI_IMR_MR3;
  board_exti_config(lines, 1);
  // Группировка задаётся только board_init; D0 и D1 не вытесняют друг друга.
  board_irq_enable(EXTI2_IRQn, 0x10u);
  board_irq_enable(EXTI3_IRQn, 0x10u);
  wgData = 0;
  wgBitCount = 0;
  wg_head = wg_count = 0;
}

// Запретить фронты D0/D1 при отключении настройкой; принятые кадры выключенному считывателю не принадлежат.
void WG_stop(void)
{
  EXTI->IMR &= ~(EXTI_IMR_MR2 | EXTI_IMR_MR3);
  wg_head = wg_count = 0;
}

// Проверить две группы чётности W26 без цикла и таблицы.
static bool wg_parity_valid(uint32_t data)
{
  uint32_t p = (data & 0x1fffu) | ((data >> 13) << 16);
  p ^= p >> 8; p ^= p >> 4; p ^= p >> 2; p ^= p >> 1;
  return (p & 0x10001u) == 1u;
}

// Снять кадр независимо от флеш; свободное хранилище обрабатывает его в этом проходе.
void WG_process(void){
  uint32_t data = 0;
  uint8_t count = 0;
  // В тишине IRQ не закрываем, но уже собранную очередь ниже продолжаем обслуживать.
  if (wgBitCount) {
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    uint32_t now = timers_getMillis();
    // SIA: между импульсами не больше 20 мс; тишина 25 мс завершает кадр.
    if ((uint32_t)(now - wgTime) > 25u) {
      data = wgData;
      count = wgBitCount;
      wgTime = now;
      wgData = 0;
      wgBitCount = 0;
    }
    __set_PRIMASK(mask); // следующий фронт уже принадлежит следующему кадру
  }
  if (count) {
    if (wg_count < 4u) {
      uint8_t slot = (wg_head + wg_count) & 3u;
      wg_frames[slot].data = data;
      wg_frames[slot].count = count;
      ++wg_count;
    } else ++wg_overflows;
  }
  if (!wg_count || !flash_exec_read_allowed()) return;
  data = wg_frames[wg_head].data;
  count = wg_frames[wg_head].count;
  if (count == 26 && (!WG_PARITY || wg_parity_valid(data))) {
    char id[11];
    format_id(id, (data >> 1) & 0x03FFFFFFu);
    if (!user_access_ready(id)) return;
  }
  wg_head = (wg_head + 1u) & 3u;
  --wg_count;
  if (count == 26 && (!WG_PARITY || wg_parity_valid(data))) {
    wgID = (data >> 1) & 0x03FFFFFFu;
    PROF_CARD_MARK();
    PROF_STEP(PROF_WG_CALL, WG_readedID(wgID));
  }
}
