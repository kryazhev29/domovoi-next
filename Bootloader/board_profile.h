#ifndef BOOT_BOARD_PROFILE_H
#define BOOT_BOARD_PROFILE_H
/* GPIO mappings from supplied v5 and RELEASE_ON_MINI/MODEM_PCB sources.
 * Old hardware IDs are preserved: MINI=11, MODEM/v5=139.
 * These IDs do not distinguish every chronological revision. */
#ifdef BOOT_BOARD_MINI
#define BOOT_CS_PORT GPIOA
#define BOOT_CS_PIN GPIO_Pin_4
#define BOOT_BUTTON_PORT GPIOB
#define BOOT_BUTTON_PIN GPIO_Pin_1
#define BOOT_USB_PORT GPIOB
#define BOOT_USB_PIN GPIO_Pin_14
#define BOOT_RED_PIN GPIO_Pin_9
#define BOOT_BLUE_PIN GPIO_Pin_8
#else
#define BOOT_CS_PORT GPIOB
#define BOOT_CS_PIN GPIO_Pin_12
#define BOOT_BUTTON_PORT GPIOA
#define BOOT_BUTTON_PIN GPIO_Pin_13
#define BOOT_USB_PORT GPIOA
#define BOOT_USB_PIN GPIO_Pin_8
#define BOOT_RED_PIN GPIO_Pin_8
#define BOOT_BLUE_PIN GPIO_Pin_6
#endif
#define USB_PULLUP_ON (BOOT_USB_PORT->BSRR=BOOT_USB_PIN)
#define USB_PULLUP_OFF (BOOT_USB_PORT->BRR=BOOT_USB_PIN)
#define USB_DETECT ((GPIOB->IDR&GPIO_Pin_15)!=0)
#endif
