

#ifndef __STM32F10x_RTC_H
#define __STM32F10x_RTC_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f10x.h"

/**
  * @}
  */

#define RTC_IT_OW            ((uint16_t)0x0004)
#define RTC_IT_ALR           ((uint16_t)0x0002)
#define RTC_IT_SEC           ((uint16_t)0x0001)
#define IS_RTC_IT(IT) ((((IT) & (uint16_t)0xFFF8) == 0x00) && ((IT) != 0x00))
#define IS_RTC_GET_IT(IT) (((IT) == RTC_IT_OW) || ((IT) == RTC_IT_ALR) || \
                           ((IT) == RTC_IT_SEC))
/**
  * @}
  */

#define RTC_FLAG_RTOFF       ((uint16_t)0x0020)
#define RTC_FLAG_RSF         ((uint16_t)0x0008)
#define RTC_FLAG_OW          ((uint16_t)0x0004)
#define RTC_FLAG_ALR         ((uint16_t)0x0002)
#define RTC_FLAG_SEC         ((uint16_t)0x0001)
#define IS_RTC_CLEAR_FLAG(FLAG) ((((FLAG) & (uint16_t)0xFFF0) == 0x00) && ((FLAG) != 0x00))
#define IS_RTC_GET_FLAG(FLAG) (((FLAG) == RTC_FLAG_RTOFF) || ((FLAG) == RTC_FLAG_RSF) || \
                               ((FLAG) == RTC_FLAG_OW) || ((FLAG) == RTC_FLAG_ALR) || \
                               ((FLAG) == RTC_FLAG_SEC))
#define IS_RTC_PRESCALER(PRESCALER) ((PRESCALER) <= 0xFFFFF)

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

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
