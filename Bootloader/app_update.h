#ifndef DOMOVOI_APP_UPDATE_H
#define DOMOVOI_APP_UPDATE_H
#include <stdint.h>
/* Returns zero on error; does not reset or write the bootloader automatically. */
int app_boot_request(void);
void app_boot_migrate_if_needed(void);
void app_boot_metadata(void);
/* Explicit migration API. image is a full padded 32 KiB bootloader binary.
 * Interruptions of power during this operation require SWD recovery. */
int app_boot_replace(const uint8_t *image,uint32_t size,uint32_t crc);
#endif
