/* Доработка: Кряжев А., 2026. Отдельный порт модема USART3. */
#ifndef COMM_UART_H
#define COMM_UART_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define COMM_UART_OVERFLOW 1u
#define COMM_UART_ORE 2u
#define COMM_UART_FE 4u
#define COMM_UART_NE 8u
typedef struct {
  uint16_t errors, overflow, ore, fe, ne;
} comm_uart_stats_t;

/* Только main: init/abort, один потребитель RX и один производитель TX.
   Сборка порта: COMM_UART_STANDALONE; старый uart_stm32f10x.c исключён.
   В рабочем проекте этапа 1 порт не подключён. */
void comm_uart_init(uint32_t baud);
/* Участок принадлежит UART, действует до consume/abort/init. */
uint16_t comm_uart_peek(const uint8_t **data);
bool comm_uart_consume(uint16_t count);
/* Единственный слот UART: заполнять прямо здесь, без чужих указателей.
   Указатель действует до commit/release/abort/init. После commit не писать. */
uint8_t *comm_uart_tx_acquire(uint16_t *capacity);
bool comm_uart_tx_commit(uint16_t count);
void comm_uart_tx_release(void);
/* Физический TC последнего принятого TX; abort не считается успехом. */
bool comm_uart_tx_done(void);
/* Накопительные насыщаемые счётчики, сброс только при init. */
void comm_uart_stats(comm_uart_stats_t *stats);
/* Прочитать и снять флаги потери потока; счётчики сохраняются. */
uint16_t comm_uart_errors(void);
/* Отменяет слот/очередь TX, очищает RX. Загруженный в DR байт не отозвать. */
void comm_uart_abort(void);
#endif
