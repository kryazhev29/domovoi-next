

#ifndef __STM32F10x_FLASH_H
#define __STM32F10x_FLASH_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "stm32f10x.h"

typedef enum
{
  FLASH_BUSY = 1,
  FLASH_ERROR_PG,
  FLASH_ERROR_WRP,
  FLASH_COMPLETE,
  FLASH_TIMEOUT
}FLASH_Status;

/**
  * @}
  */

#define FLASH_Latency_0                ((uint32_t)0x00000000)
#define FLASH_Latency_1                ((uint32_t)0x00000001)
#define FLASH_Latency_2                ((uint32_t)0x00000002)
#define IS_FLASH_LATENCY(LATENCY) (((LATENCY) == FLASH_Latency_0) || \
                                   ((LATENCY) == FLASH_Latency_1) || \
                                   ((LATENCY) == FLASH_Latency_2))
/**
  * @}
  */

#define FLASH_HalfCycleAccess_Enable   ((uint32_t)0x00000008)
#define FLASH_HalfCycleAccess_Disable  ((uint32_t)0x00000000)
#define IS_FLASH_HALFCYCLEACCESS_STATE(STATE) (((STATE) == FLASH_HalfCycleAccess_Enable) || \
                                               ((STATE) == FLASH_HalfCycleAccess_Disable))
/**
  * @}
  */

#define FLASH_PrefetchBuffer_Enable    ((uint32_t)0x00000010)
#define FLASH_PrefetchBuffer_Disable   ((uint32_t)0x00000000)
#define IS_FLASH_PREFETCHBUFFER_STATE(STATE) (((STATE) == FLASH_PrefetchBuffer_Enable) || \
                                              ((STATE) == FLASH_PrefetchBuffer_Disable))
/**
  * @}
  */

#define FLASH_WRProt_Pages0to3         ((uint32_t)0x00000001)
#define FLASH_WRProt_Pages4to7         ((uint32_t)0x00000002)
#define FLASH_WRProt_Pages8to11        ((uint32_t)0x00000004)
#define FLASH_WRProt_Pages12to15       ((uint32_t)0x00000008)
#define FLASH_WRProt_Pages16to19       ((uint32_t)0x00000010)
#define FLASH_WRProt_Pages20to23       ((uint32_t)0x00000020)
#define FLASH_WRProt_Pages24to27       ((uint32_t)0x00000040)
#define FLASH_WRProt_Pages28to31       ((uint32_t)0x00000080)

#define FLASH_WRProt_Pages32to35       ((uint32_t)0x00000100)
#define FLASH_WRProt_Pages36to39       ((uint32_t)0x00000200)
#define FLASH_WRProt_Pages40to43       ((uint32_t)0x00000400)
#define FLASH_WRProt_Pages44to47       ((uint32_t)0x00000800)
#define FLASH_WRProt_Pages48to51       ((uint32_t)0x00001000)
#define FLASH_WRProt_Pages52to55       ((uint32_t)0x00002000)
#define FLASH_WRProt_Pages56to59       ((uint32_t)0x00004000)
#define FLASH_WRProt_Pages60to63       ((uint32_t)0x00008000)
#define FLASH_WRProt_Pages64to67       ((uint32_t)0x00010000)
#define FLASH_WRProt_Pages68to71       ((uint32_t)0x00020000)
#define FLASH_WRProt_Pages72to75       ((uint32_t)0x00040000)
#define FLASH_WRProt_Pages76to79       ((uint32_t)0x00080000)
#define FLASH_WRProt_Pages80to83       ((uint32_t)0x00100000)
#define FLASH_WRProt_Pages84to87       ((uint32_t)0x00200000)
#define FLASH_WRProt_Pages88to91       ((uint32_t)0x00400000)
#define FLASH_WRProt_Pages92to95       ((uint32_t)0x00800000)
#define FLASH_WRProt_Pages96to99       ((uint32_t)0x01000000)
#define FLASH_WRProt_Pages100to103     ((uint32_t)0x02000000)
#define FLASH_WRProt_Pages104to107     ((uint32_t)0x04000000)
#define FLASH_WRProt_Pages108to111     ((uint32_t)0x08000000)
#define FLASH_WRProt_Pages112to115     ((uint32_t)0x10000000)
#define FLASH_WRProt_Pages116to119     ((uint32_t)0x20000000)
#define FLASH_WRProt_Pages120to123     ((uint32_t)0x40000000)
#define FLASH_WRProt_Pages124to127     ((uint32_t)0x80000000)

#define FLASH_WRProt_Pages0to1         ((uint32_t)0x00000001)

#define FLASH_WRProt_Pages2to3         ((uint32_t)0x00000002)

#define FLASH_WRProt_Pages4to5         ((uint32_t)0x00000004)

#define FLASH_WRProt_Pages6to7         ((uint32_t)0x00000008)

#define FLASH_WRProt_Pages8to9         ((uint32_t)0x00000010)

#define FLASH_WRProt_Pages10to11       ((uint32_t)0x00000020)

#define FLASH_WRProt_Pages12to13       ((uint32_t)0x00000040)

#define FLASH_WRProt_Pages14to15       ((uint32_t)0x00000080)

#define FLASH_WRProt_Pages16to17       ((uint32_t)0x00000100)

#define FLASH_WRProt_Pages18to19       ((uint32_t)0x00000200)

#define FLASH_WRProt_Pages20to21       ((uint32_t)0x00000400)

#define FLASH_WRProt_Pages22to23       ((uint32_t)0x00000800)

#define FLASH_WRProt_Pages24to25       ((uint32_t)0x00001000)

#define FLASH_WRProt_Pages26to27       ((uint32_t)0x00002000)

#define FLASH_WRProt_Pages28to29       ((uint32_t)0x00004000)

#define FLASH_WRProt_Pages30to31       ((uint32_t)0x00008000)

#define FLASH_WRProt_Pages32to33       ((uint32_t)0x00010000)

#define FLASH_WRProt_Pages34to35       ((uint32_t)0x00020000)

#define FLASH_WRProt_Pages36to37       ((uint32_t)0x00040000)

#define FLASH_WRProt_Pages38to39       ((uint32_t)0x00080000)

#define FLASH_WRProt_Pages40to41       ((uint32_t)0x00100000)

#define FLASH_WRProt_Pages42to43       ((uint32_t)0x00200000)

#define FLASH_WRProt_Pages44to45       ((uint32_t)0x00400000)

#define FLASH_WRProt_Pages46to47       ((uint32_t)0x00800000)

#define FLASH_WRProt_Pages48to49       ((uint32_t)0x01000000)

#define FLASH_WRProt_Pages50to51       ((uint32_t)0x02000000)

#define FLASH_WRProt_Pages52to53       ((uint32_t)0x04000000)

#define FLASH_WRProt_Pages54to55       ((uint32_t)0x08000000)

#define FLASH_WRProt_Pages56to57       ((uint32_t)0x10000000)

#define FLASH_WRProt_Pages58to59       ((uint32_t)0x20000000)

#define FLASH_WRProt_Pages60to61       ((uint32_t)0x40000000)

#define FLASH_WRProt_Pages62to127      ((uint32_t)0x80000000)
#define FLASH_WRProt_Pages62to255      ((uint32_t)0x80000000)
#define FLASH_WRProt_Pages62to511      ((uint32_t)0x80000000)

#define FLASH_WRProt_AllPages          ((uint32_t)0xFFFFFFFF)

#define IS_FLASH_WRPROT_PAGE(PAGE) (((PAGE) != 0x00000000))

#define IS_FLASH_ADDRESS(ADDRESS) (((ADDRESS) >= 0x08000000) && ((ADDRESS) < 0x080FFFFF))

#define IS_OB_DATA_ADDRESS(ADDRESS) (((ADDRESS) == 0x1FFFF804) || ((ADDRESS) == 0x1FFFF806))

/**
  * @}
  */

#define OB_IWDG_SW                     ((uint16_t)0x0001)
#define OB_IWDG_HW                     ((uint16_t)0x0000)
#define IS_OB_IWDG_SOURCE(SOURCE) (((SOURCE) == OB_IWDG_SW) || ((SOURCE) == OB_IWDG_HW))

/**
  * @}
  */

#define OB_STOP_NoRST                  ((uint16_t)0x0002)
#define OB_STOP_RST                    ((uint16_t)0x0000)
#define IS_OB_STOP_SOURCE(SOURCE) (((SOURCE) == OB_STOP_NoRST) || ((SOURCE) == OB_STOP_RST))

/**
  * @}
  */

#define OB_STDBY_NoRST                 ((uint16_t)0x0004)
#define OB_STDBY_RST                   ((uint16_t)0x0000)
#define IS_OB_STDBY_SOURCE(SOURCE) (((SOURCE) == OB_STDBY_NoRST) || ((SOURCE) == OB_STDBY_RST))

#ifdef STM32F10X_XL
/**
  * @}
  */

#define FLASH_BOOT_Bank1  ((uint16_t)0x0000)

#define FLASH_BOOT_Bank2  ((uint16_t)0x0001)

#define IS_FLASH_BOOT(BOOT) (((BOOT) == FLASH_BOOT_Bank1) || ((BOOT) == FLASH_BOOT_Bank2))
#endif
/**
  * @}
  */

#ifdef STM32F10X_XL
#define FLASH_IT_BANK2_ERROR                 ((uint32_t)0x80000400)
#define FLASH_IT_BANK2_EOP                   ((uint32_t)0x80001000)

#define FLASH_IT_BANK1_ERROR                 FLASH_IT_ERROR
#define FLASH_IT_BANK1_EOP                   FLASH_IT_EOP

#define FLASH_IT_ERROR                 ((uint32_t)0x00000400)
#define FLASH_IT_EOP                   ((uint32_t)0x00001000)
#define IS_FLASH_IT(IT) ((((IT) & (uint32_t)0x7FFFEBFF) == 0x00000000) && (((IT) != 0x00000000)))
#else
#define FLASH_IT_ERROR                 ((uint32_t)0x00000400)
#define FLASH_IT_EOP                   ((uint32_t)0x00001000)
#define FLASH_IT_BANK1_ERROR           FLASH_IT_ERROR
#define FLASH_IT_BANK1_EOP             FLASH_IT_EOP

#define IS_FLASH_IT(IT) ((((IT) & (uint32_t)0xFFFFEBFF) == 0x00000000) && (((IT) != 0x00000000)))
#endif

/**
  * @}
  */

#ifdef STM32F10X_XL
#define FLASH_FLAG_BANK2_BSY                 ((uint32_t)0x80000001)
#define FLASH_FLAG_BANK2_EOP                 ((uint32_t)0x80000020)
#define FLASH_FLAG_BANK2_PGERR               ((uint32_t)0x80000004)
#define FLASH_FLAG_BANK2_WRPRTERR            ((uint32_t)0x80000010)

#define FLASH_FLAG_BANK1_BSY                 FLASH_FLAG_BSY
#define FLASH_FLAG_BANK1_EOP                 FLASH_FLAG_EOP
#define FLASH_FLAG_BANK1_PGERR               FLASH_FLAG_PGERR
#define FLASH_FLAG_BANK1_WRPRTERR            FLASH_FLAG_WRPRTERR

#define FLASH_FLAG_BSY                 ((uint32_t)0x00000001)
#define FLASH_FLAG_EOP                 ((uint32_t)0x00000020)
#define FLASH_FLAG_PGERR               ((uint32_t)0x00000004)
#define FLASH_FLAG_WRPRTERR            ((uint32_t)0x00000010)
#define FLASH_FLAG_OPTERR              ((uint32_t)0x00000001)

#define IS_FLASH_CLEAR_FLAG(FLAG) ((((FLAG) & (uint32_t)0x7FFFFFCA) == 0x00000000) && ((FLAG) != 0x00000000))
#define IS_FLASH_GET_FLAG(FLAG)  (((FLAG) == FLASH_FLAG_BSY) || ((FLAG) == FLASH_FLAG_EOP) || \
                                  ((FLAG) == FLASH_FLAG_PGERR) || ((FLAG) == FLASH_FLAG_WRPRTERR) || \
                                  ((FLAG) == FLASH_FLAG_OPTERR)|| \
                                  ((FLAG) == FLASH_FLAG_BANK1_BSY) || ((FLAG) == FLASH_FLAG_BANK1_EOP) || \
                                  ((FLAG) == FLASH_FLAG_BANK1_PGERR) || ((FLAG) == FLASH_FLAG_BANK1_WRPRTERR) || \
                                  ((FLAG) == FLASH_FLAG_BANK2_BSY) || ((FLAG) == FLASH_FLAG_BANK2_EOP) || \
                                  ((FLAG) == FLASH_FLAG_BANK2_PGERR) || ((FLAG) == FLASH_FLAG_BANK2_WRPRTERR))
#else
#define FLASH_FLAG_BSY                 ((uint32_t)0x00000001)
#define FLASH_FLAG_EOP                 ((uint32_t)0x00000020)
#define FLASH_FLAG_PGERR               ((uint32_t)0x00000004)
#define FLASH_FLAG_WRPRTERR            ((uint32_t)0x00000010)
#define FLASH_FLAG_OPTERR              ((uint32_t)0x00000001)

#define FLASH_FLAG_BANK1_BSY                 FLASH_FLAG_BSY
#define FLASH_FLAG_BANK1_EOP                 FLASH_FLAG_EOP
#define FLASH_FLAG_BANK1_PGERR               FLASH_FLAG_PGERR
#define FLASH_FLAG_BANK1_WRPRTERR            FLASH_FLAG_WRPRTERR

#define IS_FLASH_CLEAR_FLAG(FLAG) ((((FLAG) & (uint32_t)0xFFFFFFCA) == 0x00000000) && ((FLAG) != 0x00000000))
#define IS_FLASH_GET_FLAG(FLAG)  (((FLAG) == FLASH_FLAG_BSY) || ((FLAG) == FLASH_FLAG_EOP) || \
                                  ((FLAG) == FLASH_FLAG_PGERR) || ((FLAG) == FLASH_FLAG_WRPRTERR) || \
								  ((FLAG) == FLASH_FLAG_BANK1_BSY) || ((FLAG) == FLASH_FLAG_BANK1_EOP) || \
                                  ((FLAG) == FLASH_FLAG_BANK1_PGERR) || ((FLAG) == FLASH_FLAG_BANK1_WRPRTERR) || \
                                  ((FLAG) == FLASH_FLAG_OPTERR))
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

void FLASH_Unlock(void);
void FLASH_Lock(void);
FLASH_Status FLASH_ErasePage(uint32_t Page_Address);

FLASH_Status FLASH_ProgramWord(uint32_t Address, uint32_t Data);

void FLASH_ClearFlag(uint32_t FLASH_FLAG);

FLASH_Status FLASH_WaitForLastOperation(uint32_t Timeout);

FLASH_Status FLASH_GetBank1Status(void);
FLASH_Status FLASH_WaitForLastBank1Operation(uint32_t Timeout);

#ifdef STM32F10X_XL

FLASH_Status FLASH_GetBank2Status(void);
FLASH_Status FLASH_WaitForLastBank2Operation(uint32_t Timeout);

#endif

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
