/* Доработка: Кряжев А., 2026.
   Обслуживает USART3 и приём через DMA1 канал 3 на плате v5.
   DMA владеет приёмом, прерывание USART передаёт данные до физического TC.
   PB14 управляет потоком; потери RX запрещают разбор до отмены сеанса.
*/
#include "domovoi_diag.h"
#include "comm_uart.h"
#include "board.h"

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
/* Увеличить диагностический счётчик с насыщением. */
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
/* Опубликовать принятую часть кольца по прерыванию DMA. */
void DMA1_Channel3_IRQHandler(void)
{
  comm_uart_tick();
}
#ifdef __IAR_SYSTEMS_ICC__
#pragma call_graph_root="interrupt"
#endif
/* Передать очередной байт или зафиксировать ошибку USART3. */
void USART3_IRQHandler(void)
{
  uint32_t status = USART3->SR;
  uint16_t errors = 0;
  /* В обычном RX регистр DR принадлежит только DMA. Чтение DR по IDLE
     может украсть следующий байт между SR и DR; остаток публикуют tick/peek.
     При ошибке границы уже потеряны: снимаем флаг и отменяем весь поток. */
  if (status & (USART_SR_ORE | USART_SR_FE | USART_SR_NE)) {
    (void)USART3->DR;
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
      USART3->DR = uart_state.tx_ptr[pos];
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

/* Отменить TX и очистить кольцо RX при смене сеанса. */
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
  (void)USART3->DR;
  DMA1_Channel3->CCR = DMA_CCR3_MINC | DMA_CCR3_CIRC | DMA_CCR3_HTIE | DMA_CCR3_TCIE | DMA_CCR3_TEIE | DMA_CCR3_EN;
  comm_uart_tick();
  __set_PRIMASK(mask);
}

/* Настроить регистры USART3, GPIOB, DMA и NVIC платы v5. */
void comm_uart_init(uint32_t baud)
{
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPBEN;
  RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
  RCC->AHBENR |= RCC_AHBENR_DMA1EN;
  DMA1_Channel3->CCR = 0;
  DMA1_Channel3->CPAR = (uint32_t)(uintptr_t)&USART3->DR;
  DMA1_Channel3->CMAR = (uint32_t)(uintptr_t)rx;
  RCC->APB1RSTR |= RCC_APB1RSTR_USART3RST;
  RCC->APB1RSTR &= ~RCC_APB1RSTR_USART3RST;
  comm_uart_abort();
#if DOMOVOI_DIAG
  uart_state.counters.errors = uart_state.counters.overflow = uart_state.counters.ore = uart_state.counters.fe = uart_state.counters.ne = 0;
#else
  uart_state.counters.errors = 0;
#endif
  /* PB10 — TX 50 МГц, PB11/PB13 — входы с подтяжкой, PB14 — RTS. */
  GPIOB->CRH = (GPIOB->CRH & ~0x0FF0FF00u) | 0x03808B00u;
  GPIOB->BSRR = GPIO_Pin_11 | GPIO_Pin_13;
  /* APB1 может быть делён; округление BRR совпадает с библиотекой ST. */
  uint32_t prescaler = (RCC->CFGR & RCC_CFGR_PPRE1) >> 8;
  uint32_t clock = SystemCoreClock >> (prescaler < 4 ? 0 : prescaler - 3);
  uint32_t divider = (25u * clock) / (4u * baud);
  uint32_t integer = divider / 100u;
  USART3->BRR = (integer << 4) | (((divider - 100u * integer) * 16u + 50u) / 100u & 15u);
  USART3->CR2 = 0;
  USART3->CR3 = USART_CR3_CTSE | USART_CR3_DMAR | USART_CR3_EIE;
  USART3->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_UE;
  /* До настройки RS-485 группа 0, после — 1: сохраняем прежнее кодирование NVIC. */
  uint8_t priority = (SCB->AIRCR & (7u << 8)) == (7u << 8) ? 0x70u : 0xF0u;
  board_irq_enable(USART3_IRQn, priority);
  board_irq_enable(DMA1_Channel3_IRQn, priority);
  __set_PRIMASK(mask);
}

/* Выдать непрерывный участок текущего кольца RX. */
uint16_t comm_uart_peek(const uint8_t **data)
{
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
/* Освободить принятый участок, сохранив защиту от потери DMA. */
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

/* Передать владение готовым буфером фоновому передатчику. */
bool comm_uart_tx(const uint8_t *data, uint16_t count)
{
  if (uart_state.tx_phase != TX_IDLE) return false;
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  uart_state.tx_ptr = data;
  uart_state.tx_length = count;
  __DMB(); /* IRQ здесь отложен: остальные поля ещё не опубликованы. */
  uart_state.tx_index = 0;
  uart_state.tx_done = 0;
  uart_state.tx_phase = TX_SEND;
  USART3->SR = ~USART_SR_TC;
  __DMB();
  USART3->CR1 |= USART_CR1_TXEIE;
  __set_PRIMASK(mask);
  return true;
}

/* Сообщить о физическом завершении последнего байта TX. */
bool comm_uart_tx_done(void)
{
  return uart_state.tx_done != 0;
}

/* Забрать и снять флаги потери потока. */
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
/* Скопировать насыщаемые счётчики стендовой диагностики. */
void comm_uart_stats(comm_uart_stats_t *stats)
{
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
