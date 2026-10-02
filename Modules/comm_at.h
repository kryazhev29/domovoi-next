/* Доработка: Кряжев А., 2026. Неблокирующий движок AT, без heap. */
#ifndef COMM_AT_H
#define COMM_AT_H
#include <stdint.h>
#include <stdbool.h>

/* Общий разбор ограниченной строки; пробелы убираются только явно. */
bool comm_at_equal(const uint8_t *data, uint16_t count, const char *text);
uint8_t comm_at_choice(const uint8_t *data, uint16_t count, const char *names);
bool comm_at_prefix(const uint8_t *data, uint16_t count, const char *text);
uint16_t comm_at_spaces(const uint8_t *data, uint16_t count, uint16_t pos);
bool comm_at_number(const uint8_t *data, uint16_t count, uint16_t *pos,
                    uint32_t limit, uint32_t *value);
/* Доверенный шаблон: 1 uint32 с пробелами, 2 uint32, 3 октет,
   4 uint16, 5 uint16 с пробелами, 6 октет с пробелами, * произвольный хвост; out по числу меток. */
bool comm_at_parse(const uint8_t *data, uint16_t count, const char *pattern, uint32_t *out);
/* Общая сборка команды; count==capacity означает переполнение. */
uint16_t comm_at_copy(char *out, uint16_t capacity, uint16_t pos, const char *text);
uint16_t comm_at_decimal(char *out, uint16_t capacity, uint16_t pos, uint16_t value);
/* Ответы модема разрешают пробел перед числом; команда QIRD — нет. */
#define comm_at_scan(p,n,pos,limit,out) \
  (*(pos) = comm_at_spaces(p,n,*(pos)), comm_at_number(p,n,pos,limit,out))

#define COMM_AT_POLL_BYTES 128u
enum { COMM_AT_TEXT, COMM_AT_PROMPT, COMM_AT_SLM_QIRD, COMM_AT_M66_QIRD, COMM_AT_SEND };
enum { COMM_AT_LINE, COMM_AT_URC, COMM_AT_DONE, COMM_AT_NO_ANSWER, COMM_AT_STREAM_ERROR };
enum { COMM_AT_OK, COMM_AT_ERROR, COMM_AT_CME, COMM_AT_GOT_PROMPT,
       COMM_AT_TIMEOUT, COMM_AT_LINE_OVERFLOW, COMM_AT_UART_ERROR,
       COMM_AT_BAD_FRAME, COMM_AT_BAD_LENGTH, COMM_AT_SINK_ERROR };
enum { COMM_AT_ACCEPTED, COMM_AT_BUSY, COMM_AT_INVALID };
typedef struct {
  const uint8_t *command; /* Только на время start; копируется в буфер AT. */
  uint16_t count;
  uint8_t mode;
  uint32_t timeout_ms;
} comm_at_request_t;
typedef struct {
  /* Данные принадлежат AT/UART, действуют только внутри вызова.
     LINE/URC: detail=0; DONE: detail=результат, count=код CME.
     Текст CME выдаётся LINE, код неизвестен (UINT16_MAX).
     NO_ANSWER/STREAM_ERROR: detail=причина, data=NULL, count=0.
     Нельзя запускать AT/poll из обработчика: движок отвергает рекурсию. */
  void (*event)(uint8_t kind, const uint8_t *data, uint16_t count, uint16_t detail);
  /* Тело предварительное: потребитель фиксирует его только после DONE OK.
     Возвращает принятое число, 0 — обратное давление. Копии в AT нет. */
  uint16_t (*binary)(const uint8_t *data, uint16_t count);
} comm_at_handlers_t;

/* init только после отмены UART/нового питания; указатели на данные не хранит.
   Сроки передаёт единая таблица comm_timeouts будущего драйвера; <2^31 мс.
   recovery_ms — общий срок дочитывания тела и AT+CMEE? -> +CMEE: -> OK.
   Старые финалы до +CMEE: игнорируются, потерянный финал не задерживает запрос.
   Потеря границ блокирует движок; init не заменяет отмену UART/сеанса. */
void comm_at_init(const comm_at_handlers_t *handlers, uint32_t recovery_ms);
uint8_t comm_at_start(const comm_at_request_t *request, uint32_t now_ms);
/* После приглашения: только статический слот канала, до DONE/abort.
   SEND OK подтверждает приём модемом; TCP ACK проверяет канал отдельно. */
uint8_t comm_at_payload(const uint8_t *data, uint16_t count, uint32_t timeout_ms, uint32_t now_ms);
void comm_at_poll(uint32_t now_ms);
/* После ошибки границ/потери тела остаётся занят до init нового сеанса. */
bool comm_at_busy(void);
#endif
