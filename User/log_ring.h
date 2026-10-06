// Доработка: Кряжев А., 2026
// Интерфейс кольцевого журнала и долговечных логических курсоров.
// Владелец записи сохраняет её тело до последней проверки W25.
// Подтверждение курсора сначала публикует снимок настроек A/B.
#ifndef DOMOVOI_LOG_RING_H
#define DOMOVOI_LOG_RING_H
#include "includes.h"
#define LOG_RING_MAGIC 0xD74C4752u
#define LOG_RING_SLOTS 127u
#define LOG_RING_SECTORS 24u
typedef struct { cellLog_s cell; uint32_t address,retry_at; uint8_t phase,waiting,result; } log_job_s;
extern log_job_s log_job;
// Сообщает о незавершённой записи события.
bool log_async_active(void);
// Копирует событие для фоновой записи.
void log_async_begin(const cellLog_s *);
// Выполняет один шаг записи или ротации.
uint8_t log_async_step(uint32_t);
// Запоминает границу успешно переданного кадра.
void log_ring_sent(uint8_t channel,uint8_t uniq,uint32_t next);
// Восстанавливает заголовки либо форматирует старый журнал.
uint8_t log_ring_init_step(void);
// Возвращает первую запись самого старого сектора.
uint32_t log_ring_first(void),log_ring_next(uint32_t);
// Запрашивает повторное восстановление состояния кольца.
void log_ring_invalidate(void);
// Упаковывает курсоры в резерв устройства перед сохранением A/B.
void log_ring_checkpoint(void),log_ring_restore(const device_s *),log_ring_reset(void),log_ring_quiet(uint8_t);
// Начинает долговечное подтверждение переданного кадра.
bool log_ring_ack(uint8_t,uint8_t),log_ring_ack_poll(void);
extern uint32_t log_cursor32[2],log_pending32[2];
#endif
