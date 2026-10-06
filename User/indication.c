// ƒоработка:  р€жев ј., 2026
// »ндикаци€ платы v5: два используемых светодиода и 16-битные рисунки.
// —двиг рисунка выполн€етс€ каждые 100 мс, перезагрузка Ч после 16 шагов.
// «агрузка нового рисунка может быть временно запрещена владельцем.

#include "indication.h"

#define LEDSCOUNT (uint8_t)2

Int8U led_shiftindex[LEDSCOUNT]; // »ндекс выбора состо€ни€ светодиода из регистра
Int16U led_reg[LEDSCOUNT]; // –егистр состо€ни€ светодиодов
Int16U led_set[LEDSCOUNT]; // –егистр установки значени€
Int8U leddisableloadregister = 0; // «апретить загрузку значени€ в регистр


// ¬ызываетс€ по прерывани€м таймера раз в 100 мс
// ќбновить фазу индикации через 100 мс.
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

// «агрузка начального значени€ в регистр светодиода
// «агрузить рисунок выбранного светодиода.
void led_load (Int8U led, Int16U val)
{
  if (leddisableloadregister) return;
  if (led >= LEDSCOUNT) return;
  if (led_set[led] != val){
    led_set[led] = val;
    led_shiftindex[led] = 0;
  }
}

// «агрузить состо€ние по всем светодиодам
// используетс€ при управлении светодиодами по трем проводам
// ¬ыставить уровни используемых светодиодов.
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

// –азрешить/запретить загрузку значени€ в регистр индикации
// –азрешить или запретить перезагрузку рисунка.
void led_disableloadregister (uint8_t d)
{
  leddisableloadregister = d;
}