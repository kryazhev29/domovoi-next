
#include "wiegand.h"

uint32_t wgData = 0;
uint32_t wgTime = 0;
uint8_t wgBitCount = 0;
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
  wgBitCount++;
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
  wgBitCount++;
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
    WG_readedID(wgID);
    wgDataAvalible = false;
  }  
  
  if ((timers_getMillis() - wgTime) > 250){
    wgTime = timers_getMillis();
    if (wgBitCount == 26){
      wgDataAvalible = true;
      wgID = (wgData>>1) & 0x03FFFFFF;
    } else
      wgDataAvalible = false;
    wgData = 0;
    wgBitCount = 0;
  }
}

// разбор принятых данных определяется пользователем ---------------------------
__weak void WG_readedID(uint32_t id){
  
}