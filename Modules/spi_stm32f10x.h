

#ifndef __SPI_H
#define __SPI_H

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"

// инициализация SPI
// SPI подключенный к flash памяти
void spi_init (void);

// инициализация ножки chip select
void spi_initCS (void);

uint8_t spi_ready(void);
uint8_t spi_read_byte (void);

// чтение байт с порта
void spi_read (uint8_t* dst, uint32_t length);

// Отправка байта по SPI
uint8_t spi_send_byte (uint8_t data);

// Отправка n байт по SPI
void spi_send (uint8_t* buf, uint32_t length);

#endif
