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
/* v1 CAN transceiver drives PB8/RXD. LEDs share the CAN nets: neither
 * pin may be toggled for indication. Keep TXD recessive and RXD input. */
#define BOOT_LED_PINS 0u
#define BOOT_CAN_RX_PIN GPIO_Pin_8
#define BOOT_CAN_TX_PIN GPIO_Pin_9
#else
#define BOOT_CS_PORT GPIOB
#define BOOT_CS_PIN GPIO_Pin_12
#define BOOT_BUTTON_PORT GPIOA
#define BOOT_BUTTON_PIN GPIO_Pin_13
#define BOOT_USB_PORT GPIOA
#define BOOT_USB_PIN GPIO_Pin_8
#define BOOT_LED_PRIMARY_PIN GPIO_Pin_8 /* PWRLED */
#define BOOT_LED_SECONDARY_PIN GPIO_Pin_6 /* DATALED; colour is board-dependent */
#define BOOT_LED_PINS (BOOT_LED_PRIMARY_PIN|BOOT_LED_SECONDARY_PIN)
#endif
#define USB_PULLUP_ON (BOOT_USB_PORT->BSRR=BOOT_USB_PIN)
#define USB_PULLUP_OFF (BOOT_USB_PORT->BRR=BOOT_USB_PIN)
#define USB_DETECT ((GPIOB->IDR&GPIO_Pin_15)!=0)
#endif
