/* Доработка: Кряжев А., 2026.
   Управляет двухступенчатым питанием SLM336 на плате v5.
   Каждый переход выдерживает полный срок, даже после пропуска главного цикла.
   До назначения GPIO выключение меняет только состояние автомата.
*/
#include "comm_power_v5.h"
#include "comm_timeouts.h"
static comm_power_gpio_t power_gpio;
static uint32_t power_started;
static uint8_t power_state;
/* Выключить питание модема и отпустить ключ включения. */
void comm_power_v5_off(void)
{
  if (power_gpio) { power_gpio(13, false); power_gpio(14, true); }
  power_state = COMM_PWR_OFF;
}
/* Начать включение платы v5 с полного разряда питания. */
void comm_power_v5_start(comm_power_gpio_t gpio, uint32_t now_ms)
{
  power_gpio = gpio;
  comm_power_v5_off();
  power_started = now_ms;
  power_state = COMM_PWR_DISCHARGE;
}
/* Выдать текущую ступень питания. */
uint8_t comm_power_v5_state(void) { return power_state; }
/* Выполнить один переход после полной выдержки его срока. */
void comm_power_v5_poll(uint32_t now_ms)
{
  if (power_state < COMM_PWR_DISCHARGE || power_state >= COMM_PWR_READY) return;
  if ((uint32_t)(now_ms - power_started) < comm_timeouts[power_state - 1u]) return;
  /* Один переход за poll: пропуск вызовов не сокращает физические паузы. */
  if (power_state >= COMM_PWR_LEAD && power_state <= COMM_PWR_KEY)
    power_gpio(power_state == COMM_PWR_LEAD ? 13 : 14, power_state != COMM_PWR_RAIL);
  ++power_state;
  power_started = now_ms;
}
