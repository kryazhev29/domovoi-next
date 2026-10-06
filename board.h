// ƒоработка:  р€жев ј., 2026
// –аспиновка платы v5 (HW 139), уровни GPIO и адреса приложени€.
// USART1 принадлежит RS-485, USART3 Ч отдельному драйверу модема.
// «аголовок 0x08008000 и векторы 0x08008200 сохран€ют договор с загрузчиком.
#ifndef BOARD_H
#define BOARD_H
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

// ќбраз платы v5 и договор с загрузчиком.
#define DEVICETYPE               (uint8_t)0x8b
#ifndef DOMOVOI_APP_HW
#define DOMOVOI_APP_HW           139
#endif
#define FWBLOCK_OFFSET           (uint32_t)0x200
#define BOOTLOADERSIZE           (uint32_t)0x8000
#define CPU_ENDMEMORYADDRESS     (uint32_t)0x0801ffff
#define PVD_RESET_MARK           0x5056u
#define APLICATION_BASE          (uint32_t)(0x08000000 + BOOTLOADERSIZE)
#define APLICATION_ADDRESS       (uint32_t)(APLICATION_BASE + FWBLOCK_OFFSET)
#define BOOT_VER                 *((uint8_t*)(APLICATION_BASE - 4))
#define BOOTHW_VER               *((uint8_t*)(APLICATION_BASE - 3))

// –еле: активна€ единица, индексы 0 и 1.
#define OUT1                     0
#define OUT2                     1
#define OUT1_ON                  GPIOB->BSRR = GPIO_Pin_3
#define OUT1_OFF                 GPIOB->BRR = GPIO_Pin_3
#define OUT1_IN                  ((GPIOB->ODR & GPIO_Pin_3) != (uint32_t)Bit_RESET)
#define OUT2_ON                  GPIOA->BSRR = GPIO_Pin_0
#define OUT2_OFF                 GPIOA->BRR = GPIO_Pin_0
#define OUT2_IN                  ((GPIOA->ODR & GPIO_Pin_0) != (uint32_t)Bit_RESET)

// —ветодиоды: питание и сеть имеют рисунки, данные переключает приложение.
#define PWR_LED                  0
#define NET_LED                  1
#define LED1_ON                  GPIOB->BSRR = GPIO_Pin_8
#define LED1_OFF                 GPIOB->BRR = GPIO_Pin_8
#define LED2_ON                  GPIOB->BSRR = GPIO_Pin_7
#define LED2_OFF                 GPIOB->BRR = GPIO_Pin_7
#define LED3_ON                  GPIOB->BSRR = GPIO_Pin_6
#define LED3_OFF                 GPIOB->BRR = GPIO_Pin_6

// ћодем USART3: PC13 Ч питание, PC14 Ч ключ, PB13/PB14 Ч CTS/RTS.
#define GSM_PWR_ON               GPIOC->BSRR = GPIO_Pin_13
#define GSM_PWR_OFF              GPIOC->BRR = GPIO_Pin_13
#define GSM_KEY_ON               GPIOC->BRR = GPIO_Pin_14
#define GSM_KEY_OFF              GPIOC->BSRR = GPIO_Pin_14
#define RTSG_HIGH                GPIOB->BSRR = GPIO_Pin_14
#define RTSG_LOW                 GPIOB->BRR = GPIO_Pin_14
#define CTSG                     ((GPIOB->IDR & GPIO_Pin_13) == (uint32_t)Bit_RESET)

// USB: PA11/PA12 Ч шина, PA8 Ч подт€жка, PB15 Ч детект кабел€.
#define USB_PULLUP_ON            GPIOA->BSRR = GPIO_Pin_8
#define USB_PULLUP_OFF           GPIOA->BRR = GPIO_Pin_8
#define USB_DETECT               ((GPIOB->IDR & GPIO_Pin_15) != (uint32_t)Bit_RESET)

// јналоговые входы PB0/PB1 Ч каналы 8/9; PA4 и приЄмник 433 не используютс€.
#define IN1_CHANEL               8u
#define IN2_CHANEL               9u

// ¬нешн€€ W25: PA5/PA6/PA7 Ч SPI1, PB12 Ч выбор флеш.
#define SPI_CS_PIN               GPIO_Pin_12
#define SPI_CS_PORT              GPIOB
#define SPI_MOSI_SCK             GPIO_Pin_5 | GPIO_Pin_7
#define SPI_MISO                 GPIO_Pin_6
#define SPI_PORT                 GPIOA
#define CSEL_HIGH                GPIOB->BSRR = GPIO_Pin_12
#define CSEL_LOW                 GPIOB->BRR = GPIO_Pin_12

// Ќастроить выбранные линии EXTI при закрытых масках.
void board_exti_config(uint32_t lines, uint8_t falling);
// –азрешить IRQ с готовым приоритетом CMSIS.
void board_irq_enable(uint8_t irq, uint8_t priority);
// Ќастроить входы, безопасные уровни выходов и детектор питани€.
void board_init(void);
// —бросить GPIO и AFIO перед переходом к загрузчику.
void board_deinit(void);
// «апросить системный сброс.
void board_reset(void);
// ќграниченно ждать восстановление питани€, обслужива€ сторож.
void board_waitPowerStability(void);
#endif
