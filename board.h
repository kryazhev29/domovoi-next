// Доработка: Кряжев А., 2026
// Распиновка платы v5 (HW 139), уровни GPIO и адреса приложения.
// USART1 принадлежит RS-485, USART3 — отдельному драйверу модема.
// Заголовок 0x08008000 и векторы 0x08008200 сохраняют договор с загрузчиком.

#ifndef BOARD_H
#define BOARD_H

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"


/*************************************************************************/

#define DEVICETYPE                       (uint8_t)0x8b

// Номер платы в образе (DOMOVOI_APP_HW): 139 - плата v5. Пока прошивка только для v5
// (решение владельца 03.10.2026); другие платы - единой прошивкой вместе с драйвером M66.
// Доработка: Кряжев А., 2026
#ifndef DOMOVOI_APP_HW
#define DOMOVOI_APP_HW                   139
#endif
#define FWBLOCK_OFFSET                  (uint32_t)0x200 /* 512*/
#define BOOTLOADERSIZE                  (uint32_t)0x8000 /* 32768 байт*/
#define CPU_ENDMEMORYADDRESS            (uint32_t)0x0801ffff

/* BKP_DR1 свободен в приложении и загрузчике; метка причины сброса по питанию. */
#define PVD_RESET_MARK                  0x5056u

#define APLICATION_BASE                 (uint32_t)(0x08000000 + BOOTLOADERSIZE)
#define APLICATION_ADDRESS              (uint32_t)(APLICATION_BASE + FWBLOCK_OFFSET)

#define BOOT_VER                        *((uint8_t*)(APLICATION_BASE - 4)) /* версия загрузчика (предпоследний 4-й байт конца области загрузчика)*/
#define BOOTHW_VER                      *((uint8_t*)(APLICATION_BASE - 3)) /* версия железа прописанная в загрузчике*/

/******************************************************************************/

/******************************************************************************/
/************************** Индексы *******************************************/
#define OUT1            0
#define OUT2            1

#define PWR_LED         0
#define NET_LED         1
#define DATA_LED        2

/******************************************************************************/
/************************** Описание входов / выходов *************************/
#define PORT1_PORT      (uint8_t)0 /*485*/

#define GSM_KEY_ON      GPIOC->BRR = GPIO_Pin_14 /* выход: управление включением GSM (активный 0)*/
#define GSM_KEY_OFF     GPIOC->BSRR = GPIO_Pin_14 /* выход: управление включением GSM*/

#define GSM_PWR_ON      GPIOC->BSRR = GPIO_Pin_13 /* выход управления питанием GSM*/
#define GSM_PWR_OFF     GPIOC->BRR = GPIO_Pin_13 /* выход управления питанием GSM*/

#define OUT1_ON 	GPIOB->BSRR = GPIO_Pin_3
#define OUT1_OFF	GPIOB->BRR = GPIO_Pin_3
#define OUT1_IN         ((GPIOB->ODR & GPIO_Pin_3) != (uint32_t)Bit_RESET)

#define OUT2_ON 	GPIOA->BSRR = GPIO_Pin_0
#define OUT2_OFF	GPIOA->BRR = GPIO_Pin_0
#define OUT2_IN         ((GPIOA->ODR & GPIO_Pin_0) != (uint32_t)Bit_RESET)

#define BOOT_PIN        ((GPIOA->IDR & GPIO_Pin_13) == (uint32_t)Bit_RESET) /* ножка swdio */

#define USB_PULLUP_ON   GPIOA->BSRR = GPIO_Pin_8 /* включить подтягивающий резистор*/
#define USB_PULLUP_OFF  GPIOA->BRR = GPIO_Pin_8 /* выключить подтягивающий резистор*/
#define USB_DETECT      ((GPIOB->IDR & GPIO_Pin_15) != (uint32_t)Bit_RESET)

// Порт RS485
// P_TX A9, P_RX A10

// Порт USB
// DM A11, DP A12

#define LED1_ON         GPIOB->BSRR = GPIO_Pin_8
#define LED1_OFF        GPIOB->BRR = GPIO_Pin_8

#define LED2_ON         GPIOB->BSRR = GPIO_Pin_7
#define LED2_OFF        GPIOB->BRR = GPIO_Pin_7

#define LED3_ON         GPIOB->BSRR = GPIO_Pin_6
#define LED3_OFF        GPIOB->BRR = GPIO_Pin_6

#define CSEL_HIGH	GPIOB->BSRR = GPIO_Pin_12
#define CSEL_LOW	GPIOB->BRR = GPIO_Pin_12

// Приемник 433
// PA1 - 433RX

// Считыватель Wiegand
// PA2 - WG26 D0
// PA3 - WG26 D1

// Порт GSM
// PB10 - Tx
// PB11 - Rx
// PB14 - RTS GSM
#define RTSG_HIGH	GPIOB->BSRR = GPIO_Pin_14
#define RTSG_LOW	GPIOB->BRR = GPIO_Pin_14
// PB13 - CTS GSM
#define CTSG            ((GPIOB->IDR & GPIO_Pin_13) == (uint32_t)Bit_RESET)

// Внешнее питание (аналог)
// PA4 - IN4
#define VCC_PIN         GPIO_Pin_4
#define VCC_PORT        GPIOA
#define VCC_CHANEL      4u

// Входы IN1 и IN2 (аналог)
// PB0, PB1
#define IN1_PIN         GPIO_Pin_0
#define IN1_PORT        GPIOB
#define IN1_CHANEL      8u

#define IN2_PIN         GPIO_Pin_1
#define IN2_PORT        GPIOB
#define IN2_CHANEL      9u

// Память
// PA5:PA7 - SPI1
#define SPI_CS_PIN      GPIO_Pin_12
#define SPI_CS_PORT     GPIOB
#define SPI_MOSI_SCK    GPIO_Pin_5 | GPIO_Pin_7
#define SPI_MISO        GPIO_Pin_6
#define SPI_PORT        GPIOA

// Настроить проверенную маску EXTI с прежним порядком отключения и включения.
void board_exti_config(uint32_t lines, uint8_t falling);
// Разрешить внутреннее IRQ с уже рассчитанным приоритетом CMSIS.
void board_irq_enable(uint8_t irq, uint8_t priority);

// инициализация
void board_init (void);

// деинициализировать периферию
void board_deinit (void);

// перезагрузка
void board_reset (void);

// ожидание стабилизации питания
void board_waitPowerStability (void);

#endif