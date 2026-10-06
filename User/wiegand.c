
#include "wiegand.h"
#include "prof.h"

volatile uint32_t wgData = 0;
volatile uint32_t wgTime = 0;
volatile uint8_t wgBitCount = 0;
bool wgDataAvalible = false;
uint32_t wgID = 0;


// >>> прерывания -------------------------------------------------------------------
// WG26
void EXTI2_IRQHandler(void){
  // D0
  wgData <<= 1;
  /*if (++wgBitCount == 26){
    wgDataAvalible = true;
    wgID = (wgData>>1) & 0x03FFFFFF;
  }*/
  if (wgBitCount < 255) wgBitCount++; // Шум не должен снова стать кадром из 26 бит.
  wgTime = timers_getMillis();
  EXTI->PR = EXTI_Line2;
}

void EXTI3_IRQHandler(void){
  // D1
  wgData = (wgData<<1) | 1;
  /*if (++wgBitCount == 26){
    wgDataAvalible = true;
    wgID = (wgData>>1) & 0x03FFFFFF;
  }*/
  if (wgBitCount < 255) wgBitCount++; // Шум не должен снова стать кадром из 26 бит.
  wgTime = timers_getMillis();
  EXTI->PR = EXTI_Line3;
}
// <<< прерывания -------------------------------------------------------------------

// инициализация
void WG_init(void){
  EXTI_InitTypeDef EXTI_InitStruct;
  
  // прерывания на ножках D0 и D1
  EXTI_InitStruct.EXTI_Line = EXTI_Line2 | EXTI_Line3;
  EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStruct.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStruct);
  
  NVIC_InitTypeDef NVIC_InitStructure;

  NVIC_InitStructure.NVIC_IRQChannel = EXTI2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  wgData = 0;
  wgBitCount = 0;
}

// обработчик
void WG_process(void){
  if (wgDataAvalible){
    uint32_t id = wgID;
    wgDataAvalible = false; // снимаем владение до вызова обработчика
    PROF_STEP(PROF_WG_CALL, WG_readedID(id));
  }
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  uint32_t now = timers_getMillis();
  /* SIA: интервал между импульсами не больше 20 мс, поэтому тишина 25 мс -
     конец кадра (было 250 мс: четверть секунды ожидания у двери). */
  bool complete = ((now - wgTime) > 25u);
  uint32_t data = 0;
  uint8_t count = 0;
  if (complete){
    data = wgData;
    count = wgBitCount;
    wgTime = now;
    wgData = 0;
    wgBitCount = 0;
  }
  __set_PRIMASK(mask); // следующий фронт уже принадлежит следующему кадру
  if (complete){
    wgDataAvalible = (count == 26);
    if (wgDataAvalible)
      PROF_CARD_MARK();
    if (wgDataAvalible)
      wgID = (data>>1) & 0x03FFFFFF; // прежнее формирование ID; паритет требует решения Q6
  }
}

// разбор принятых данных определяется пользователем ---------------------------
__weak void WG_readedID(uint32_t id){
  
}