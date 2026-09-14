#ifndef DOMOVOI_APP_UPDATE_H
#define DOMOVOI_APP_UPDATE_H
#include <stdint.h>
/* Returns zero on error; does not reset or write the bootloader automatically. */
int app_boot_request(void);
void app_boot_metadata(void);
void app_boot_error_reported(void);
#endif
