// Доработка: Кряжев А., 2026
// Интерфейс миллисекундных таймеров и задержек.
// Таймеры разделяются между ISR и главным циклом через атомарные снимки.
// timers_take сохраняет остаток периода, чтобы не терять время при задержке цикла.

#ifndef __TIMERS_H
#define __TIMERS_H

#include "includes.h"

#define delay_ms        timers_pause

// Запустить миллисекундный SysTick.
void timers_init(void);
// Атомарно задать счётчик с внутренним индексом 0–5.
void timers_set(unsigned char t, unsigned int vl);
// Снять счётчик с внутренним индексом 0–5.
unsigned int timers_get(unsigned char t);
// Забрать полные периоды, сохраняя остаток; period больше нуля.
unsigned int timers_take(unsigned char t, unsigned int period);
// Обслужить очередной миллисекундный тик.
void timers_handle(void);
// Пользовательский обработчик миллисекундного тика.
void timers_user_handle(void);
// Выдержать задержку с обслуживанием сторожа.
void timers_pause(const unsigned int msec);
// Снять миллисекундное время с сохранением PRIMASK.
unsigned int timers_getMillis(void);

#endif
