// Доработка: Кряжев А., 2026
/**
  ******************************************************************************
  * @file    usb_endp.c
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   Endpoint routines
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


#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_mem.h"
#include "hw_config.h"
#include "usb_istr.h"
#include "usb_pwr.h"

#define VCOMPORT_IN_FRAME_INTERVAL             5

uint8_t USB_Rx_Buffer[VIRTUAL_COM_PORT_DATA_SIZE];
extern  uint8_t USART_Rx_Buffer[];
#ifndef BOOTLOADER
extern volatile uint32_t USART_Rx_ptr_out;
extern volatile uint32_t USART_Rx_length;
extern volatile uint8_t  USB_Tx_State;
extern void usb_in_done(void);
extern void usb_sof_seen(void);
#else
extern volatile uint32_t USART_Rx_ptr_out;
extern volatile uint32_t USART_Rx_length;
extern volatile uint8_t  USB_Tx_State;
#endif

// Продолжить передачу после подтверждения хостом пакета IN.
void EP1_IN_Callback (void)
{
#ifndef BOOTLOADER
  usb_in_done();
  if (USB_Tx_State == 1) {
    USB_Tx_State = 0;
    Handle_USBAsynchXfer();
  }
#else
  uint32_t mask = __get_PRIMASK(); __disable_irq();
  if (USB_Tx_State == 1) {
    uint32_t remaining = USART_Rx_length;
    if (remaining == 0) USB_Tx_State = 0;
    else {
      uint32_t out = USART_Rx_ptr_out;
      uint16_t n = remaining > VIRTUAL_COM_PORT_DATA_SIZE ? VIRTUAL_COM_PORT_DATA_SIZE : (uint16_t)remaining;
      USART_Rx_ptr_out = out + n; USART_Rx_length = remaining - n;
      UserToPMABufferCopy(&USART_Rx_Buffer[out], ENDP1_TXADDR, n);
      SetEPTxCount(ENDP1, n); SetEPTxValid(ENDP1);
    }
  }
  __set_PRIMASK(mask);
#endif
}


// Принять OUT-пакет и вернуть готовность конечной точке.
void EP3_OUT_Callback(void)
{
  uint16_t USB_Rx_Cnt;


  USB_Rx_Cnt = USB_SIL_Read(EP3_OUT, USB_Rx_Buffer);



  USB_To_USART_Send_Data(USB_Rx_Buffer, USB_Rx_Cnt);


  SetEPRxValid(ENDP3);
}

// Отметить USB-кадр и обслужить очередь IN.
void SOF_Callback(void)
{
  static uint32_t FrameCount = 0;
#ifndef BOOTLOADER
  usb_sof_seen();
#endif

  if(bDeviceState == CONFIGURED)
  {
    if (FrameCount++ == VCOMPORT_IN_FRAME_INTERVAL)
    {

      FrameCount = 0;


      Handle_USBAsynchXfer();
    }
  }
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

