/* Доработка: Кряжев А., 2026. Один слот, одно серверное TCP. */
#ifndef COMM_CHANNEL_H
#define COMM_CHANNEL_H
#include <stdint.h>
#include <stdbool.h>
enum { COMM_CH_OPENED, COMM_CH_CLOSED, COMM_CH_TX_DONE, COMM_CH_PROBE_DONE, COMM_CH_FAULT };
enum { COMM_CH_PING = 1, COMM_CH_DNS = 2 };
/* init только после отмены UART/начала изолированного сеанса драйвера.
   callback только отмечает событие; не запускает AT. */
void comm_channel_init(void (*event)(uint8_t kind, uint8_t result));
void comm_channel_poll(uint32_t now_ms);
/* Тело предварительно до read_done OK; вместимость 0 откладывает QIRD. */
void comm_channel_receive(uint16_t (*binary)(const uint8_t *, uint16_t),
                          void (*read_done)(uint16_t, uint8_t));
void comm_channel_rx_capacity(uint16_t count);
bool comm_channel_ready(void);
/* Модем перестал отвечать посреди работы канала: нужен перезапуск модема. */
bool comm_channel_fault(void);
/* Служебная команда при свободном канале; false - занят или нет соединения. */
bool comm_channel_query(const char *cmd, void (*line)(const uint8_t *, uint16_t));
uint32_t comm_channel_diag(void);
/* Производитель пишет прямо сюда; capacity по sizeof слота. Занято/приоритет — NULL. */
uint8_t *comm_tx_acquire(uint16_t *capacity);
bool comm_tx_commit(uint16_t count);
/* Отмена только до commit; после commit слот держится до TCP ACK/безопасной отмены. */
void comm_tx_release(void);
bool comm_tx_busy(void);
void comm_channel_priority(bool pending);
void comm_channel_reopen(uint32_t pause_ms);
void comm_channel_registered(void);
void comm_channel_stable(void);
bool comm_channel_probe(uint8_t kind);
#endif
