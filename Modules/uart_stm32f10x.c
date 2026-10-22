// Доработка: Кряжев А., 2026
// Единственный UART приложения — USART1 для RS-485.
// ISR дописывает буфер, главный цикл разбирает его после межбайтовой тишины.
// Граница приёма проверяет ёмкость; ожидание освобождения TX ограничено.

#include "uart_stm32f10x.h"
#include <string.h>

typedef struct {
  uint8_t *rxbuffer;
  uint16_t bfsize;
  uint16_t received;
  uint16_t timetowait;
  uint16_t twait;
  uint8_t state;
  uint8_t receiving;
} uart_t;

#pragma data_alignment=4
static volatile uart_t uart;

// Принять внешний байт; при полном буфере не записывать за его границей.
static bool uart_addbyte(uint8_t data)
{
  uint8_t *buffer = uart.rxbuffer;
  if (buffer == NULL) return false;
  uint16_t count = uart.received;
  if (count < uart.bfsize) {
    buffer[count] = data;
    uart.received = count + 1u;
    uart.twait = uart.timetowait;
    uart.receiving = 1;
    uart.state = 0;
    return true;
  }
  return false;
}

#pragma call_graph_root="interrupt"
// Принять аппаратный байт RS-485 в единственный буфер.
// RXNEIE вызывает прерывание и по ORE (RM0008 §27.6.1); ORE снимает только чтение SR, затем DR,
// иначе ORE без RXNE повторяет прерывание до сторожевого таймера. Байт в DR при ORE целый.
void USART1_IRQHandler(void)
{
  uint32_t sr = USART1->SR;
  if (sr & (USART_SR_RXNE | USART_SR_ORE)) {
    uint8_t data = (uint8_t)USART1->DR;
    if (sr & USART_SR_RXNE) uart_addbyte(data);
  }
}

// Настроить USART1 и передать ему приёмный буфер владельца RS-485.
bool uart_init(uint32_t Baudrate, uint8_t Stopbits, uint8_t *rxbuffer, uint16_t bfsize)
{
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  uart.rxbuffer = rxbuffer;
  uart.bfsize = bfsize;
  uart.timetowait = 2;
  uart.received = uart.twait = 0;
  uart.state = uart.receiving = 0;
  __set_PRIMASK(mask);
  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
  RCC->APB2RSTR |= RCC_APB2RSTR_USART1RST;
  RCC->APB2RSTR &= ~RCC_APB2RSTR_USART1RST;
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_IOPAEN;
  GPIOA->CRH = (GPIOA->CRH & ~0x00000FF0u) | 0x000008B0u;
  GPIOA->BSRR = GPIO_Pin_10;
  USART1->CR2 = Stopbits == 0 ? 0 : USART_CR2_STOP_1;
  USART1->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;
  USART1->CR3 = 0;
  // Делитель APB2 и округление дроби BRR повторяют StdPeriph побитно.
  uint32_t prescaler = (RCC->CFGR & RCC_CFGR_PPRE2) >> 11;
  uint32_t clock = SystemCoreClock >> (prescaler < 4 ? 0 : prescaler - 3);
  uint32_t divider = (25u * clock) / (4u * Baudrate);
  uint32_t integer = divider / 100u;
  USART1->BRR = (integer << 4) | (((divider - 100u * integer) * 16u + 50u) / 100u & 15u);
  SCB->AIRCR = 0x05FA0600u;
  board_irq_enable(USART1_IRQn, 0xF0u);
  USART1->CR1 |= USART_CR1_UE;
  return true;
}

// Сменить приёмный буфер атомарно относительно ISR; порт и скорость не трогаются.
// used — уже разобранные байты: пришедшие после них во время разбора переносятся в начало
// нового буфера (UINT16_MAX — ничего не переносить).
void uart_rebuffer(uint8_t *rxbuffer, uint16_t bfsize, uint16_t used)
{
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  uint16_t left = uart.received > used ? (uint16_t)(uart.received - used) : 0u;
  if (left > bfsize) left = bfsize;
  // Перенос к началу: прямое копирование верно и внутри того же буфера; короче вызова memmove.
  for (uint16_t i = 0; i < left; ++i) rxbuffer[i] = uart.rxbuffer[used + i];
  uart.rxbuffer = rxbuffer;
  uart.bfsize = bfsize;
  uart.received = left;
  if (!left) uart.twait = uart.state = uart.receiving = 0;
  __set_PRIMASK(mask);
}

// Передать байт с ограниченным ожиданием освобождения регистра.
static bool uart_sendbyte(uint8_t ch)
{
  uint32_t budget = 32000u;
  while ((USART1->SR & USART_SR_TXE) == 0) {
    if (--budget == 0) return false;
  }
  USART1->DR = ch;
  return true;
}

// Передать двоичный буфер; при отказе порта прекратить текущую посылку.
void uart_send(uint8_t *buf, uint16_t size)
{
  if (!(USART1->CR1 & USART_CR1_UE) || (size && buf == NULL)) return;
  for (uint16_t i = 0; i < size; ++i) if (!uart_sendbyte(buf[i])) return;
}

// Атомарно освободить принятый кадр относительно ISR.
void uart_clear(void)
{
  if (uart.rxbuffer == NULL) return;
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  memset(uart.rxbuffer, 0, uart.received);
  uart.received = uart.twait = 0;
  uart.state = uart.receiving = 0;
  __set_PRIMASK(mask);
}

// Вернуть число байтов для разбора фотоответа.
uint16_t uart_received(void)
{
  return uart.received;
}

// Разрешить разбор кадра после заданной межбайтовой тишины.
uint8_t uart_getstate(void)
{
  return uart.twait ? 0 : uart.state;
}

// Отсчитать миллисекунду межбайтовой тишины единственного порта.
void uart_timer1ms(void)
{
  if (uart.twait && --uart.twait == 0) {
    uart.state = 1;
    uart.receiving = 0;
  }
}

// Задать межбайтовую задержку, минимум 1 мс.
void uart_setRecTimeout(unsigned int t)
{
  uart.timetowait = t == 0 ? 1 : t;
}

// Проверить, продолжается ли приём текущего кадра.
bool uart_isReceiving(void)
{
  return uart.receiving != 0;
}
