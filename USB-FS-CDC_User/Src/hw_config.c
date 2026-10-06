// Доработка: Кряжев А., 2026
// USB CDC платы v5: hw_config.
// Модуль сохраняет дескрипторы, порядок событий и двоичный протокол приложения.
// Диагностические счётчики отделены от обязательных таймаутов восстановления.
#ifndef BOOTLOADER
#include "../../User/domovoi_diag.h"
#endif
// Доработка: Кряжев А., 2026
/**
  ******************************************************************************
  * @file    hw_config.c
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   Hardware Configuration & Setup
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

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


USART_InitTypeDef USART_InitStructure;

static const uint8_t * volatile usb_tx_source;
static volatile uint16_t usb_tx_length, usb_tx_offset;

volatile uint8_t  USB_Tx_State = 0;
volatile uint32_t usb_in_armed = 0; /* пакеты IN, отданные EP1, команда 111 */
static volatile uint32_t usb_tx_epoch = 0;
static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len);

extern LINE_CODING linecoding;


// Настроить внутреннюю линию USB wake-up EXTI18.
void Set_System(void)
{
  EXTI->PR = EXTI_PR_PR18;
  board_exti_config(EXTI_IMR_MR18, 0);
}


// Подать 48 МГц на USB с учётом частоты PLL платы.
void Set_USBClock(void)
{
  if (SystemCoreClock == 72000000u) RCC->CFGR &= ~RCC_CFGR_USBPRE;
  else RCC->CFGR |= RCC_CFGR_USBPRE;
  RCC->APB1ENR |= RCC_APB1ENR_USBEN;
}


// Отметить переход USB в приостановленное состояние.
void Enter_LowPowerMode(void)
{

  bDeviceState = SUSPENDED;
}


// Восстановить состояние USB после выхода из приостановки.
void Leave_LowPowerMode(void)
{
  DEVICE_INFO *pInfo = &Device_Info;
  uint32_t vectorBase = SCB->VTOR;


  if (pInfo->Current_Configuration != 0)
  {

    bDeviceState = CONFIGURED;
  }
  else
  {
    bDeviceState = ATTACHED;
  }

  SystemInit();
  // Восстановление тактирования не должно менять таблицу векторов.
  SCB->VTOR = vectorBase;
}


// Задать прежние приоритеты USB и разрешить его прерывания.
void USB_Interrupts_Config(void)
{
  SCB->AIRCR = 0x05FA0500u;
  board_irq_enable(USB_LP_CAN1_RX0_IRQn, 0x80u);
  board_irq_enable(USBWakeUp_IRQn, 0);
}

// Принять настройки CDC; виртуальный порт не управляет физическим UART платы.
// Принять настройки CDC без перенастройки физического UART.
_Bool USART_Config(void)
{
  return TRUE;
}


// Передать входящие USB-байты разборщику протокола приложения.
// Передать принятые байты USB разборщику кадров.
void USB_To_USART_Send_Data(uint8_t *data_buffer, uint8_t Nb_bytes)
{
  usb_datareceived(data_buffer, Nb_bytes);
}


// Выставить следующий неизменяемый пакет IN, если хост готов.
void Handle_USBAsynchXfer (void)
{
  uint16_t count,offset; const uint8_t *source;
  if (USB_Tx_State || bDeviceState != CONFIGURED || !usb_tx_source) return;
  source=usb_tx_source; count=usb_tx_length; offset=usb_tx_offset; count-=offset;
  if(!count) { usb_tx_source=NULL; return; } /* Хост подтвердил последний пакет IN. */
  if(count>VIRTUAL_COM_PORT_DATA_SIZE) count=VIRTUAL_COM_PORT_DATA_SIZE;
  UserToPMABufferCopy((uint8_t*)source+offset,ENDP1_TXADDR,count);
  usb_tx_offset+=count;
  USB_Tx_State=1;
#if DOMOVOI_DIAG
  usb_in_armed++;
#endif
  SetEPTxCount(ENDP1,count); SetEPTxValid(ENDP1);
}
// Освободить TX и сменить эпоху после сброса USB.
void USB_Tx_Reset(void)
{
  USB_Tx_State=0; usb_tx_source=NULL;
  usb_tx_length=0; usb_tx_offset=0; usb_tx_epoch++;
}
// Проверить владение источником очереди передачи.
uint8_t USB_Tx_SourceBusy(void) { return usb_tx_source!=NULL; }
#include "work_memory.h"

// Опубликовать проверенный неизменяемый источник целиком или отказать.
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
// Проверить внешний буфер и атомарно передать его очереди USB.
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
// Разрешить новую команду после освобождения рабочих буферов.
uint8_t USB_Tx_CommandReady(void)
{
  extern bool user_storage_command_ready(void);
  return user_storage_command_ready() && !USB_Tx_SourceBusy();
}

// Передать ответ только в исходную USB-сессию команды.
uint8_t USB_Tx_Try(const uint8_t *source,uint16_t length)
{
  extern uint8_t usb_enabled; extern uint8_t store[];
  extern volatile uint32_t usb_bus_resets;
  uint32_t mask,epoch; uint8_t result=0;
  if(length>=USART_RX_DATA_SIZE || (length && !source) || !usb_enabled || !(GPIOB->IDR&(1u<<15))) return 0;
  mask=__get_PRIMASK(); __disable_irq(); memcpy(&epoch,store+12,4);
  if(epoch==usb_bus_resets) result=usb_tx_submit(source,length,usb_tx_epoch);
  __set_PRIMASK(mask); return result;
}
#if DOMOVOI_DIAG
// Поставить отладочную строку в очередь с учётом её ёмкости.
uint16_t USB_Send_Debug(uint8_t *data,uint16_t count)
{
  uint32_t mask=__get_PRIMASK(); uint8_t result;
  if(count && data == NULL) return 0;
  if(count>=USART_RX_DATA_SIZE) count=USART_RX_DATA_SIZE-1;
  __disable_irq(); result=usb_tx_submit(data,count,usb_tx_epoch);
  __set_PRIMASK(mask); return result?count:0;
}
#endif
// Проверить наличие пакета IN или оставшегося источника.
uint8_t USB_Tx_Pending(void) { return USB_Tx_State || usb_tx_source!=NULL; }
// Атомарно вернуть число ещё не отправленных байтов.
uint16_t USB_Tx_Queued(void)
{
  uint32_t mask=__get_PRIMASK(); uint16_t length,offset;
  __disable_irq();
  length=usb_tx_source ? usb_tx_length : 0; offset=usb_tx_offset;
  __set_PRIMASK(mask); return length ? length-offset : 0;
}

// Сформировать неизменный серийный USB-дескриптор из UID платы.
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


// Закодировать шестнадцатеричные цифры UID в UTF-16 USB.
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
