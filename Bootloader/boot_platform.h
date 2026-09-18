/* Разработчик: Кряжев А., 2026 */
#ifndef BOOT_PLATFORM_H
#define BOOT_PLATFORM_H
#include "boot_core.h"
void boot_platform_init(void);
int boot_platform_supported(void);
int boot_button(void);
void boot_usb_init(void);
void boot_start_app(void);
uint32_t boot_millis(void);
#endif
