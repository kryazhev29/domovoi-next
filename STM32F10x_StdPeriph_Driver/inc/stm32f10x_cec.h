

#ifndef __STM32F10x_CEC_H
#define __STM32F10x_CEC_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f10x.h"

typedef struct
{
  uint16_t CEC_BitTimingMode;

  uint16_t CEC_BitPeriodMode;

}CEC_InitTypeDef;

/**
  * @}
  */

#define CEC_BitTimingStdMode                    ((uint16_t)0x00)
#define CEC_BitTimingErrFreeMode                CEC_CFGR_BTEM

#define IS_CEC_BIT_TIMING_ERROR_MODE(MODE) (((MODE) == CEC_BitTimingStdMode) || \
                                            ((MODE) == CEC_BitTimingErrFreeMode))
/**
  * @}
  */

#define CEC_BitPeriodStdMode                    ((uint16_t)0x00)
#define CEC_BitPeriodFlexibleMode                CEC_CFGR_BPEM

#define IS_CEC_BIT_PERIOD_ERROR_MODE(MODE) (((MODE) == CEC_BitPeriodStdMode) || \
                                            ((MODE) == CEC_BitPeriodFlexibleMode))
/**
  * @}
  */

#define CEC_IT_TERR                              CEC_CSR_TERR
#define CEC_IT_TBTRF                             CEC_CSR_TBTRF
#define CEC_IT_RERR                              CEC_CSR_RERR
#define CEC_IT_RBTF                              CEC_CSR_RBTF
#define IS_CEC_GET_IT(IT) (((IT) == CEC_IT_TERR) || ((IT) == CEC_IT_TBTRF) || \
                           ((IT) == CEC_IT_RERR) || ((IT) == CEC_IT_RBTF))
/**
  * @}
  */

#define IS_CEC_ADDRESS(ADDRESS) ((ADDRESS) < 0x10)
/**
  * @}
  */

#define IS_CEC_PRESCALER(PRESCALER) ((PRESCALER) <= 0x3FFF)

/**
  * @}
  */

#define CEC_FLAG_BTE                            ((uint32_t)0x10010000)
#define CEC_FLAG_BPE                            ((uint32_t)0x10020000)
#define CEC_FLAG_RBTFE                          ((uint32_t)0x10040000)
#define CEC_FLAG_SBE                            ((uint32_t)0x10080000)
#define CEC_FLAG_ACKE                           ((uint32_t)0x10100000)
#define CEC_FLAG_LINE                           ((uint32_t)0x10200000)
#define CEC_FLAG_TBTFE                          ((uint32_t)0x10400000)

#define CEC_FLAG_TEOM                           ((uint32_t)0x00000002)
#define CEC_FLAG_TERR                           ((uint32_t)0x00000004)
#define CEC_FLAG_TBTRF                          ((uint32_t)0x00000008)
#define CEC_FLAG_RSOM                           ((uint32_t)0x00000010)
#define CEC_FLAG_REOM                           ((uint32_t)0x00000020)
#define CEC_FLAG_RERR                           ((uint32_t)0x00000040)
#define CEC_FLAG_RBTF                           ((uint32_t)0x00000080)

#define IS_CEC_CLEAR_FLAG(FLAG) ((((FLAG) & (uint32_t)0xFFFFFF03) == 0x00) && ((FLAG) != 0x00))

#define IS_CEC_GET_FLAG(FLAG) (((FLAG) == CEC_FLAG_BTE) || ((FLAG) == CEC_FLAG_BPE) || \
                               ((FLAG) == CEC_FLAG_RBTFE) || ((FLAG)== CEC_FLAG_SBE) || \
                               ((FLAG) == CEC_FLAG_ACKE) || ((FLAG) == CEC_FLAG_LINE) || \
                               ((FLAG) == CEC_FLAG_TBTFE) || ((FLAG) == CEC_FLAG_TEOM) || \
                               ((FLAG) == CEC_FLAG_TERR) || ((FLAG) == CEC_FLAG_TBTRF) || \
                               ((FLAG) == CEC_FLAG_RSOM) || ((FLAG) == CEC_FLAG_REOM) || \
                               ((FLAG) == CEC_FLAG_RERR) || ((FLAG) == CEC_FLAG_RBTF))

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
