/* Доработка: Кряжев А., 2026.
   Управляет двухступенчатым питанием SLM336 на плате v5.
   Миллисекундный тик выдерживает паузы независимо от главного цикла.
   До назначения GPIO выключение меняет только состояние автомата.
*/
#ifndef COMM_POWER_V5_H
#define COMM_POWER_V5_H
#include <stdint.h>
#include <stdbool.h>
enum { COMM_PWR_OFF, COMM_PWR_DISCHARGE, COMM_PWR_LEAD, COMM_PWR_RAIL,
       COMM_PWR_KEY, COMM_PWR_BOOT, COMM_PWR_READY, COMM_PWR_COLD };
/* pin — номер вывода порта C (13 или 14), high — физический уровень.
   Адаптер GPIO подключается только при замене старого владельца питания. */
typedef void (*comm_power_gpio_t)(uint8_t pin, bool high);
/* Первое включение после POR/PDR не требует разряда; повторное требует. */
void comm_power_v5_cold_start(void);
/* Начать включение платы v5, потребив отметку холодного старта. */
void comm_power_v5_start(comm_power_gpio_t gpio, uint32_t now_ms);
/* Вызывается только миллисекундным SysTick, независимо от главного цикла. */
void comm_power_v5_poll(uint32_t now_ms);
/* Выдать текущую ступень питания. */
uint8_t comm_power_v5_state(void);
/* Отметка окончания питания; читать после состояния READY. */
uint32_t comm_power_v5_ready_at(void);
/* Выключить питание модема и отпустить ключ включения. */
void comm_power_v5_off(void);
#endif
