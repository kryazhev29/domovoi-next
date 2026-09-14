// Подпрограммы работы со входами и выходами
// Автор: Корякин Сергей

#include "includes.h"

#ifndef INPUTOUT_H
#define INPUTOUT_H

#define OUTSCOUNT               (uint8_t)2      
#define INPUTFILTERSIZE         (uint8_t)10
#define EXTINPUTSCOUNT          (uint8_t)0

typedef enum{
OUT_OFF = 0, OUT_ON, OUT_INV, OUT_IMP
}outmode_e;

// Перевод вывода в активное/неактивное состояние
// 0 - неактивное, 1 - активное
static void out_set (Int8U out, Int8U set);

// Запрос состояния выхода
Int8U out_getstate (Int8U out);

// Обработка выходов
// обрабатывается раз в 100 мс
void out_process (void);

// Активация выхода
void out_load (Int8U out, Int8U mode, Int16U activ, Int16U pause, Int8U period);

// узнаем выполняется алгоритм выхода или нет
uint8_t out_getmodestate (uint8_t out);

// состояние всех выходов
uint8_t out_outstates (void);

#endif