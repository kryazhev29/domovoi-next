// Доработка: Кряжев А., 2026
// Два реле платы: уровни, импульсы и паузы с шагом 100 мс.
// Индекс выхода проверяется владельцем команды до вызова этого модуля.
// Внутренние функции используют только индексы 0 и 1.

#include "outs.h"
#include "prof.h"

Int16U out_timeactiv[OUTSCOUNT] = {0};
Int16U out_timepause[OUTSCOUNT] = {0};
Int16U out_period[OUTSCOUNT] = {0};
Int16U out_time[OUTSCOUNT]={0};
Int16U out_activpause = 0xFF; // битовая переменная (1-импульс 0-пауза)
uint8_t out_st = 0; // состояние выходов

// Изменить аппаратный уровень проверенного выхода.
static void out_set (Int8U out, Int8U set)
{

  switch (out)
  {
  case 0:
    if (set != 0)
      OUT1_ON;
    else
      OUT1_OFF;
    break;
  case 1:
    if (set != 0)
      OUT2_ON;
    else
      OUT2_OFF;
    break;
  default: break;
  }

  if (set && !(out_st & (1<<out))) prof_relay_on();
  if (set)
    out_st |= (1<<out);
  else
    out_st &= ~(1<<out);
}

// Прочитать аппаратное состояние выхода.
Int8U out_getstate (Int8U out)
{
  Int8U o = 0;

  switch (out)
  {
  case 0: o = OUT1_IN ? 1 : 0; break;
  case 1: o = OUT2_IN ? 1 : 0; break;
  default: break;
  }

  return o;
}

// Обслужить импульсы и паузы каждые 100 мс.
void out_process (void)
{
  Int8U i;

  for (i=0; i<OUTSCOUNT; i++)
  {
    if (out_period[i]) // идет счет импульсов (повтор)
    {
      if (out_time[i]) // счетчик активного сост. и паузы
      {
        out_time[i]--;
        out_set (i, (out_activpause>>i) & 1);
      }

      if (out_time[i] == 0)
      {
        if (out_activpause & (1<<i)) // был активный уровень, загружаем паузу
        {
          if (out_timepause[i] == 0) // формирование длинного импульса без паузы
          {
            out_time[i] = out_timeactiv[i];
            out_period[i]--;
            if (out_period[i] == 0)
              out_set (i, 0); // закончился повтор - выключаем выход
          }
          else
          {
            out_activpause &= ~(1<<i);
            out_time[i] = out_timepause[i];
          }
        }
        else // загружаем активный уровень и проверяем число повторов
        {
          out_activpause |= (1<<i);
          out_time[i] = out_timeactiv[i];
          out_period[i]--;
        }
      }
    }
  }
}

// Загрузить режим для уже проверенного индекса выхода.
void out_load (Int8U out, Int8U mode, Int16U activ, Int16U pause, Int8U period)
{

  switch (mode)
  {
  case OUT_OFF: // выключить
  case OUT_ON: // включить
    out_period[out] = 0;
    out_set (out, mode);
    break;

  case OUT_INV: // инвертировать
    out_period[out] = 0;
    out_set (out, !out_getstate(out));
    break;

  case OUT_IMP: // импульс
    // Пустой импульс останавливает автомат, сохраняя текущий уровень выхода.
    out_period[out] = 0;
    if (!period || !activ) break;
    /* Протокол2 (E254/E255): время в секундах 1-255; out_process идёт раз в 100 мс,
       поэтому внутри десятые доли. Реле включается сразу, а не на следующем такте:
       проход не ждёт до секунды. */
    out_timeactiv[out] = (Int16U)(activ * 10u);
    out_timepause[out] = (Int16U)(pause * 10u);
    out_period[out] = period;
    out_time[out] = (Int16U)(activ * 10u);
    out_activpause |= (1<<out);
    out_set (out, 1);
    break;

  default: break;
  }
}

// Проверить активность алгоритма выбранного выхода.
uint8_t out_getmodestate (uint8_t out)
{
  return out_period[out] != 0;
}
