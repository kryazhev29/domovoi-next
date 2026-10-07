

#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"

/**
  * @}
  */

#define CR1_UE_Set                ((uint16_t)0x2000)
#define CR1_UE_Reset              ((uint16_t)0xDFFF)

#define CR1_WAKE_Mask             ((uint16_t)0xF7FF)

#define CR1_RWU_Set               ((uint16_t)0x0002)
#define CR1_RWU_Reset             ((uint16_t)0xFFFD)
#define CR1_SBK_Set               ((uint16_t)0x0001)
#define CR1_CLEAR_Mask            ((uint16_t)0xE9F3)
#define CR2_Address_Mask          ((uint16_t)0xFFF0)

#define CR2_LINEN_Set              ((uint16_t)0x4000)
#define CR2_LINEN_Reset            ((uint16_t)0xBFFF)

#define CR2_LBDL_Mask             ((uint16_t)0xFFDF)
#define CR2_STOP_CLEAR_Mask       ((uint16_t)0xCFFF)
#define CR2_CLOCK_CLEAR_Mask      ((uint16_t)0xF0FF)

#define CR3_SCEN_Set              ((uint16_t)0x0020)
#define CR3_SCEN_Reset            ((uint16_t)0xFFDF)

#define CR3_NACK_Set              ((uint16_t)0x0010)
#define CR3_NACK_Reset            ((uint16_t)0xFFEF)

#define CR3_HDSEL_Set             ((uint16_t)0x0008)
#define CR3_HDSEL_Reset           ((uint16_t)0xFFF7)

#define CR3_IRLP_Mask             ((uint16_t)0xFFFB)
#define CR3_CLEAR_Mask            ((uint16_t)0xFCFF)

#define CR3_IREN_Set              ((uint16_t)0x0002)
#define CR3_IREN_Reset            ((uint16_t)0xFFFD)
#define GTPR_LSB_Mask             ((uint16_t)0x00FF)
#define GTPR_MSB_Mask             ((uint16_t)0xFF00)
#define IT_Mask                   ((uint16_t)0x001F)

#define CR1_OVER8_Set             ((u16)0x8000)
#define CR1_OVER8_Reset           ((u16)0x7FFF)

#define CR3_ONEBITE_Set           ((u16)0x0800)
#define CR3_ONEBITE_Reset         ((u16)0xF7FF)

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

void USART_SendData(USART_TypeDef* USARTx, uint16_t Data)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_USART_DATA(Data));

  USARTx->DR = (Data & (uint16_t)0x01FF);
}

uint16_t USART_ReceiveData(USART_TypeDef* USARTx)
{

  assert_param(IS_USART_ALL_PERIPH(USARTx));

  return (uint16_t)(USARTx->DR & (uint16_t)0x01FF);
}

FlagStatus USART_GetFlagStatus(USART_TypeDef* USARTx, uint16_t USART_FLAG)
{
  FlagStatus bitstatus = RESET;

  assert_param(IS_USART_ALL_PERIPH(USARTx));
  assert_param(IS_USART_FLAG(USART_FLAG));

  if (USART_FLAG == USART_FLAG_CTS)
  {
    assert_param(IS_USART_123_PERIPH(USARTx));
  }

  if ((USARTx->SR & USART_FLAG) != (uint16_t)RESET)
  {
    bitstatus = SET;
  }
  else
  {
    bitstatus = RESET;
  }
  return bitstatus;
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
