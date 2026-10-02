// Подпрограммы работы со входами и выходами
// Автор: Корякин Сергей

#include "outs.h"

Int16U out_timeactiv[OUTSCOUNT] = {0};
Int16U out_timepause[OUTSCOUNT] = {0};
Int16U out_period[OUTSCOUNT] = {0};
Int16U out_time[OUTSCOUNT]={0};
Int16U out_activpause = 0xFF; // битовая переменная (1-импульс 0-пауза)
uint8_t out_st = 0; // состояние выходов


// Перевод выхода в активное/неактивное состояние
// 0 - неактивное, 1 - активное
static void out_set (Int8U out, Int8U set)
{
  if (out >= OUTSCOUNT) return;

  switch (out)
  {
#ifdef OUT1_ON     
  case 0:
    if (set != 0)
      OUT1_ON;
    else
      OUT1_OFF;
    break;
#endif
#ifdef OUT2_ON
  case 1:
    if (set != 0)
      OUT2_ON;
    else
      OUT2_OFF;
    break;
#endif
  default: break;
  }
  
  if (set)
    out_st |= (1<<out);
  else
    out_st &= ~(1<<out);
}

// Запрос состояния выхода
Int8U out_getstate (Int8U out)
{
  Int8U o = 0;

  switch (out)
  {
#ifdef OUT1_IN   
  case 0: o = OUT1_IN ? 1 : 0; break;
#endif
#ifdef OUT2_IN
  case 1: o = OUT2_IN ? 1 : 0; break;
#endif
  default: break; 
  }
  
  return o;
}

// Обработка выходов
// обрабатывается раз в 100 мс
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
            if (out_period[i] != 255)
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
          if (out_period[i] != 255) 
            out_period[i]--;
        }
      }
    } // out_period
  }
}

// Активация выхода
void out_load (Int8U out, Int8U mode, Int16U activ, Int16U pause, Int8U period)
{
  if (out >= OUTSCOUNT) return;
  
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
    /* Протокол2 (E254/E255): время в секундах 1-255; out_process идёт раз в 100 мс,
       поэтому внутри десятые доли. Реле включается сразу, а не на следующем такте:
       проход не ждёт до секунды. */
    out_timeactiv[out] = (Int16U)(activ * 10u); 
    out_timepause[out] = (Int16U)(pause * 10u); 
    out_period[out] = period;
    out_time[out] = (Int16U)(activ * 10u); 
    out_activpause |= (1<<out); 
    if (period && activ) out_set (out, 1);
    break;
    
  default: break;
  }
}

// узнаем выполняется алгоритм выхода или нет
uint8_t out_getmodestate (uint8_t out)
{
  if (out >= OUTSCOUNT) return 0;  
  if (out_period[out]) 
    return 1;
  else 
    return 0;
}

// состояние всех выходов
uint8_t out_outstates (void)
{
  return out_st;
}