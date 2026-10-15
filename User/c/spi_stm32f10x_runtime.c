// Доработка: Кряжев А., 2026
// Эталон SPI1 внешней W25: инициализация, обмен байтом и буфером.
// Каждое аппаратное ожидание ограничено 512 опросами, отказ выключает SPI.
// Приём отправляет FF; перед возвратом порт дожидается конца последнего байта.

#include "spi_stm32f10x.h"
#include "includes.h"
// Проверить доступность SPI после старта или аппаратного отказа.
uint8_t spi_ready(void) { return (SPI1->CR1 & SPI_CR1_SPE) != 0; }
// Ограниченно ждать аппаратный флаг, отключая SPI при таймауте.
static uint8_t spi_wait_flag(uint16_t flag, uint8_t set)
{
  if (!spi_ready()) return 0;
  uint32_t budget = 512u;
  while (((SPI1->SR & flag) != 0) != (set != 0)) {
    if (--budget == 0) { SPI1->CR1 &= ~SPI_CR1_SPE; return 0; }
  }
  return 1;
}

// Настроить SPI1 внешней флеш и очистить остаток принятого байта.
void spi_init (void)
{
  spi_initCS();
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
  RCC->APB2RSTR |= RCC_APB2RSTR_SPI1RST;
  RCC->APB2RSTR &= ~RCC_APB2RSTR_SPI1RST;
  // PA5/PA7: AF push-pull 50 МГц; PA6: вход с подтяжкой вверх.
  SPI_PORT->BSRR = SPI_MISO;
  SPI_PORT->CRL = (SPI_PORT->CRL & ~0xfff00000u) | 0xb8b00000u;
  SPI1->CRCPR = 7;
  // Full duplex, 8 бит, mode 0, MSB; NSS программный, 72/4=18 МГц.
  SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSI | SPI_CR1_SSM | SPI_CR1_BR_0 | SPI_CR1_SPE;
  if (!spi_wait_flag(SPI_SR_TXE, 1)) return;
  (void)SPI1->DR;
}

// Настроить PB12 CS в push-pull 50 МГц и высокий уровень до включения SPI.
void spi_initCS (void)
{
  RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
  SPI_CS_PORT->CRH = (SPI_CS_PORT->CRH & ~0x000f0000u) | 0x00030000u;
  SPI_CS_PORT->BSRR = SPI_CS_PIN;
}

// чтение байта
uint8_t spi_read_byte(void)
{
  return spi_send_byte(0xff);
}

// чтение байт с порта
void spi_read(uint8_t* dst, uint32_t length)
{
  for (uint32_t i = 0; i < length; ++i) {
    if (!spi_wait_flag(SPI_SR_TXE, 1)) return;
    SPI1->DR = 0xff;
    if (!spi_wait_flag(SPI_SR_RXNE, 1)) return;
    *dst++ = (uint8_t)SPI1->DR;
  }
  (void)spi_wait_flag(SPI_SR_BSY, 0);
}

// Отправка байта по SPI
uint8_t spi_send_byte(uint8_t data)
{
  if (!spi_wait_flag(SPI_SR_TXE, 1)) return 0xff;
  SPI1->DR = data;
  if (!spi_wait_flag(SPI_SR_RXNE, 1) || !spi_wait_flag(SPI_SR_BSY, 0)) return 0xff;
  return (uint8_t)SPI1->DR;
}

// Отправка n байт по SPI
void spi_send(uint8_t* buf, uint32_t length)
{
  for (uint32_t i = 0; i < length; ++i) {
    if (!spi_wait_flag(SPI_SR_TXE, 1)) return;
    SPI1->DR = *buf++;
    if (!spi_wait_flag(SPI_SR_RXNE, 1)) return;
    (void)SPI1->DR;
  }
  (void)spi_wait_flag(SPI_SR_BSY, 0);
}
