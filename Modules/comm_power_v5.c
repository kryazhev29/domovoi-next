/* Доработка: Кряжев А., 2026.
   Управляет двухступенчатым питанием SLM336 на плате v5.
   Миллисекундный тик выдерживает паузы независимо от главного цикла.
   До назначения GPIO выключение меняет только состояние автомата.
*/
#include "comm_power_v5.h"
#include "comm_timeouts.h"
static comm_power_gpio_t volatile power_gpio;
static volatile uint32_t power_started;
static volatile uint8_t power_state;
/* Выключить питание модема и отпустить ключ включения. */
void comm_power_v5_off(void)
{
  power_state = COMM_PWR_OFF;
  comm_power_gpio_t gpio = power_gpio;
  if (gpio) { gpio(13, false); gpio(14, true); }
}
/* Отметка первого включения использует уже существующее состояние питания. */
void comm_power_v5_cold_start(void) { power_state = COMM_PWR_COLD; }
/* Начать включение: POR/PDR без разряда только один раз, иначе полный разряд. */
void comm_power_v5_start(comm_power_gpio_t gpio, uint32_t now_ms)
{
  uint8_t first = power_state == COMM_PWR_COLD ? COMM_PWR_LEAD : COMM_PWR_DISCHARGE;
  power_gpio = gpio;
  comm_power_v5_off();
  power_started = now_ms;
  power_state = first;
}
/* Выдать текущую ступень питания. */
uint8_t comm_power_v5_state(void) { return power_state; }
uint32_t comm_power_v5_key_released(void) { return power_started; }
/* Только SysTick: GPIO и отметка времени, без AT, SPI и вызовов приложения. */
void comm_power_v5_poll(uint32_t now_ms)
{
  uint8_t state = power_state;
  if (state < COMM_PWR_DISCHARGE || state >= COMM_PWR_READY) return;
  if ((uint32_t)(now_ms - power_started) < comm_timeouts[state - 1u]) return;
  if (state >= COMM_PWR_LEAD && state <= COMM_PWR_KEY)
    power_gpio(state == COMM_PWR_LEAD ? 13 : 14, state != COMM_PWR_RAIL);
  if (state != COMM_PWR_BOOT) power_started = now_ms;
  power_state = state + 1u;
}
