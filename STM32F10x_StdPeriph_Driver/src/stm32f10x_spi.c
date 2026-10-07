

#include "stm32f10x_spi.h"
#include "stm32f10x_rcc.h"

/**
  * @}
  */

#define CR1_SPE_Set          ((uint16_t)0x0040)
#define CR1_SPE_Reset        ((uint16_t)0xFFBF)

#define I2SCFGR_I2SE_Set     ((uint16_t)0x0400)
#define I2SCFGR_I2SE_Reset   ((uint16_t)0xFBFF)

#define CR1_CRCNext_Set      ((uint16_t)0x1000)

#define CR1_CRCEN_Set        ((uint16_t)0x2000)
#define CR1_CRCEN_Reset      ((uint16_t)0xDFFF)

#define CR2_SSOE_Set         ((uint16_t)0x0004)
#define CR2_SSOE_Reset       ((uint16_t)0xFFFB)

#define CR1_CLEAR_Mask       ((uint16_t)0x3040)
#define I2SCFGR_CLEAR_Mask   ((uint16_t)0xF040)

#define SPI_Mode_Select      ((uint16_t)0xF7FF)
#define I2S_Mode_Select      ((uint16_t)0x0800)

#define I2S2_CLOCK_SRC       ((uint32_t)(0x00020000))
#define I2S3_CLOCK_SRC       ((uint32_t)(0x00040000))
#define I2S_MUL_MASK         ((uint32_t)(0x0000F000))
#define I2S_DIV_MASK         ((uint32_t)(0x000000F0))

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

void SPI_I2S_DeInit(SPI_TypeDef* SPIx)
{

  assert_param(IS_SPI_ALL_PERIPH(SPIx));

  if (SPIx == SPI1)
  {

    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1, ENABLE);

    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1, DISABLE);
  }
  else if (SPIx == SPI2)
  {

    RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI2, ENABLE);

    RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI2, DISABLE);
  }
  else
  {
    if (SPIx == SPI3)
    {

      RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI3, ENABLE);

      RCC_APB1PeriphResetCmd(RCC_APB1Periph_SPI3, DISABLE);
    }
  }
}

void SPI_Init(SPI_TypeDef* SPIx, SPI_InitTypeDef* SPI_InitStruct)
{
  uint16_t tmpreg = 0;

  assert_param(IS_SPI_ALL_PERIPH(SPIx));

  assert_param(IS_SPI_DIRECTION_MODE(SPI_InitStruct->SPI_Direction));
  assert_param(IS_SPI_MODE(SPI_InitStruct->SPI_Mode));
  assert_param(IS_SPI_DATASIZE(SPI_InitStruct->SPI_DataSize));
  assert_param(IS_SPI_CPOL(SPI_InitStruct->SPI_CPOL));
  assert_param(IS_SPI_CPHA(SPI_InitStruct->SPI_CPHA));
  assert_param(IS_SPI_NSS(SPI_InitStruct->SPI_NSS));
  assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_InitStruct->SPI_BaudRatePrescaler));
  assert_param(IS_SPI_FIRST_BIT(SPI_InitStruct->SPI_FirstBit));
  assert_param(IS_SPI_CRC_POLYNOMIAL(SPI_InitStruct->SPI_CRCPolynomial));

  tmpreg = SPIx->CR1;

  tmpreg &= CR1_CLEAR_Mask;

  tmpreg |= (uint16_t)((uint32_t)SPI_InitStruct->SPI_Direction | SPI_InitStruct->SPI_Mode |
                  SPI_InitStruct->SPI_DataSize | SPI_InitStruct->SPI_CPOL |
                  SPI_InitStruct->SPI_CPHA | SPI_InitStruct->SPI_NSS |
                  SPI_InitStruct->SPI_BaudRatePrescaler | SPI_InitStruct->SPI_FirstBit);

  SPIx->CR1 = tmpreg;

  SPIx->I2SCFGR &= SPI_Mode_Select;

  SPIx->CRCPR = SPI_InitStruct->SPI_CRCPolynomial;
}

void SPI_StructInit(SPI_InitTypeDef* SPI_InitStruct)
{

  SPI_InitStruct->SPI_Direction = SPI_Direction_2Lines_FullDuplex;

  SPI_InitStruct->SPI_Mode = SPI_Mode_Slave;

  SPI_InitStruct->SPI_DataSize = SPI_DataSize_8b;

  SPI_InitStruct->SPI_CPOL = SPI_CPOL_Low;

  SPI_InitStruct->SPI_CPHA = SPI_CPHA_1Edge;

  SPI_InitStruct->SPI_NSS = SPI_NSS_Hard;

  SPI_InitStruct->SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;

  SPI_InitStruct->SPI_FirstBit = SPI_FirstBit_MSB;

  SPI_InitStruct->SPI_CRCPolynomial = 7;
}

void SPI_Cmd(SPI_TypeDef* SPIx, FunctionalState NewState)
{

  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    SPIx->CR1 |= CR1_SPE_Set;
  }
  else
  {

    SPIx->CR1 &= CR1_SPE_Reset;
  }
}

uint16_t SPI_I2S_ReceiveData(SPI_TypeDef* SPIx)
{

  assert_param(IS_SPI_ALL_PERIPH(SPIx));

  return SPIx->DR;
}

void SPI_CalculateCRC(SPI_TypeDef* SPIx, FunctionalState NewState)
{

  assert_param(IS_SPI_ALL_PERIPH(SPIx));
  assert_param(IS_FUNCTIONAL_STATE(NewState));
  if (NewState != DISABLE)
  {

    SPIx->CR1 |= CR1_CRCEN_Set;
  }
  else
  {

    SPIx->CR1 &= CR1_CRCEN_Reset;
  }
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
