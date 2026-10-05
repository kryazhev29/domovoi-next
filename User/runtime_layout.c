#define PHOTO_RX 64u
#include "includes.h"
typedef struct {
  uint32_t stamp, address, deadline, started, last;
  uint32_t readAddress;
  uint16_t size, written, offset, packet, used;
  uint8_t stage, retry, syncAck, syncSeen, pending, requestId;
  uint16_t rxBytes; // диагностика связи с камерой: байт с 485 за съёмку
  uint8_t rxHead[8]; // и первые из них
  dataChanel_e channel;
  uint8_t rx[PHOTO_RX], frame[PHOTO_RX];
} photo_s;
__root const device_s * const asm_debug_device_s = 0;
__root const config_s * const asm_debug_config_s = 0;
__root const photo_s * const asm_debug_photo_s = 0;
__root const cellID_s * const asm_debug_cellID_s = 0;
__root const cellGroup_s * const asm_debug_cellGroup_s = 0;
__root const cellShedule_s * const asm_debug_cellShedule_s = 0;
__root const cellHoliday_s * const asm_debug_cellHoliday_s = 0;
__root const photoParam_s * const asm_debug_photoParam_s = 0;
__root const bootFWparam_s * const asm_debug_bootFWparam_s = 0;
#include "sector_runtime.h"
__root const sj_runtime_state_s * const asm_debug_sjr = 0;
#include "photo_runtime.h"
__root const photo_runtime_s * const asm_debug_photo_runtime = 0;
