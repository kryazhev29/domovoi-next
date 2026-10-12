/* Доработка: Кряжев А., 2026.
   Владеет TCP сервера и единственным буфером отправки.
   Слот освобождается после TCP ACK; чтение выдаёт предварительное тело до финала AT.
   После звонка ожидаемый обрыв восстанавливается обычным путём без первой паузы.
*/
#ifndef COMM_CHANNEL_H
#define COMM_CHANNEL_H
#include <stdint.h>
#include "domovoi_diag.h"
#include <stdbool.h>
/* OPENING, OPENED и OPEN_FAILED - с адресом (0 - основной, 1 - резервный). */
enum { COMM_CH_OPENED, COMM_CH_CLOSED, COMM_CH_TX_DONE, COMM_CH_PROBE_DONE, COMM_CH_FAULT, COMM_CH_OPEN_FAILED, COMM_CH_OPENING };
enum { COMM_CH_PING = 1, COMM_CH_DNS = 2 };
/* init только после отмены UART/начала изолированного сеанса драйвера.
   обработчик только отмечает событие; не запускает AT. */
void comm_channel_init(void (*event)(uint8_t kind, uint8_t result));
/* Продвинуть канал, передачу, чтение и фоновые запросы. */
void comm_channel_poll(uint32_t now_ms);
/* Тело предварительно до read_done OK; вместимость 0 откладывает QIRD. */
void comm_channel_receive(uint16_t (*binary)(const uint8_t *, uint16_t),
                          void (*read_done)(uint16_t, uint8_t));
/* Ограничить следующий QIRD свободным местом потребителя. */
void comm_channel_rx_capacity(uint16_t count);
/* Сообщить, открыт ли серверный TCP. */
bool comm_channel_ready(void);
/* Модем перестал отвечать посреди работы канала: нужен перезапуск модема. */
bool comm_channel_fault(void);
/* Служебная команда при свободном канале; false - занят или нет соединения. */
bool comm_channel_query(const char *cmd, void (*line)(const uint8_t *, uint16_t));
/* Собрать состояние канала для стендовой команды 111. */
#if DOMOVOI_DIAG
// Выдать состояние канала для стендовой команды 111.
uint32_t comm_channel_diag(void);
#endif
/* Производитель пишет прямо сюда; capacity по sizeof слота. Занято/приоритет — NULL. */
uint8_t *comm_tx_acquire(uint16_t *capacity);
/* Передать заполненный слот каналу до подтверждения TCP. */
bool comm_tx_commit(uint16_t count);
/* Отмена только до commit; после commit слот держится до TCP ACK/безопасной отмены. */
void comm_tx_release(void);
/* Сообщить, занят ли единственный слот отправки. */
bool comm_tx_busy(void);
/* Отложить фоновые операции ради регистрации или статуса. */
void comm_channel_priority(bool pending);
/* Закрыть прежний TCP и назначить обычную повторную попытку. */
void comm_channel_reopen(uint32_t pause_ms);
/* Зафиксировать принятый сервером новый сеанс. */
void comm_channel_registered(void);
/* Снять ступени отката после устойчивой связи. */
void comm_channel_stable(void);
/* Вызов/проверка данных: ожидаемый обрыв не увеличивает ступени отката. */
void comm_channel_call(bool active);
/* Первое восстановление после вызова пропускает счёт и откат. */
bool comm_channel_expected(void);
/* Ответ сервера подтверждает, что прежний TCP пережил вызов. */
void comm_channel_alive(void);
/* Сообщить, занят ли AT голосовым вызовом. */
bool comm_channel_call_pending(void);
/* Поставить проверку интернета при открытом TCP или после отказа QIOPEN. */
bool comm_channel_probe(uint8_t kind);
/* Последний законченный ya.ru: 0 нет, 1 ping, 2 DNS, 3 отказ. */
uint8_t comm_channel_probe_result(void);
#endif
