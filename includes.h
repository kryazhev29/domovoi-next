// Файл определений

#ifndef __INCLUDES_H
#define __INCLUDES_H

#include "stm32f10x.h"
#include "stm32f10x_conf.h"

#include "arm_comm.h"
#include "crc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "board.h"
#include "stm32_it.h"
#include "structures.h"
#include "user_str_fun.h"

#ifdef BOOTLOADER
  #include "bootloader.h"
#endif

// usb
#include "hw_config.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "usb_istr.h"

// периферия
#include "main.h"
#include "timers.h"
#include "adc_stm32f10x.h"
#include "uart_stm32f10x.h"
#include "wdt_stm32f10x.h"
#include "spi_stm32f10x.h"
#include "usb_stm32f10x.h"
#include "flash_w25.h"
#include "memory.h"
#include "indication.h"
#include "user_fun.h"
#include "outs.h"
#include "crc.h"

#include "wiegand.h"
/*#include "prime_reader.h"
#include "r16_reader.h"
#include "rfid.h"
#include "log.h"
#include "number_list.h"
#include "other.h"
*/
#endif

