// Доработка: Кряжев А., 2026
/**
  ******************************************************************************
  * @file    hw_config.h
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

#ifndef __HW_CONFIG_H
#define __HW_CONFIG_H
#include "../../User/domovoi_diag.h"

#include "stm32f10x.h"
#include "usb_type.h"

#define MASS_MEMORY_START     0x04002000
#define BULK_MAX_PACKET_SIZE  0x00000040


#ifndef BOOTLOADER
/* Самый длинный ответ: 32 записи журнала, 1038 байт вместе с обрамлением. */
#include "../../User/usb_ring_size.h"
#define USART_RX_DATA_SIZE   USB_TX_RING_BYTES
#else
#define USART_RX_DATA_SIZE   512
#endif

void Set_System(void);
void Set_USBClock(void);
void Enter_LowPowerMode(void);
void Leave_LowPowerMode(void);
void USB_Interrupts_Config(void);
void USB_Cable_Config (FunctionalState NewState);
_Bool USART_Config(void);
void USB_To_USART_Send_Data(uint8_t* data_buffer, uint8_t Nb_bytes);
#ifndef BOOTLOADER
uint8_t USART_To_USB_Send_Data(uint8_t* data_buffer, uint16_t Nb_bytes);
#else
uint8_t USART_To_USB_Send_Data(uint8_t* data_buffer, uint16_t Nb_bytes);
#endif
void USB_Tx_Reset(void);
#if DOMOVOI_DIAG
uint16_t USB_Send_Debug(uint8_t *data, uint16_t count);
#endif
uint8_t USB_Tx_Pending(void);
uint16_t USB_Tx_Queued(void);
void Handle_USBAsynchXfer (void);
void Get_SerialNum(void);


#endif
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
