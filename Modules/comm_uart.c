/* ƒоработка:  р€жев ј., 2026. USART3: кольцо RX, фонова€ передача TX. */
#include "comm_uart.h"
#include "board.h"
#include "stm32f10x_usart.h"
#include "misc.h"

#ifndef COMM_UART_STANDALONE
#error "comm_uart: select standalone USART3 owner, exclude uart_stm32f10x.c"
#endif

/* 511 полезных байт: один элемент различает полное и пустое кольцо. */
static uint8_t rx[512];
static const uint8_t * volatile tx_data;
static volatile uint16_t rx_write, rx_read;
static uint16_t rx_view;
static volatile uint16_t tx_count, tx_pos;
static volatile uint8_t tx_state, tx_complete;
static volatile comm_uart_stats_t counters;
enum { TX_IDLE, TX_SEND, TX_WAIT_TC };

/* ¬ызываетс€ в IRQ или при запрещЄнных IRQ, чтобы не разрешить приЄм
   по устаревшему заполнению. LOW Ч приЄм, HIGH Ч остановка модема. */
static void rts_update(void)
{
#if DOMOVOI_APP_HW == 139 && !defined(BOARD_IP_V3)
  uint16_t write = rx_write, read = rx_read;
  uint16_t used = (uint16_t)((write - read) & (sizeof(rx) - 1u));
  if (sizeof(rx) - 1u - used < 128u) { RTSG_HIGH; }
  else if (used < 128u) { RTSG_LOW; }
#endif
}

static void error_add(volatile uint16_t *count)
{
  if (*count != UINT16_MAX) ++*count;
}

#ifdef __IAR_SYSTEMS_ICC__
#pragma call_graph_root="interrupt"
#endif
void USART3_IRQHandler(void)
{
  uint32_t status = USART3->SR;
  uint16_t errors = 0;
  /* SR, затем DR: единственна€ правильна€ очистка ORE/FE/NE/RXNE. */
  if (status & (USART_SR_RXNE | USART_SR_ORE | USART_SR_FE | USART_SR_NE)) {
    uint8_t byte = (uint8_t)USART_ReceiveData(USART3);
    if (status & USART_SR_ORE) { errors |= COMM_UART_ORE; error_add(&counters.ore); }
    if (status & USART_SR_FE) { errors |= COMM_UART_FE; error_add(&counters.fe); }
    if (status & USART_SR_NE) { errors |= COMM_UART_NE; error_add(&counters.ne); }
    if ((status & USART_SR_RXNE) && !(status & (USART_SR_ORE | USART_SR_FE | USART_SR_NE))) {
      uint16_t next = (uint16_t)((rx_write + 1u) & (sizeof(rx) - 1u));
      if (rx_write >= sizeof(rx) || next == rx_read) { errors |= COMM_UART_OVERFLOW; error_add(&counters.overflow); }
      else {
        rx[rx_write] = byte;
        __DMB();
        rx_write = next;
        rts_update();
      }
    }
  }
  if (errors) counters.errors |= errors;
  if ((USART3->CR1 & USART_CR1_TXEIE) && (status & USART_SR_TXE)) {
    uint16_t pos = tx_pos, count = tx_count;
    if (tx_state == TX_SEND && pos < count) {
      USART_SendData(USART3, tx_data[pos]);
      tx_pos = ++pos;
      if (pos == count) {
        USART3->CR1 = (USART3->CR1 & ~USART_CR1_TXEIE) | USART_CR1_TCIE;
        tx_state = TX_WAIT_TC;
      }
    }
    /* TC из снимка до записи DR относитс€ к прежнему байту. */
  } else if ((USART3->CR1 & USART_CR1_TCIE) && (status & USART_SR_TC)) {
    USART3->CR1 &= ~USART_CR1_TCIE;
    if (tx_state == TX_WAIT_TC) {
      tx_data = NULL;
      tx_state = TX_IDLE;
      tx_complete = 1;
    }
  }
}

void comm_uart_abort(void)
{
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  USART3->CR1 &= ~(USART_CR1_TXEIE | USART_CR1_TCIE);
  tx_state = TX_IDLE;
  tx_complete = 0;
  tx_count = tx_pos = 0;
  tx_data = NULL;
  rx_write = rx_read = rx_view = 0;
  rts_update();
  /* —н€тие старого RX; ошибки до abort остаютс€ в диагностике. */
  (void)USART3->SR;
  (void)USART_ReceiveData(USART3);
  __DMB();
  __set_PRIMASK(mask);
}

void comm_uart_init(uint32_t baud)
{
  GPIO_InitTypeDef gpio;
  USART_InitTypeDef uart;
  NVIC_InitTypeDef irq;
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
  USART_DeInit(USART3);
  comm_uart_abort();
  counters.errors = counters.overflow = counters.ore = counters.fe = counters.ne = 0;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  gpio.GPIO_Pin = GPIO_Pin_11;
#if DOMOVOI_APP_HW == 139 && !defined(BOARD_IP_V3)
  gpio.GPIO_Pin |= GPIO_Pin_13;
#endif
  gpio.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOB, &gpio);
  gpio.GPIO_Pin = GPIO_Pin_10;
  gpio.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOB, &gpio);
  USART_StructInit(&uart);
  uart.USART_BaudRate = baud;
  uart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
#if DOMOVOI_APP_HW == 139 && !defined(BOARD_IP_V3)
  /* v5: PB14 Ч программный RTS, PB13 Ч аппаратный CTS. */
  gpio.GPIO_Pin = GPIO_Pin_14;
  gpio.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &gpio);
  uart.USART_HardwareFlowControl = USART_HardwareFlowControl_CTS;
#else
  uart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
#endif
  USART_Init(USART3, &uart);
  USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
  USART_ITConfig(USART3, USART_IT_ERR, ENABLE);
  irq.NVIC_IRQChannel = USART3_IRQn;
  irq.NVIC_IRQChannelPreemptionPriority = 1;
  irq.NVIC_IRQChannelSubPriority = 7;
  irq.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&irq);
  USART_Cmd(USART3, ENABLE);
  __set_PRIMASK(mask);
}

uint16_t comm_uart_peek(const uint8_t **data)
{
  if (data == NULL) return 0;
  uint16_t write = rx_write;
  __DMB();
  uint16_t read = rx_read;
  uint16_t count = (uint16_t)((write - read) & (sizeof(rx) - 1u));
  uint16_t contiguous = (uint16_t)(sizeof(rx) - read);
  if (count > contiguous) count = contiguous;
  rx_view = count;
  *data = count ? &rx[read] : NULL;
  return count;
}

bool comm_uart_consume(uint16_t count)
{
  uint16_t view = rx_view;
  if (count > view) return false;
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  __DMB();
  rx_read = (uint16_t)((rx_read + count) & (sizeof(rx) - 1u));
  rx_view = (uint16_t)(view - count);
  rts_update();
  __set_PRIMASK(mask);
  return true;
}

bool comm_uart_tx(const uint8_t *data, uint16_t count)
{
  if (data == NULL || count > COMM_UART_TX_MAX || tx_state != TX_IDLE) return false;
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  tx_data = count ? data : NULL;
  tx_count = count;
  __DMB(); /* IRQ здесь отложен: остальные пол€ ещЄ не опубликованы. */
  tx_pos = 0;
  tx_complete = count == 0;
  tx_state = count ? TX_SEND : TX_IDLE;
  USART_ClearFlag(USART3, USART_FLAG_TC);
  __DMB();
  if (count) USART3->CR1 |= USART_CR1_TXEIE;
  __set_PRIMASK(mask);
  return true;
}

bool comm_uart_tx_done(void)
{
  return tx_complete != 0;
}

uint16_t comm_uart_errors(void)
{
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  uint16_t errors = counters.errors;
  counters.errors = 0;
  __set_PRIMASK(mask);
  return errors;
}

void comm_uart_stats(comm_uart_stats_t *stats)
{
  if (stats == NULL) return;
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  *stats = counters;
  __set_PRIMASK(mask);
}
