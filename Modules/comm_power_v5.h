/* Доработка: Кряжев А., 2026.
   Управляет двухступенчатым питанием SLM336 на плате v5.
   Каждый переход выдерживает полный срок, даже после пропуска главного цикла.
   До назначения GPIO выключение меняет только состояние автомата.
*/
#ifndef COMM_POWER_V5_H
#define COMM_POWER_V5_H
#include <stdint.h>
#include <stdbool.h>
enum { COMM_PWR_OFF, COMM_PWR_DISCHARGE, COMM_PWR_LEAD, COMM_PWR_RAIL,
       COMM_PWR_KEY, COMM_PWR_BOOT, COMM_PWR_READY };
/* pin — номер вывода порта C (13 или 14), high — физический уровень.
   Адаптер GPIO подключается только при замене старого владельца питания. */
typedef void (*comm_power_gpio_t)(uint8_t pin, bool high);
/* Начать включение платы v5 с полного разряда питания. */
void comm_power_v5_start(comm_power_gpio_t gpio, uint32_t now_ms);
/* Выполнить один переход после полной выдержки его срока. */
void comm_power_v5_poll(uint32_t now_ms);
/* Выдать текущую ступень питания. */
uint8_t comm_power_v5_state(void);
/* Выключить питание модема и отпустить ключ включения. */
void comm_power_v5_off(void);
#endif
