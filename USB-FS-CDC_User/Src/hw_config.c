// Доработка: Кряжев А., 2026
// Очередь IN, приём OUT и серийный номер платы.
// Дескрипторы и порядок событий сохраняют протокол CDC.
// Общие файлы также используются загрузчиком; память PMA требует особого доступа.

// Доработка: Кряжев А., 2026
// USB CDC платы v5: hw_config.
// Модуль сохраняет дескрипторы, порядок событий и двоичный протокол приложения.
// Диагностические счётчики отделены от обязательных таймаутов восстановления.
#ifndef BOOTLOADER
#include "../../User/domovoi_diag.h"
#include "../../User/c/runtime_internal.h"
#endif
// Доработка: Кряжев А., 2026

#include "includes.h"

#include "stm32_it.h"
#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "hw_config.h"
#include "usb_pwr.h"

#define         ID1          (0x1FFFF7E8)
#define         ID2          (0x1FFFF7EC)
#define         ID3          (0x1FFFF7F0)

static const uint8_t * volatile usb_tx_source;
static volatile uint16_t usb_tx_length, usb_tx_offset;
#ifndef BOOTLOADER
/* Очередь журнала: кладёт главный цикл, забирает EP1 при отсутствии ответа.
   [0] - длина кадра до 63 байт: короткий пакет хост выдаёт сразу. */
static uint8_t usb_log_slots[4][64];
static volatile uint8_t usb_log_head, usb_log_tail;
static void usb_log_next(void);
#endif

volatile uint8_t  USB_Tx_State = 0;
volatile uint32_t usb_in_armed = 0; /* пакеты IN, отданные EP1, команда 111 */
static volatile uint32_t usb_tx_epoch = 0;
static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len);

// Настроить внутреннюю линию USB EXTI18.
void Set_System(void)
{
  EXTI->PR = EXTI_PR_PR18;
  board_exti_config(EXTI_IMR_MR18, 0);
}

// Подать 48 МГц на USB от PLL платы.
void Set_USBClock(void)
{
  if (SystemCoreClock == 72000000u) RCC->CFGR &= ~RCC_CFGR_USBPRE;
  else RCC->CFGR |= RCC_CFGR_USBPRE;
  RCC->APB1ENR |= RCC_APB1ENR_USBEN;
}

// Назначить приоритеты и разрешить IRQ USB.
void USB_Interrupts_Config(void)
{
  board_irq_enable(USB_LP_CAN1_RX0_IRQn, 0x80u);
  board_irq_enable(USBWakeUp_IRQn, 0x80u);
}

// Передать принятые USB-байты разборщику приложения.
void USB_To_USART_Send_Data(uint8_t *data_buffer, uint8_t Nb_bytes)
{
  usb_datareceived(data_buffer, Nb_bytes);
}

// Выдать следующий пакет неизменяемого источника на EP1.
void Handle_USBAsynchXfer (void)
{
  uint16_t count,offset; const uint8_t *source;
  if (USB_Tx_State || bDeviceState != CONFIGURED) return;
  if (!usb_tx_source) {
#ifndef BOOTLOADER
    usb_log_next();
#endif
    return;
  }
  source=usb_tx_source; count=usb_tx_length; offset=usb_tx_offset; count-=offset;
  if(!count) { /* Хост подтвердил последний пакет IN. */
    usb_tx_source=NULL;
#ifndef BOOTLOADER
    usb_log_next();
#endif
    return;
  }
  if(count>VIRTUAL_COM_PORT_DATA_SIZE) count=VIRTUAL_COM_PORT_DATA_SIZE;
  UserToPMABufferCopy((uint8_t*)source+offset,ENDP1_TXADDR,count);
  usb_tx_offset+=count;
  USB_Tx_State=1;
#if DOMOVOI_DIAG
  usb_in_armed++;
#endif
  SetEPTxCount(ENDP1,count); SetEPTxValid(ENDP1);
}
// Освободить источник передачи и пометить смену поколения очереди.
void USB_Tx_Reset(void)
{
  USB_Tx_State=0; usb_tx_source=NULL;
  usb_tx_length=0; usb_tx_offset=0; usb_tx_epoch++;
#ifndef BOOTLOADER
  usb_log_tail=usb_log_head;
#endif
}
// Проверить, продолжает ли USB владеть данным источником.
uint8_t USB_Tx_SourceBusy(void) { return usb_tx_source!=NULL; }
#ifndef BOOTLOADER
// Отдать в EP1 следующий кадр журнала; только при свободной EP1 и без ответа.
static void usb_log_next(void)
{
  const uint8_t *slot;
  if (usb_log_tail == usb_log_head) return;
  slot = usb_log_slots[usb_log_tail & 3u];
  UserToPMABufferCopy((uint8_t *)slot + 1, ENDP1_TXADDR, slot[0]);
  usb_log_tail++;
  USB_Tx_State = 1;
#if DOMOVOI_DIAG
  usb_in_armed++;
#endif
  SetEPTxCount(ENDP1, slot[0]);
  SetEPTxValid(ENDP1);
}
// Поставить кадр журнала в очередь; ответ, занявший источник, уйдёт раньше неё.
/* Проверено на плате №1 14.10.2026 (0e107dc): 363 кадра без потерь, статус 59/59, чтение настроек 106 при журнале. */
uint8_t USB_Log_Try(const uint8_t *source, uint8_t length, uint32_t epoch)
{
  extern volatile uint32_t usb_bus_resets;
  uint32_t mask = __get_PRIMASK();
  uint8_t accepted = 0;
  __disable_irq();
  if (bDeviceState == CONFIGURED && epoch == usb_bus_resets && (uint8_t)(usb_log_head - usb_log_tail) < 4u) {
    uint8_t *slot = usb_log_slots[usb_log_head & 3u];
    slot[0] = length;
    memcpy(slot + 1, source, length);
    usb_log_head++;
    accepted = 1;
    if (!USB_Tx_State && !usb_tx_source) usb_log_next();
  }
  __set_PRIMASK(mask);
  return accepted;
}
#endif
#include "work_memory.h"

// Атомарно принять источник целиком, если прежняя передача закончена.
static uint8_t usb_tx_submit(const uint8_t *source,uint16_t length,uint32_t epoch)
{
  const uint8_t *owned;
  if(bDeviceState!=CONFIGURED || epoch!=usb_tx_epoch) return 0;
  if(!length) return 1;
  if(usb_tx_source) return 0;
  owned=work_usb_source(source,length); if(!owned) return 0;
  usb_tx_length=length; usb_tx_offset=0;
  __DMB(); usb_tx_source=owned; return 1;
}
// Передать USB источник ответа с сохранением владения до завершения.
uint8_t USART_To_USB_Send_Data(uint8_t* data_buffer,uint16_t Nb_bytes)
{
  uint32_t mask=__get_PRIMASK(),epoch=usb_tx_epoch; uint8_t result;
  if(Nb_bytes>=USART_RX_DATA_SIZE || (Nb_bytes && !data_buffer)) return 0;
  __disable_irq();
  result=usb_tx_submit(data_buffer,Nb_bytes,epoch);
  __set_PRIMASK(mask);
  return result;
}
/* RX сохраняет кадр, пока хост не подтвердит предыдущий источник ответа. */
// Разрешить новую команду только после освобождения прежнего ответа.
uint8_t USB_Tx_CommandReady(void)
{
  return !USB_Tx_SourceBusy();
}

// Передать ответ только в исходную USB-сессию, сохраняя источник при занятости.
uint8_t USB_Tx_Try(const uint8_t *source,uint16_t length)
{
  extern uint8_t usb_enabled;
  extern volatile uint32_t usb_bus_resets;
  uint32_t mask,epoch; uint8_t result=0;
  if(length>=USART_RX_DATA_SIZE || (length && !source) || !usb_enabled || !(GPIOB->IDR&(1u<<15))) return 0;
  mask=__get_PRIMASK(); __disable_irq(); epoch=store.usb_epoch;
  if(epoch==usb_bus_resets) result=usb_tx_submit(source,length,usb_tx_epoch);
  __set_PRIMASK(mask); return result;
}
#if DOMOVOI_DIAG
// Принять диагностический текст в ту же очередь USB.

#endif
// Проверить наличие незавершённой передачи USB.
uint8_t USB_Tx_Pending(void) { return USB_Tx_State || usb_tx_source!=NULL; }
// Вернуть оставшееся число байтов источника.
uint16_t USB_Tx_Queued(void)
{
  uint32_t mask=__get_PRIMASK(); uint16_t length,offset;
  __disable_irq();
  length=usb_tx_source ? usb_tx_length : 0; offset=usb_tx_offset;
  __set_PRIMASK(mask); return length ? length-offset : 0;
}

// Собрать постоянный серийный номер из заводского идентификатора МК.
void Get_SerialNum(void)
{
  uint32_t Device_Serial0, Device_Serial1, Device_Serial2;

  Device_Serial0 = *(uint32_t*)ID1;
  Device_Serial1 = *(uint32_t*)ID2;
  Device_Serial2 = *(uint32_t*)ID3;

  Device_Serial0 += Device_Serial2;

  if (Device_Serial0 != 0)
  {
    IntToUnicode (Device_Serial0, &Virtual_Com_Port_StringSerial[2] , 8);
    IntToUnicode (Device_Serial1, &Virtual_Com_Port_StringSerial[18], 4);
  }
}

// Записать шестнадцатеричные цифры в строку USB UTF-16LE.
static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len)
{
  uint8_t idx = 0;

  for( idx = 0 ; idx < len ; idx ++)
  {
    if( ((value >> 28)) < 0xA )
    {
      pbuf[ 2* idx] = (value >> 28) + '0';
    }
    else
    {
      pbuf[2* idx] = (value >> 28) + 'A' - 10;
    }

    value = value << 4;

    pbuf[ 2* idx + 1] = 0;
  }
}
