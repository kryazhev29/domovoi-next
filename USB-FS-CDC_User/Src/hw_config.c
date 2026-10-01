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

/* Includes ------------------------------------------------------------------*/
#include "stm32_it.h"
#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "hw_config.h"
#include "usb_pwr.h"

#define         ID1          (0x1FFFF7E8)
#define         ID2          (0x1FFFF7EC)
#define         ID3          (0x1FFFF7F0)

//#define USB_DISCONNECT_PIN                    GPIO_Pin_14
//#define USB_DISCONNECT                        GPIOB
//#define RCC_APB2Periph_GPIO_DISCONNECT        RCC_APB2Periph_GPIOB
//#define FULL_CDC_MODULE

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
USART_InitTypeDef USART_InitStructure;
EXTI_InitTypeDef EXTI_InitStructure;
uint8_t  USART_Rx_Buffer [USART_RX_DATA_SIZE]; 
volatile uint32_t USART_Rx_ptr_in = 0;
volatile uint32_t USART_Rx_ptr_out = 0;
volatile uint32_t USART_Rx_length  = 0;

volatile uint8_t  USB_Tx_State = 0;
volatile uint32_t usb_in_armed = 0; /* пакеты IN, отданные EP1, команда 111 */
static volatile uint32_t usb_tx_epoch = 0;
static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len);
/* Extern variables ----------------------------------------------------------*/

extern LINE_CODING linecoding;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : Set_System
* Description    : Configures Main system clocks & power
* Input          : None.
* Return         : None.
*******************************************************************************/
void Set_System(void)
{
  /*!< At this stage the microcontroller clock setting is already configured, 
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f10x_xx.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f10x.c file
     */   

#ifdef USB_DISCONNECT_PIN
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable USB_DISCONNECT GPIO clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIO_DISCONNECT, ENABLE);

  /* Configure USB pull-up pin */
  GPIO_InitStructure.GPIO_Pin = USB_DISCONNECT_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(USB_DISCONNECT, &GPIO_InitStructure);
#endif  

  /* Configure the EXTI line 18 connected internally to the USB IP */
  EXTI_ClearITPendingBit(EXTI_Line18);
  EXTI_InitStructure.EXTI_Line = EXTI_Line18; 
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
}

/*******************************************************************************
* Function Name  : Set_USBClock
* Description    : Configures USB Clock input (48MHz)
* Input          : None.
* Return         : None.
*******************************************************************************/
void Set_USBClock(void)
{
  /* Select USBCLK source */
  if (SystemCoreClock == 72000000) RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5); // если частота 72 МГц
  else RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_Div1); // если частота 48 МГц
  /* Enable the USB clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
}

/*******************************************************************************
* Function Name  : Enter_LowPowerMode
* Description    : Power-off system clocks and power while entering suspend mode
* Input          : None.
* Return         : None.
*******************************************************************************/
void Enter_LowPowerMode(void)
{
  /* Set the device state to suspend */
  bDeviceState = SUSPENDED;
}

/*******************************************************************************
* Function Name  : Leave_LowPowerMode
* Description    : Restores system clocks and power while exiting suspend mode
* Input          : None.
* Return         : None.
*******************************************************************************/
void Leave_LowPowerMode(void)
{
  DEVICE_INFO *pInfo = &Device_Info;
  uint32_t vectorBase = SCB->VTOR;

  /* Set the device state to the correct state */
  if (pInfo->Current_Configuration != 0)
  {
    /* Device configured */
    bDeviceState = CONFIGURED;
  }
  else
  {
    bDeviceState = ATTACHED;
  }
  /*Enable SystemCoreClock*/
  SystemInit();
  // Восстановление тактирования не должно менять таблицу векторов.
  SCB->VTOR = vectorBase;
}

/*******************************************************************************
* Function Name  : USB_Interrupts_Config
* Description    : Configures the USB interrupts
* Input          : None.
* Return         : None.
*******************************************************************************/
void USB_Interrupts_Config(void)
{
  NVIC_InitTypeDef NVIC_InitStructure; 
  
  /* 2 bit for pre-emption priority, 2 bits for subpriority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  
  
  NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
    /* Enable the USB Wake-up interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USBWakeUp_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_Init(&NVIC_InitStructure);
}

/*******************************************************************************
* Function Name  : USB_Cable_Config
* Description    : Software Connection/Disconnection of USB Cable
* Input          : None.
* Return         : Status
*******************************************************************************/
void USB_Cable_Config (FunctionalState NewState)
{
#ifdef USB_DISCONNECT_PIN  
  if (NewState != DISABLE)
  {
  // подключить
  //GPIO_ResetBits(USB_DISCONNECT, USB_DISCONNECT_PIN);
  GPIO_SetBits(USB_DISCONNECT, USB_DISCONNECT_PIN);
  }
  else
  {
  // отключить
  //GPIO_SetBits(USB_DISCONNECT, USB_DISCONNECT_PIN);
  GPIO_ResetBits(USB_DISCONNECT, USB_DISCONNECT_PIN);
  }
#endif
}

/*******************************************************************************
* Function Name  :  USART_Config_Default.
* Description    :  configure the EVAL_COM1 with default values.
* Input          :  None.
* Return         :  None.
*******************************************************************************/
void USART_Config_Default(void)
{
#ifdef FULL_CDC_MODULE  
  /* EVAL_COM1 default configuration */
  /* EVAL_COM1 configured as follow:
        - BaudRate = 9600 baud  
        - Word Length = 8 Bits
        - One Stop Bit
        - Parity Odd
        - Hardware flow control disabled
        - Receive and transmit enabled
  */
  USART_InitStructure.USART_BaudRate = 9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_Odd;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  /* Configure and enable the USART */
  STM_EVAL_COMInit(COM1, &USART_InitStructure);

  /* Enable the USART Receive interrupt */
  USART_ITConfig(EVAL_COM1, USART_IT_RXNE, ENABLE);
#endif
}

/*******************************************************************************
* Function Name  :  USART_Config.
* Description    :  Configure the EVAL_COM1 according to the line coding structure.
* Input          :  None.
* Return         :  Configuration status
                    TRUE : configuration done with success
                    FALSE : configuration aborted.
*******************************************************************************/
bool USART_Config(void)
{
/*uint8_t stopbits = 0;
  
  // set the Stop bit
  switch (linecoding.format)
  {
    case 0:
      stopbits = 0;
      break;
    case 2:
      stopbits = 1;
      break;
    default :
    {
      //USART_Config_Default();
      return (FALSE);
    }
  }*/  
  
  // set the parity bit
  /*switch (linecoding.paritytype)
  {
    case 0:
      parity = 0;
      break;
    case 1:
      parity = 1;
      break;
    case 2:
      parity = 2;
      break;
    default :
    {
      //USART_Config_Default();
      return (FALSE);
    }
  }*/

  // set the data type : only 8bits and 9bits is supported 
  /*switch (linecoding.datatype)
  {
    //case 0x07:
      // With this configuration a parity (Even or Odd) should be set 
      //USART_InitStructure.USART_WordLength = USART_WordLength_8b;
      //break;
    case 0x08:
      break;
    default :
    {
      //USART_Config_Default();
      return (FALSE);
    }
  }*/
return (TRUE);
}

/*******************************************************************************
* Function Name  : USB_To_USART_Send_Data.
* Description    : send the received data from USB to the UART 0.
* Input          : data_buffer: data address.
                   Nb_bytes: number of bytes to send.
* Return         : none.
*******************************************************************************/
void USB_To_USART_Send_Data(uint8_t* data_buffer, uint8_t Nb_bytes)
{
#ifdef FULL_CDC_MODULE  
  uint32_t i;
  
  for (i = 0; i < Nb_bytes; i++)
  {
    USART_SendData(EVAL_COM1, *(data_buffer + i));
    while(USART_GetFlagStatus(EVAL_COM1, USART_FLAG_TXE) == RESET); 
  }
#else
  /* User function prototype */
  usb_datareceived(data_buffer, Nb_bytes);
#endif
}

/*******************************************************************************
* Function Name  : Handle_USBAsynchXfer.
* Description    : send data to USB.
* Input          : None.
* Return         : none.
*******************************************************************************/
void Handle_USBAsynchXfer (void)
{
  uint32_t in, out, count;
  if (USB_Tx_State == 1 || bDeviceState != CONFIGURED) return;
  in = USART_Rx_ptr_in;
  out = USART_Rx_ptr_out;
  if (out == USART_RX_DATA_SIZE) out = 0;
  if (in == out) return;
  count = out > in ? USART_RX_DATA_SIZE - out : in - out;
  if (count > VIRTUAL_COM_PORT_DATA_SIZE) count = VIRTUAL_COM_PORT_DATA_SIZE;
  UserToPMABufferCopy(&USART_Rx_Buffer[out], ENDP1_TXADDR, (uint16_t)count);
  out += count;
  if (out == USART_RX_DATA_SIZE) out = 0;
  USART_Rx_ptr_out = out;
  USART_Rx_length = 0;
  USB_Tx_State = 1;
  usb_in_armed++;
  SetEPTxCount(ENDP1, (uint16_t)count);
  SetEPTxValid(ENDP1);
}
/*******************************************************************************
* Function Name  : UART_To_USB_Send_Data.
* Description    : send the received data from UART 0 to USB.
* Input          : None.
* Return         : none.
*******************************************************************************/
/* Вызывается из прерывания сброса USB. Незаконченную очередь через сброс никогда не сохранять. */
void USB_Tx_Reset(void)
{
  USB_Tx_State = 0;
  USART_Rx_ptr_in = 0;
  USART_Rx_ptr_out = 0;
  USART_Rx_length = 0;
  usb_tx_epoch++;
}
static uint32_t usb_tx_space(void)
{
  uint32_t out = USART_Rx_ptr_out;
  if (out == USART_RX_DATA_SIZE) out = 0;
  return (out + USART_RX_DATA_SIZE - USART_Rx_ptr_in - 1u) % USART_RX_DATA_SIZE;
}
/* Вызывающий держит PRIMASK: данные и индекс записи публикуются вместе. */
static void usb_tx_enqueue(uint8_t *p, uint16_t n)
{
  uint32_t in = USART_Rx_ptr_in;
  while (n--) {
    USART_Rx_Buffer[in++] = *p++;
    if (in == USART_RX_DATA_SIZE) in = 0;
  }
  __DMB();
  USART_Rx_ptr_in = in;
}
/* Успех означает, что кадр целиком поставлен в очередь, а не подтверждён хостом. Один
 * ограниченный бюджет ожидания на весь вызов; сторожевой таймер здесь не перезаряжается.
 */
uint8_t USART_To_USB_Send_Data(uint8_t* data_buffer, uint16_t Nb_bytes)
{
  uint32_t budget = 320000u, epoch = usb_tx_epoch;
  if (Nb_bytes >= USART_RX_DATA_SIZE) return 0;
  do {
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    if (bDeviceState != CONFIGURED || epoch != usb_tx_epoch) {
      __set_PRIMASK(mask); return 0;
    }
    if (usb_tx_space() >= Nb_bytes) {
      usb_tx_enqueue(data_buffer, Nb_bytes);
      __set_PRIMASK(mask); return 1;
    }
    __set_PRIMASK(mask);
    if (mask) return 0; /* Потребитель не продвинется при запрещённых прерываниях. */
    --budget;
  } while (budget != 0);
  return 0;
}
uint16_t USB_Send_Debug(uint8_t *data, uint16_t count)
{
  uint32_t mask = __get_PRIMASK(), available;
  __disable_irq();
  available = bDeviceState == CONFIGURED ? usb_tx_space() : 0;
  if (count > available) count = (uint16_t)available;
  usb_tx_enqueue(data, count);
  __set_PRIMASK(mask);
  return count;
}

/* Сторож USB: данные ждут хоста или выставлен пакет IN. */
uint8_t USB_Tx_Pending(void)
{
  uint32_t out = USART_Rx_ptr_out;
  if (out == USART_RX_DATA_SIZE) out = 0;
  return USB_Tx_State == 1 || USART_Rx_ptr_in != out;
}
uint16_t USB_Tx_Queued(void)
{
  uint32_t out = USART_Rx_ptr_out;
  if (out == USART_RX_DATA_SIZE) out = 0;
  return (uint16_t)((USART_Rx_ptr_in + USART_RX_DATA_SIZE - out) % USART_RX_DATA_SIZE);
}
/*******************************************************************************
* Function Name  : Get_SerialNum.
* Description    : Create the serial number string descriptor.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
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

/*******************************************************************************
* Function Name  : HexToChar.
* Description    : Convert Hex 32Bits value into char.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
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
