/******************************************************************************
*    SPI functions
******************************************************************************/

#include "spi_stm32f10x.h"
#include "includes.h"


// инициализация SPI
// SPI подключенный к flash памяти
void spi_init (void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  SPI_InitTypeDef SPI_InitStruct;

  // первым делом инициализируем ножку чипселект
  spi_initCS();

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

  SPI_I2S_DeInit(SPI1);
  GPIO_StructInit(&GPIO_InitStructure);

  // ножки SCK, MOSI
  GPIO_InitStructure.GPIO_Pin = SPI_MOSI_SCK;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(SPI_PORT, &GPIO_InitStructure);
  // ножка MISO
  GPIO_InitStructure.GPIO_Pin = SPI_MISO;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(SPI_PORT, &GPIO_InitStructure);

  SPI_StructInit(&SPI_InitStruct);

  SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
  if (SystemCoreClock >= 48000000) 
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4; // частота тактирования SPI 72 / 4 = 18 МГц
  else 
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2; // 24/2 = 12
  SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;

  // инициализируем
  SPI_Init(SPI1, &SPI_InitStruct);
  // не считать контрольную сумма
  SPI_CalculateCRC(SPI1, DISABLE);
  // включаем
  SPI_Cmd(SPI1, ENABLE);

  // ждем передачи
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
  SPI_I2S_ReceiveData(SPI1);
}

// инициализация ножки chip select
void spi_initCS (void)
{
  GPIO_InitTypeDef        GPIO_InitStructure;

  // включаем тактирование
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

  // инициализируем структуру по умолчанию
  GPIO_StructInit(&GPIO_InitStructure);

  // ножка chip select
  GPIO_InitStructure.GPIO_Pin = SPI_CS_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(SPI_CS_PORT, &GPIO_InitStructure);

  // линию  chip select в высокий уровень
  GPIO_WriteBit(SPI_CS_PORT, SPI_CS_PIN, Bit_SET);
}

// чтение байта
uint8_t spi_read_byte (void)
{
  while((SPI1->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);  
  SPI1->DR = 0xff;
  while((SPI1->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);
  while((SPI1->SR & SPI_I2S_FLAG_BSY) != (uint16_t)RESET); 
   
  return (uint8_t)SPI1->DR;
}

// чтение байт с порта
void spi_read (uint8_t* dst, uint32_t length)
{
  if (length == 0) return;

  uint32_t i;

  for (i=0; i < length; i++){
    while((SPI1->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);  
    SPI1->DR = 0xff;
    while((SPI1->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);
    *dst++ = (uint8_t)SPI1->DR;
  }
  while((SPI1->SR & SPI_I2S_FLAG_BSY) != (uint16_t)RESET); 
}

// Отправка байта по SPI
uint8_t spi_send_byte (uint8_t data)
{
  while((SPI1->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);  
  SPI1->DR = data;
  while((SPI1->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);
  while((SPI1->SR & SPI_I2S_FLAG_BSY) != (uint16_t)RESET); 

  return (uint8_t)SPI1->DR;
}

// Отправка n байт по SPI
void spi_send (uint8_t* buf, uint32_t length)
{
  if (length == 0) return;

  uint32_t i;
  uint8_t dt;

  for (i = 0; i < length; i++)
  {
    while((SPI1->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);  
    SPI1->DR = *buf++;
    while((SPI1->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);
    dt = SPI1->DR;
  }
  while((SPI1->SR & SPI_I2S_FLAG_BSY) != (uint16_t)RESET);  
}

// Ожидание завершения передачи по SPI
void spi_wait_busy (void)
{
  while((SPI1->SR & SPI_I2S_FLAG_BSY) != (uint16_t)RESET);  
}