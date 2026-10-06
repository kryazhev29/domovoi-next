// Доработка: Кряжев А., 2026
// Интерфейс единственного RS-485 USART1, без ветвей других портов.
// Буфер принадлежит вызывающему модулю, драйвер принимает и передаёт байты.

#ifndef __UART_H
#define __UART_H
#include "arm_comm.h"
#include "stm32f10x.h"

// Принять аппаратный байт RS-485.
void USART1_IRQHandler(void);
// Настроить единственный USART1 и буфер приёма.
bool uart_init(uint32_t Baudrate, uint8_t Stopbits, uint8_t *rxbuffer, uint16_t bfsize);
// Передать буфер с ограниченным аппаратным ожиданием.
void uart_send(uint8_t *buf, uint16_t size);
// Атомарно освободить принятый кадр.
void uart_clear(void);
// Вернуть число принятых байтов.
uint16_t uart_received(void);
// Разрешить разбор после межбайтовой тишины.
uint8_t uart_getstate(void);
// Отсчитать миллисекунду тишины.
void uart_timer1ms(void);
// Задать межбайтовую задержку не меньше 1 мс.
void uart_setRecTimeout(unsigned int t);
// Проверить незавершённый приём.
bool uart_isReceiving(void);
#endif
