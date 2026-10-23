// Один исполнитель принятых работ флеш; следующий запрос ждёт у источника.
#ifndef FLASH_EXEC_H
#define FLASH_EXEC_H
#include <stdint.h>
#include <stdbool.h>
typedef uint8_t flash_kind_t;
enum { FX_NONE, FX_IDCLEAR, FX_EXPIRY, FX_FWSERVICE, FX_UPLOAD,
       FX_INDEX, FX_CRC, FX_TABLE, FX_PHOTO, FX_FWBLOCK, FX_LOG,
       FX_CONFIG, FX_STORE, FX_FACTORY };
typedef uint8_t flash_result_t;
enum { FX_IDLE, FX_PENDING, FX_OK, FX_ERROR, FX_CANCELLED };
typedef struct flash_exec_args flash_exec_args_t;
bool flash_exec_begin(flash_kind_t kind, const flash_exec_args_t *args);
void flash_exec_poll(uint32_t now);
bool flash_exec_idle(void);
bool flash_exec_read_allowed(void);
flash_result_t flash_exec_take_result(void);
void flash_exec_cancel(void);
void flash_exec_quiet(uint8_t channel);
#endif
