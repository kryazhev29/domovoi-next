#ifndef DOMOVOI_BOOT_MANUAL_H
#define DOMOVOI_BOOT_MANUAL_H
#include <stdint.h>
#define MANUAL_BOOT_SIZE 32768u
#define MANUAL_BOOT_COMMAND 110u
enum { MANUAL_OK=0, MANUAL_BAD=1, MANUAL_IO=2, MANUAL_BUSY=3,
       MANUAL_DENIED=4, MANUAL_CRC=5 };
/* USB service command payload: op, then op-specific data. Response: 8 bytes.
 * 0 BEGIN(size:u32,crc:u32,selectedHW:u8), 1 DATA(offset:u32,1..256 bytes),
 * 2 COMMIT, 3 CANCEL, 4 STATUS. All integer fields little-endian.
 * COMMIT only arms a volatile request; apply is explicit, never on startup. */
void boot_manual_command(int usb,const uint8_t *p,uint16_t n,uint8_t reply[8]);
int boot_manual_busy(void);
int boot_manual_apply(void);
/* Board adapter. All stage offsets are within the candidate slot. */
int manual_stage_read(uint32_t off,void *data,uint32_t n);
int manual_stage_write(uint32_t off,const void *data,uint32_t n);
int manual_stage_erase(uint32_t off);
int manual_platform_supported(void);
/* Nonzero when a bootloader for this HW ID matches the board the running
 * application is built for. A wrong file chosen by the board photo is refused. */
int manual_board_hw(uint8_t hw);
int manual_program(uint32_t expected_crc);
void manual_service(void);
#endif
