// Доработка: Кряжев А., 2026
// Интерфейс двух реле платы v5 и их импульсного режима.
// Индекс выхода проверяется владельцем внешней команды один раз на границе.
// Длительности активного уровня и паузы задаются в секундах.

#ifndef INPUTOUT_H
#define INPUTOUT_H
#include "includes.h"

#define OUTSCOUNT (uint8_t)2

typedef enum{
OUT_OFF = 0, OUT_ON, OUT_INV, OUT_IMP
}outmode_e;

// Запрос состояния выхода
Int8U out_getstate (Int8U out);

// Обработка выходов
// обрабатывается раз в 100 мс
void out_process (void);

// Активация выхода
void out_load (Int8U out, Int8U mode, Int16U activ, Int16U pause, Int8U period);

// узнаем выполняется алгоритм выхода или нет
uint8_t out_getmodestate (uint8_t out);

#endif
