// ƒоработка:  р€жев ј., 2026
// ќбщие внутренние контракты автоматов хранилища.
// —труктура store сохран€ет размещение данных гибридного ASM.
// √отовые аргументы не провер€ютс€ повторно; владение флешем и рабочей пам€тью об€зательно.
#ifndef C_RUNTIME_INTERNAL_H
#define C_RUNTIME_INTERNAL_H
#include "includes.h"
#include "../work_memory.h"
#include "../packet.h"
#include "idindex.h"
#include "sector_runtime.h"
#include "../asm/flash_owner.h"
#include "../../Modules/comm_channel.h"
#include <stddef.h>
// —осто€ние сохранени€ из user_fun; пор€док полей сохран€ет контракт гибридного ASM.
typedef struct {
  uint32_t erase, end, retry_at, usb_epoch;
  uint16_t cursor;
  uint8_t job, clear, waiting, cmd, channel, uniq, quiet, repeat;
  uint8_t last_id[4], seen;
} store_state_s;
typedef char store_state_size[(sizeof(store_state_s)==32)?1:-1];
typedef char store_state_job[(offsetof(store_state_s,job)==18)?1:-1];
typedef char store_state_command[(offsetof(store_state_s,cmd)==21)?1:-1];
typedef char store_state_channel[(offsetof(store_state_s,channel)==22)?1:-1];
typedef char store_state_uniq[(offsetof(store_state_s,uniq)==23)?1:-1];
typedef char store_state_quiet[(offsetof(store_state_s,quiet)==24)?1:-1];
enum { STORE_JOB_COMMAND = 9 };
extern uint8_t flash_owner, store_runtime, store_reset_requested;
extern device_s device;
extern config_s config;
extern bool mem_storage_ready;
extern uint32_t id_used_bytes, cfg_cached_active;
extern uint16_t id_cells_end;
uint8_t flash_claim(uint8_t owner);
uint8_t flash_release(uint8_t owner);
uint8_t flash_read_allowed(void);
void idindex_runtime_suspend(void);
void idindex_table_commit(uint32_t delta, uint16_t end);
bool idindex_runtime_active(void);
uint8_t *user_comm_acquire(uint16_t n);
uint8_t usb_try_send(const uint8_t *packet, uint16_t size);
void *service_crc_active(void);
uint8_t service_crc_begin_pack(uint8_t,uint8_t);
uint8_t service_crc_begin_id(uint8_t,uint8_t,uint8_t);
uint8_t service_crc_begin_config(uint8_t,uint8_t);
void service_crc_poll(void),service_crc_cancel(void),service_crc_quiet(uint8_t);
bool expiry_runtime_active(void),expiry_runtime_begin(void);
void expiry_runtime_poll(uint32_t),expiry_runtime_cancel(void);
uint8_t idclear_runtime_active(void),idclear_runtime_begin(uint8_t,uint8_t);
void idclear_runtime_poll(uint32_t),idclear_runtime_cancel(void),idclear_runtime_quiet(uint8_t);
bool upload_runtime_active(void),upload_runtime_begin(uint8_t,dataChanel_e,uint8_t);
void upload_runtime_poll(void),upload_runtime_cancel(void),upload_runtime_quiet(uint8_t);
void *fw_service_busy(void);
bool fw_service_active(void),fw_service_begin(const uint8_t *,dataChanel_e,uint8_t);
void fw_service_poll(void),fw_service_cancel(void),fw_service_quiet(uint8_t),fw_service_config_result(uint8_t);
// «аписывает 16 бит в пор€дке байтов протокола.
static inline void rt_put16(uint8_t *p, uint16_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
// «аписывает 32 бита без требовани€ выравнивани€.
static inline void rt_put32(uint8_t *p, uint32_t v) { memcpy(p,&v,4); }
// „итает 16 бит из проверенного тела кадра.
static inline uint16_t rt_get16(const uint8_t *p) { uint16_t v; memcpy(&v,p,2); return v; }
// „итает 32 бита из проверенного тела кадра.
static inline uint32_t rt_get32(const uint8_t *p) { uint32_t v; memcpy(&v,p,4); return v; }
#include "../id_crc_io.h"
#define RT_ASSERT(name, condition) typedef char name[(condition)?1:-1]
#endif
