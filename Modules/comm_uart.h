/* Доработка: Кряжев А., 2026. Отдельный порт модема USART3. */
#ifndef COMM_UART_H
#define COMM_UART_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Предел кадра канала: 1038 байт журнала, округлено до 1040. */
#define COMM_UART_TX_MAX 1040u
#define COMM_UART_OVERFLOW 1u
#define COMM_UART_ORE 2u
#define COMM_UART_FE 4u
#define COMM_UART_NE 8u
typedef struct {
  uint16_t errors, overflow, ore, fe, ne;
} comm_uart_stats_t;

/* Только main: init/abort, один потребитель RX и один производитель TX.
   USART3 принадлежит только этому модулю. */
void comm_uart_init(uint32_t baud);
/* Только системный тик 1 мс и IRQ порта: учёт DMA и программный RTS. */
void comm_uart_tick(void);
/* Участок принадлежит UART, действует до consume/abort/init или ошибки RX. */
uint16_t comm_uart_peek(const uint8_t **data);
bool comm_uart_consume(uint16_t count);
/* Буфер принадлежит вызывающему модулю и статический; живёт до
   tx_done/abort/init, менять его нельзя. Стек и heap не передавать.
   Без копии: строка AT до 128 или кадр канала до COMM_UART_TX_MAX.
   Занято/NULL/длина выше предела — false, текущий TX не меняется.
   Ноль с ненулевым указателем — успех сразу, без DR и IRQ. */
bool comm_uart_tx(const uint8_t *data, uint16_t count);
/* Физический TC последнего принятого TX; abort не считается успехом. */
bool comm_uart_tx_done(void);
/* Накопительные насыщаемые счётчики, сброс только при init. */
void comm_uart_stats(comm_uart_stats_t *stats);
/* Прочитать и снять флаги потери потока; счётчики сохраняются. */
uint16_t comm_uart_errors(void);
/* Отменяет TX, забывает указатель, очищает RX. Байт в DR не отозвать. */
void comm_uart_abort(void);
#endif
