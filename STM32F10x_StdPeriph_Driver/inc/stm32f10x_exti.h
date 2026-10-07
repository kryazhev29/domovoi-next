

#ifndef __STM32F10x_EXTI_H
#define __STM32F10x_EXTI_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f10x.h"

typedef enum
{
  EXTI_Mode_Interrupt = 0x00,
  EXTI_Mode_Event = 0x04
}EXTIMode_TypeDef;

#define IS_EXTI_MODE(MODE) (((MODE) == EXTI_Mode_Interrupt) || ((MODE) == EXTI_Mode_Event))

typedef enum
{
  EXTI_Trigger_Rising = 0x08,
  EXTI_Trigger_Falling = 0x0C,
  EXTI_Trigger_Rising_Falling = 0x10
}EXTITrigger_TypeDef;

#define IS_EXTI_TRIGGER(TRIGGER) (((TRIGGER) == EXTI_Trigger_Rising) || \
                                  ((TRIGGER) == EXTI_Trigger_Falling) || \
                                  ((TRIGGER) == EXTI_Trigger_Rising_Falling))

typedef struct
{
  uint32_t EXTI_Line;

  EXTIMode_TypeDef EXTI_Mode;

  EXTITrigger_TypeDef EXTI_Trigger;

  FunctionalState EXTI_LineCmd;

}EXTI_InitTypeDef;

/**
  * @}
  */

#define EXTI_Line0       ((uint32_t)0x00001)
#define EXTI_Line1       ((uint32_t)0x00002)
#define EXTI_Line2       ((uint32_t)0x00004)
#define EXTI_Line3       ((uint32_t)0x00008)
#define EXTI_Line4       ((uint32_t)0x00010)
#define EXTI_Line5       ((uint32_t)0x00020)
#define EXTI_Line6       ((uint32_t)0x00040)
#define EXTI_Line7       ((uint32_t)0x00080)
#define EXTI_Line8       ((uint32_t)0x00100)
#define EXTI_Line9       ((uint32_t)0x00200)
#define EXTI_Line10      ((uint32_t)0x00400)
#define EXTI_Line11      ((uint32_t)0x00800)
#define EXTI_Line12      ((uint32_t)0x01000)
#define EXTI_Line13      ((uint32_t)0x02000)
#define EXTI_Line14      ((uint32_t)0x04000)
#define EXTI_Line15      ((uint32_t)0x08000)
#define EXTI_Line16      ((uint32_t)0x10000)
#define EXTI_Line17      ((uint32_t)0x20000)
#define EXTI_Line18      ((uint32_t)0x40000)

#define EXTI_Line19      ((uint32_t)0x80000)

#define IS_EXTI_LINE(LINE) ((((LINE) & (uint32_t)0xFFF00000) == 0x00) && ((LINE) != (uint16_t)0x00))
#define IS_GET_EXTI_LINE(LINE) (((LINE) == EXTI_Line0) || ((LINE) == EXTI_Line1) || \
                            ((LINE) == EXTI_Line2) || ((LINE) == EXTI_Line3) || \
                            ((LINE) == EXTI_Line4) || ((LINE) == EXTI_Line5) || \
                            ((LINE) == EXTI_Line6) || ((LINE) == EXTI_Line7) || \
                            ((LINE) == EXTI_Line8) || ((LINE) == EXTI_Line9) || \
                            ((LINE) == EXTI_Line10) || ((LINE) == EXTI_Line11) || \
                            ((LINE) == EXTI_Line12) || ((LINE) == EXTI_Line13) || \
                            ((LINE) == EXTI_Line14) || ((LINE) == EXTI_Line15) || \
                            ((LINE) == EXTI_Line16) || ((LINE) == EXTI_Line17) || \
                            ((LINE) == EXTI_Line18) || ((LINE) == EXTI_Line19))

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

void EXTI_Init(EXTI_InitTypeDef* EXTI_InitStruct);

void EXTI_ClearITPendingBit(uint32_t EXTI_Line);

#ifdef __cplusplus
}
#endif

#endif
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
