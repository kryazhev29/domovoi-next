/* Разработчик: Кряжев А., 2026 */
#ifndef BOOT_BOARD_PROFILE_H
#define BOOT_BOARD_PROFILE_H
/* Назначение выводов по присланным исходникам v5 и RELEASE_ON_MINI/MODEM_PCB. Старые номера
 * железа сохранены: MINI=11, v5=139, MODEM=140 (с 29.09.2026 свой номер, Кряжев А.). Эти номера различают не все ревизии плат. */
#ifdef BOOT_BOARD_MINI
#define BOOT_CS_PORT GPIOA
#define BOOT_CS_PIN GPIO_Pin_4
#define BOOT_BUTTON_PORT GPIOB
#define BOOT_BUTTON_PIN GPIO_Pin_1
#define BOOT_USB_PORT GPIOB
#define BOOT_USB_PIN GPIO_Pin_14
/* На v1 трансивер CAN управляет PB8/RXD. Светодиоды сидят на линиях CAN: ни один из этих
 * выводов нельзя переключать для индикации. TXD держать в пассивном уровне, RXD - входом. */
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
#define BOOT_LED_SECONDARY_PIN GPIO_Pin_6 /* DATALED; цвет зависит от платы */
#define BOOT_LED_PINS (BOOT_LED_PRIMARY_PIN|BOOT_LED_SECONDARY_PIN)
#endif
#define USB_PULLUP_ON (BOOT_USB_PORT->BSRR=BOOT_USB_PIN)
#define USB_PULLUP_OFF (BOOT_USB_PORT->BRR=BOOT_USB_PIN)
#define USB_DETECT ((GPIOB->IDR&GPIO_Pin_15)!=0)
#endif
