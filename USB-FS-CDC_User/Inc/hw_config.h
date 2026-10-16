// Доработка: Кряжев А., 2026
// Очередь IN, приём OUT и серийный номер платы.
// Дескрипторы и порядок событий сохраняют протокол CDC.
// Общие файлы также используются загрузчиком; память PMA требует особого доступа.

#ifndef __HW_CONFIG_H
#define __HW_CONFIG_H
#include "../../User/domovoi_diag.h"

#include "stm32f10x.h"
#include "usb_type.h"

#ifndef BOOTLOADER
/* Самый длинный ответ: 32 записи журнала, 1038 байт вместе с обрамлением. */
#include "../../User/usb_ring_size.h"
#define USART_RX_DATA_SIZE   USB_TX_RING_BYTES
#else
#define USART_RX_DATA_SIZE   512
#endif

// Настроить внутреннюю линию USB EXTI18.
void Set_System(void);
// Подать 48 МГц на USB от PLL платы.
void Set_USBClock(void);
// Назначить приоритеты и разрешить IRQ USB.
void USB_Interrupts_Config(void);
// Передать принятые USB-байты разборщику приложения.
void USB_To_USART_Send_Data(uint8_t* data_buffer, uint8_t Nb_bytes);
// Передать USB источник ответа с сохранением владения до завершения.
uint8_t USART_To_USB_Send_Data(uint8_t* data_buffer, uint16_t Nb_bytes);
// Проверить владение источником ответа.
uint8_t USB_Tx_SourceBusy(void);
#ifndef BOOTLOADER
// Ставит кадр журнала до 63 байт в очередь из 4; источник ответа не занимает.
uint8_t USB_Log_Try(const uint8_t *source, uint8_t length, uint32_t epoch);
#endif
// Разрешить новую команду после освобождения ответа и рабочего хранилища.
uint8_t USB_Tx_CommandReady(void);
// Передать ответ только в исходную USB-сессию команды.
uint8_t USB_Tx_Try(const uint8_t *source, uint16_t length);

// Освободить источник передачи и пометить смену поколения очереди.
void USB_Tx_Reset(void);
// Проверить наличие незавершённой передачи USB.
uint8_t USB_Tx_Pending(void);
// Вернуть оставшееся число байтов источника.
uint16_t USB_Tx_Queued(void);
// Выдать следующий пакет неизменяемого источника на EP1.
void Handle_USBAsynchXfer (void);
// Собрать постоянный серийный номер из заводского идентификатора МК.
void Get_SerialNum(void);

#endif
