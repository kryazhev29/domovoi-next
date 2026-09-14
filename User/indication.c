// Модуль световой индикации
// Автор: Корякин Сергей

#include "indication.h"

#define LEDSCOUNT (uint8_t)2

Int8U led_shiftindex[LEDSCOUNT]; // Индекс выбора состояния светодиода из регистра
Int16U led_reg[LEDSCOUNT]; // Регистр состояния светодиодов
Int16U led_set[LEDSCOUNT]; // Регистр установки значения
Int8U leddisableloadregister = 0; // Запретить загрузку значения в регистр 


// Вызывается по прерываниям таймера раз в 100 мс
void led_fun100ms (void)
{
  Int8U i;
  Int8U k = 0;

  for (i=0; i<LEDSCOUNT; i++)
  { 
    if (((led_reg[i]>>led_shiftindex[i]) & 1) == 1)
    {
      k |= (1<<i);
    }

    if ((++led_shiftindex[i]) >= 16)
    {
      led_shiftindex[i] = 0;
      led_reg[i] = led_set[i]; // перезагружаем регистр
    }
  }

  led_set_port(k);
}

// Загрузка начального значения в регистр светодиода
void led_load (Int8U led, Int16U val)
{
  if (leddisableloadregister) return;  
  if (led >= LEDSCOUNT) return;
  if (led_set[led] != val){
    led_set[led] = val;
    led_shiftindex[led] = 0;
  }
}

// Загрузить состояние по всем светодиодам
// используется при управлении светодиодами по трем проводам
void led_set_port (unsigned char data)
{
  unsigned char i;

  for (i=0; i<LEDSCOUNT; i++)
    switch (i)
    {

      case 0: // LED1
#ifdef LED1_ON        
        if(data & 1) LED1_ON;
        else LED1_OFF;
#endif        
      break;

      case 1: // LED2
#ifdef LED2_ON        
        if(data & 2) LED2_ON;
        else LED2_OFF;
#endif        
      break;

      case 2: // LED3
#ifdef LED3_ON        
        if(data & 4) LED3_ON;
        else LED3_OFF;
#endif        
      break;     
      
      default: break;
    }
}

// Разрешить/запретить загрузку значения в регистр индикации
void led_disableloadregister (uint8_t d)
{
  leddisableloadregister = d;
}