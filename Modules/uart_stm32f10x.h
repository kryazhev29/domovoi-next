// Доработка: Кряжев А., 2026
// Интерфейс единственного RS-485 USART1, без ветвей других портов.
// Буфер принадлежит вызывающему модулю, драйвер принимает и передаёт байты.
// legacy_port не используется; его удаление требует правки чужого ABI.

#ifndef __UART_H
#define __UART_H
#include "arm_comm.h"
#include "stm32f10x.h"

void USART1_IRQHandler(void);
bool uart_init(uint8_t legacy_port, uint32_t Baudrate, uint8_t Stopbits, uint8_t *rxbuffer, uint16_t bfsize);
void uart_send(uint8_t legacy_port, uint8_t *buf, uint16_t size);
void uart_clear(uint8_t legacy_port);
uint16_t uart_received(uint8_t legacy_port);
uint8_t uart_getstate(uint8_t legacy_port);
void uart_timer1ms(void);
void uart_setRecTimeout(uint8_t legacy_port, unsigned int t);
bool uart_isReceiving(uint8_t legacy_port);
#endif
