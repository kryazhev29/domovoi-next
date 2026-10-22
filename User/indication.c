// Доработка: Кряжев А., 2026
// Индикация платы v5: два используемых светодиода и 16-битные рисунки.
// Фаза рисунка меняется каждые 100 мс, период — 16 шагов.
// Рисунок меняется при загрузке, аппаратные уровни — на очередном шаге.

#include "indication.h"

static void led_set_port(unsigned char data);

#define LEDSCOUNT (uint8_t)2

Int8U led_shiftindex[LEDSCOUNT]; // Индекс выбора состояния светодиода из регистра
Int16U led_reg[LEDSCOUNT]; // Регистр состояния светодиодов

// Обновить фазу индикации через 100 мс.
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
    }
  }

  led_set_port(k);
}

// Загрузить рисунок выбранного светодиода.
void led_load (Int8U led, Int16U val)
{
  if (led_reg[led] != val){
    led_reg[led] = val;
    led_shiftindex[led] = 0;
  }
}

// Выставить два используемых светодиода по битовой маске.
static void led_set_port(unsigned char data)
{
  if (data & 1u) LED1_ON;
  else LED1_OFF;
  if (data & 2u) LED2_ON;
  else LED2_OFF;
}
