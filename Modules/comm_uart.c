#include "domovoi_diag.h"
/* Доработка: Кряжев А., 2026. USART3: кольцо RX, фоновая передача TX. */
#include "comm_uart.h"
#include "board.h"
#include "stm32f10x_usart.h"
#include "misc.h"


/* DMA пишет прямо в кольцо; 511 полезных байт сохраняют прежний предел. */
static uint8_t rx[512];
/* Общая структура убирает межобъектные зазоры и сохраняет бюджет ОЗУ. */
static volatile struct {
  const uint8_t *tx_ptr;
  uint16_t rx_write, rx_read, rx_view, tx_length, tx_index;
  uint8_t tx_phase, tx_done;
  #if DOMOVOI_DIAG
  comm_uart_stats_t counters;
  #else
  struct { uint16_t errors; } counters;
  #endif
} uart_state;
enum { TX_IDLE, TX_SEND, TX_WAIT_TC };

#if DOMOVOI_DIAG
static void error_add(volatile uint16_t *count)
{
  if (*count != UINT16_MAX) ++*count;
}
#else
#define error_add(count) ((void)0)
#endif

/* RM0008 §13.3.3/§13.4: CNDTR и TC дают число записанных байт,
   а не только совпадение индексов. Обслуживать не реже половины круга
   (22,2 мс при 115200); SysTick — 1 мс. IRQ не вложены в учёт RX.
   Повтор снимка исключает переход HT/TC между ISR и CNDTR.
   Сбрасываем только увиденные флаги, остальные каналы не затронуты. */
void comm_uart_tick(void)
{
  if (!(RCC->AHBENR & RCC_AHBENR_DMA1EN)) return; /* SysTick может начаться до init. */
  uint32_t mask = __get_PRIMASK(), flags;
  uint16_t pos, delta, used;
  __disable_irq();
  do {
    flags = DMA1->ISR & (DMA_ISR_HTIF3 | DMA_ISR_TCIF3 | DMA_ISR_TEIF3);
    pos = (uint16_t)(sizeof(rx) - DMA1_Channel3->CNDTR) & (sizeof(rx) - 1u);
  } while (flags != (DMA1->ISR & (DMA_ISR_HTIF3 | DMA_ISR_TCIF3 | DMA_ISR_TEIF3)));
  DMA1->IFCR = flags;
  delta = (uint16_t)((pos - uart_state.rx_write) & (sizeof(rx) - 1u));
  if ((flags & DMA_ISR_TCIF3) && pos >= (uart_state.rx_write & (sizeof(rx) - 1u))) delta += sizeof(rx);
  uart_state.rx_write += delta;
  __DMB();
  uint16_t written = uart_state.rx_write, taken = uart_state.rx_read; /* порядок чтений volatile задан */
  used = (uint16_t)(written - taken);
  if ((delta && used >= sizeof(rx)) || (flags & DMA_ISR_TEIF3)) {
    uart_state.rx_view = sizeof(rx); /* Защёлка потери потока до abort, без нового ОЗУ. */
    uart_state.counters.errors |= COMM_UART_OVERFLOW;
    error_add(&uart_state.counters.overflow);
  }
  if (uart_state.rx_view == sizeof(rx)) used = sizeof(rx);
  /* PB14: HIGH — стоп при свободно < 128, LOW — пуск при занято < 128. */
  if (used > sizeof(rx) - 1u - 128u) { RTSG_HIGH; }
  else if (used < 128u) { RTSG_LOW; }
  __set_PRIMASK(mask);
}

#ifdef __IAR_SYSTEMS_ICC__
#pragma call_graph_root="interrupt"
#endif
void DMA1_Channel3_IRQHandler(void)
{
  comm_uart_tick();
}
#ifdef __IAR_SYSTEMS_ICC__
#pragma call_graph_root="interrupt"
#endif
void USART3_IRQHandler(void)
{
  uint32_t status = USART3->SR;
  uint16_t errors = 0;
  /* В обычном RX регистр DR принадлежит только DMA. Чтение DR по IDLE
     может украсть следующий байт между SR и DR; остаток публикуют tick/peek.
     При ошибке границы уже потеряны: снимаем флаг и отменяем весь поток. */
  if (status & (USART_SR_ORE | USART_SR_FE | USART_SR_NE)) {
    (void)USART_ReceiveData(USART3);
    if (status & USART_SR_ORE) { errors |= COMM_UART_ORE; error_add(&uart_state.counters.ore); }
    if (status & USART_SR_FE) { errors |= COMM_UART_FE; error_add(&uart_state.counters.fe); }
    if (status & USART_SR_NE) { errors |= COMM_UART_NE; error_add(&uart_state.counters.ne); }
    if (errors) uart_state.rx_view = sizeof(rx); /* DMA мог уже записать ошибочный байт. */
    comm_uart_tick();
  }
  if (errors) uart_state.counters.errors |= errors;
  if ((USART3->CR1 & USART_CR1_TXEIE) && (status & USART_SR_TXE)) {
    uint16_t pos = uart_state.tx_index, count = uart_state.tx_length;
    if (uart_state.tx_phase == TX_SEND && pos < count) {
      USART_SendData(USART3, uart_state.tx_ptr[pos]);
      uart_state.tx_index = ++pos;
      if (pos == count) {
        USART3->CR1 = (USART3->CR1 & ~USART_CR1_TXEIE) | USART_CR1_TCIE;
        uart_state.tx_phase = TX_WAIT_TC;
      }
    }
    /* TC из снимка до записи DR относится к прежнему байту. */
  } else if ((USART3->CR1 & USART_CR1_TCIE) && (status & USART_SR_TC)) {
    USART3->CR1 &= ~USART_CR1_TCIE;
    if (uart_state.tx_phase == TX_WAIT_TC) {
      uart_state.tx_ptr = NULL;
      uart_state.tx_phase = TX_IDLE;
      uart_state.tx_done = 1;
    }
  }
}

void comm_uart_abort(void)
{
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  DMA1_Channel3->CCR = 0; /* RM0008 §13.4.3: CNDTR меняется только при EN=0. */
  USART3->CR1 &= ~(USART_CR1_TXEIE | USART_CR1_TCIE);
  uart_state.tx_phase = TX_IDLE;
  uart_state.tx_done = 0;
  uart_state.tx_length = uart_state.tx_index = 0;
  uart_state.tx_ptr = NULL;
  uart_state.rx_write = uart_state.rx_read = uart_state.rx_view = 0;
  DMA1->IFCR = DMA_IFCR_CGIF3;
  DMA1_Channel3->CNDTR = sizeof(rx);
  /* Снятие старого RX; ошибки до abort остаются в диагностике. */
  (void)USART3->SR;
  (void)USART_ReceiveData(USART3);
  DMA1_Channel3->CCR = DMA_CCR3_MINC | DMA_CCR3_CIRC | DMA_CCR3_HTIE | DMA_CCR3_TCIE | DMA_CCR3_TEIE | DMA_CCR3_EN;
  comm_uart_tick();
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
  RCC->AHBENR |= RCC_AHBENR_DMA1EN;
  DMA1_Channel3->CCR = 0;
  DMA1_Channel3->CPAR = (uint32_t)(uintptr_t)&USART3->DR;
  DMA1_Channel3->CMAR = (uint32_t)(uintptr_t)rx;
  USART_DeInit(USART3);
  comm_uart_abort();
#if DOMOVOI_DIAG
  uart_state.counters.errors = uart_state.counters.overflow = uart_state.counters.ore = uart_state.counters.fe = uart_state.counters.ne = 0;
#else
  uart_state.counters.errors = 0;
#endif
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  gpio.GPIO_Pin = GPIO_Pin_11;
  gpio.GPIO_Pin |= GPIO_Pin_13;
  gpio.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOB, &gpio);
  gpio.GPIO_Pin = GPIO_Pin_10;
  gpio.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOB, &gpio);
  USART_StructInit(&uart);
  uart.USART_BaudRate = baud;
  uart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  /* v5: PB14 — программный RTS, PB13 — аппаратный CTS. */
  gpio.GPIO_Pin = GPIO_Pin_14;
  gpio.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &gpio);
  uart.USART_HardwareFlowControl = USART_HardwareFlowControl_CTS;
  USART_Init(USART3, &uart);
  /* IDLE IRQ не нужен: DMA HT/TC, SysTick и peek публикуют весь RX. */
  USART3->CR3 |= USART_CR3_DMAR;
  USART_ITConfig(USART3, USART_IT_ERR, ENABLE);
  irq.NVIC_IRQChannel = USART3_IRQn;
  irq.NVIC_IRQChannelPreemptionPriority = 1;
  irq.NVIC_IRQChannelSubPriority = 7;
  irq.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&irq);
  irq.NVIC_IRQChannel = DMA1_Channel3_IRQn;
  NVIC_Init(&irq);
  USART_Cmd(USART3, ENABLE);
  __set_PRIMASK(mask);
}

uint16_t comm_uart_peek(const uint8_t **data)
{
  if (data == NULL) return 0;
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  comm_uart_tick();
  uint16_t read = uart_state.rx_read & (sizeof(rx) - 1u), count = 0;
  if (uart_state.rx_view != sizeof(rx)) {
    uint16_t written = uart_state.rx_write;
    count = (uint16_t)(written - uart_state.rx_read);
    uint16_t contiguous = (uint16_t)(sizeof(rx) - read);
    if (count > contiguous) count = contiguous;
    uart_state.rx_view = count;
  }
  *data = count ? &rx[read] : NULL;
  __set_PRIMASK(mask);
  return count;
}
bool comm_uart_consume(uint16_t count)
{
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  comm_uart_tick();
  uint16_t view = uart_state.rx_view;
  if (view == sizeof(rx) || count > view) { __set_PRIMASK(mask); return false; }
  __DMB();
  uart_state.rx_read += count;
  uart_state.rx_view = (uint16_t)(view - count);
  comm_uart_tick();
  __set_PRIMASK(mask);
  return true;
}

bool comm_uart_tx(const uint8_t *data, uint16_t count)
{
  if (data == NULL || count > COMM_UART_TX_MAX || uart_state.tx_phase != TX_IDLE) return false;
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  uart_state.tx_ptr = count ? data : NULL;
  uart_state.tx_length = count;
  __DMB(); /* IRQ здесь отложен: остальные поля ещё не опубликованы. */
  uart_state.tx_index = 0;
  uart_state.tx_done = count == 0;
  uart_state.tx_phase = count ? TX_SEND : TX_IDLE;
  USART_ClearFlag(USART3, USART_FLAG_TC);
  __DMB();
  if (count) USART3->CR1 |= USART_CR1_TXEIE;
  __set_PRIMASK(mask);
  return true;
}

bool comm_uart_tx_done(void)
{
  return uart_state.tx_done != 0;
}

uint16_t comm_uart_errors(void)
{
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  uint16_t errors = uart_state.counters.errors;
  uart_state.counters.errors = 0;
  __set_PRIMASK(mask);
  return errors;
}

#if DOMOVOI_DIAG || defined(DOMOVOI_HOST_TEST)
void comm_uart_stats(comm_uart_stats_t *stats)
{
  if (stats == NULL) return;
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
#if DOMOVOI_DIAG
  *stats = uart_state.counters;
#else
  memset(stats,0,sizeof *stats); stats->errors = uart_state.counters.errors;
#endif
  __set_PRIMASK(mask);
}

#endif
