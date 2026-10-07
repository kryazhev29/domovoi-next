// Доработка: Кряжев А., 2026
// Общие внутренние контракты автоматов хранилища.
// Общий тип store используется сохранением настроек и сервисом прошивки.
// Готовые аргументы не проверяются повторно; владение флешем и рабочей памятью обязательно.
#ifndef C_RUNTIME_INTERNAL_H
#define C_RUNTIME_INTERNAL_H
#include "includes.h"
#include "../work_memory.h"
#include "../packet.h"
#include "idindex.h"
#include "sector_runtime.h"
#include "../asm/flash_owner.h"
#include "../flash_exec.h"
struct flash_exec_args { void *context; uint8_t step; };
enum { FX_INDEX_READ = 7 };
// Источники и продолжения принятой работы живут до verify и снятия результата.
typedef struct {
  uint8_t *records;
  uint8_t mb, cmd, channel, uniq, quiet, cancelled, failed, reply_wait, scan_full;
  // Количество приходит одним байтом; batch/position не превышают count.
  uint8_t count, position, batch, cell_size;
  uint16_t bytes, day, crc, numbers, last;
  uint32_t stamp, sector, start, offset, sum, used, reply_until, crc_limit;
} table_job_s;
typedef struct {
  uint8_t *data;
  uint8_t channel, id, quiet, cancelled, code, final, reason;
  uint16_t offset, block;
  uint32_t address, position, remaining, crc, expected_crc, usb_epoch;
} fw_block_job_s;
typedef struct {
  cellLog_s cell;
  uint32_t address, retry_at;
  uint8_t after;
} log_job_s;
typedef struct {
  uint8_t after, task, quiet, cancelled, committed, address, reply_wait;
  // Источник камеры не больше 58 Б, заголовок/ACK ещё меньше.
  uint8_t io_len, io_done, io_step;
  uint16_t offset, count, requested_offset, first_offset, target_done;
  uint16_t send_size, send_offset, send_crc;
  uint32_t io_address, scan, h[4], target, first, max_number, requested_number, reply_until;
} photo_job_s;
typedef struct {
  uint8_t kind, step, result;
  union {
    void *context;
    struct { uint32_t source[2], bucket; } index;
    table_job_s table;
    photo_job_s photo;
    fw_block_job_s fwblock;
    log_job_s log;
  } data;
} flash_exec_job_t;
extern flash_exec_job_t flash_job;
void flash_exec_finish(flash_result_t result);
// Временные переходы прежних владельцев, удаляемые при переносе их автоматов.
bool flash_exec_legacy_claim(uint8_t owner);
void flash_exec_legacy_release(uint8_t owner);
void flash_exec_legacy_yield_store(void);
#include "../../Modules/comm_channel.h"
// Состояние владельца настроек из user_store.c.
typedef struct {
  uint32_t erase, end, retry_at, usb_epoch;
  uint16_t cursor;
  uint8_t job, clear, waiting, cmd, channel, uniq, quiet, repeat;
  uint8_t last_id[4], seen;
} store_state_s;
enum {
  STORE_JOB_COMMAND = 9
};
extern uint8_t flash_owner, store_runtime, store_reset_requested;
extern store_state_s store;
extern device_s device;
extern config_s config;
extern bool mem_storage_ready;
extern uint32_t id_used_bytes, cfg_cached_active;
extern uint16_t id_cells_end;
uint8_t flash_claim(uint8_t owner);
uint8_t flash_release(uint8_t owner);
uint8_t flash_read_allowed(void);
void idindex_runtime_suspend(void);
bool idindex_runtime_begin(void);
bool idcrc_background_begin(void);
void idcrc_background_poll(void);
void idindex_table_commit(uint32_t delta, uint16_t end);
uint8_t *user_comm_acquire(uint16_t n);
uint8_t usb_try_send(const uint8_t *packet, uint16_t size);
uint8_t service_crc_begin_pack(uint8_t,uint8_t);
uint8_t service_crc_begin_id(uint8_t,uint8_t,uint8_t);
uint8_t service_crc_begin_config(uint8_t,uint8_t);
void service_crc_poll(void),service_crc_cancel(void),service_crc_quiet(uint8_t);
bool expiry_runtime_begin(void);
void expiry_runtime_poll(uint32_t),expiry_runtime_cancel(void);
uint8_t idclear_runtime_begin(uint8_t,uint8_t);
void idclear_runtime_poll(uint32_t),idclear_runtime_cancel(void),idclear_runtime_quiet(uint8_t);
bool upload_runtime_begin(uint8_t,dataChanel_e,uint8_t);
void upload_runtime_poll(void),upload_runtime_cancel(void),upload_runtime_quiet(uint8_t);
void *fw_service_busy(void);
bool fw_service_store_waiting(void);
bool fw_service_begin(const uint8_t *,dataChanel_e,uint8_t);
void fw_service_poll(void),fw_service_cancel(void),fw_service_quiet(uint8_t),fw_service_config_result(uint8_t);
// Записывает 16 бит в порядке байтов протокола.
static inline void rt_put16(uint8_t *p, uint16_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
// Записывает 32 бита без требования выравнивания.
static inline void rt_put32(uint8_t *p, uint32_t v) { memcpy(p,&v,4); }
// Читает 16 бит из проверенного тела кадра.
static inline uint16_t rt_get16(const uint8_t *p) { uint16_t v; memcpy(&v,p,2); return v; }
// Читает 32 бита из проверенного тела кадра.
static inline uint32_t rt_get32(const uint8_t *p) { uint32_t v; memcpy(&v,p,4); return v; }
#include "../id_crc_io.h"
#define RT_ASSERT(name, condition) typedef char name[(condition)?1:-1]
#endif
