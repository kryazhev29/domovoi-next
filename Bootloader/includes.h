/* Разработчик: Кряжев А., 2026 */
#ifndef BOOT_INCLUDES_H
#define BOOT_INCLUDES_H
#include "stm32f10x.h"
#include "stm32f10x_conf.h"
#include <string.h>
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "hw_config.h"
#include "board_profile.h"
void usb_datareceived(uint8_t *data,uint16_t size);
#endif
