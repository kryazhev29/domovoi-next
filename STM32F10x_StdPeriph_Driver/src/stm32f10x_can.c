

#include "stm32f10x_can.h"
#include "stm32f10x_rcc.h"

/**
  * @}
  */

#define MCR_DBF      ((uint32_t)0x00010000)

#define TMIDxR_TXRQ  ((uint32_t)0x00000001)

#define FMR_FINIT    ((uint32_t)0x00000001)

#define INAK_TIMEOUT        ((uint32_t)0x0000FFFF)

#define SLAK_TIMEOUT        ((uint32_t)0x0000FFFF)

#define CAN_FLAGS_TSR              ((uint32_t)0x08000000)

#define CAN_FLAGS_RF1R             ((uint32_t)0x04000000)

#define CAN_FLAGS_RF0R             ((uint32_t)0x02000000)

#define CAN_FLAGS_MSR              ((uint32_t)0x01000000)

#define CAN_FLAGS_ESR              ((uint32_t)0x00F00000)

#define CAN_TXMAILBOX_0                   ((uint8_t)0x00)
#define CAN_TXMAILBOX_1                   ((uint8_t)0x01)
#define CAN_TXMAILBOX_2                   ((uint8_t)0x02)

#define CAN_MODE_MASK              ((uint32_t) 0x00000003)
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

static ITStatus CheckITStatus(uint32_t CAN_Reg, uint32_t It_Bit);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
