// Доработка: Кряжев А., 2026
// Интерфейс единственного RS-485 USART1, без ветвей других портов.
// Буфер принадлежит вызывающему модулю, драйвер принимает и передаёт байты.

#ifndef __UART_H
#define __UART_H
#include "arm_comm.h"
#include "stm32f10x.h"

void USART1_IRQHandler(void);
bool uart_init(uint32_t Baudrate, uint8_t Stopbits, uint8_t *rxbuffer, uint16_t bfsize);
void uart_send(uint8_t *buf, uint16_t size);
void uart_clear(void);
uint16_t uart_received(void);
uint8_t uart_getstate(void);
void uart_timer1ms(void);
void uart_setRecTimeout(unsigned int t);
bool uart_isReceiving(void);
#endif
