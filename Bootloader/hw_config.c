/* Разработчик: Кряжев А., 2026 */

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

/* Типы ---------------------------------------------------------------------- */
/* Определения --------------------------------------------------------------- */
/* Макросы ------------------------------------------------------------------- */
/* Переменные ---------------------------------------------------------------- */

EXTI_InitTypeDef EXTI_InitStructure;
uint8_t  USART_Rx_Buffer [USART_RX_DATA_SIZE];
volatile uint32_t USART_Rx_ptr_in = 0;
volatile uint32_t USART_Rx_ptr_out = 0;
volatile uint32_t USART_Rx_length  = 0;

volatile uint8_t  USB_Tx_State = 0;
static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len);
/* Внешние переменные -------------------------------------------------------- */

extern LINE_CODING linecoding;

/* Прототипы функций --------------------------------------------------------- */
/* Функции ------------------------------------------------------------------- */

void Set_System(void)
{
  /*!< К этому моменту тактирование микроконтроллера уже настроено функцией
       SystemInit(), которая вызывается из файла startup (startup_stm32f10x_xx.s)
       до перехода в main. Чтобы изменить настройки SystemInit(), смотрите
       system_stm32f10x.c
     */

#ifdef USB_DISCONNECT_PIN
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Включить тактирование порта USB_DISCONNECT */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIO_DISCONNECT, ENABLE);

  /* Настроить вывод подтяжки USB */
  GPIO_InitStructure.GPIO_Pin = USB_DISCONNECT_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(USB_DISCONNECT, &GPIO_InitStructure);
#endif

  /* Настроить линию EXTI 18, внутренне подключённую к блоку USB */
  EXTI_ClearITPendingBit(EXTI_Line18);
  EXTI_InitStructure.EXTI_Line = EXTI_Line18;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
}

void Set_USBClock(void)
{
  /* Выбрать источник USBCLK */
  if (SystemCoreClock == 72000000) RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5); // если частота 72 МГц
  else RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_Div1); // если частота 48 МГц
  /* Включить тактирование USB */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
}

void USB_Interrupts_Config(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  /* 2 бита приоритета вытеснения, 2 бита подприоритета */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

  NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

    /* Разрешить прерывание пробуждения USB */
  NVIC_InitStructure.NVIC_IRQChannel = USBWakeUp_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_Init(&NVIC_InitStructure);
}

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
  /* Прототип пользовательской функции */
  usb_datareceived(data_buffer, Nb_bytes);
#endif
}

void USB_Tx_Reset(void)
{
 uint32_t mask = __get_PRIMASK(); __disable_irq();
 USART_Rx_ptr_in = 0; USART_Rx_ptr_out = 0; USART_Rx_length = 0; USB_Tx_State = 0;
 __set_PRIMASK(mask);
}

void Handle_USBAsynchXfer (void)
{
 uint32_t mask = __get_PRIMASK(); __disable_irq();
 if (USB_Tx_State != 1) {
  uint32_t out = USART_Rx_ptr_out, in = USART_Rx_ptr_in;
  if (out == USART_RX_DATA_SIZE) out = 0;
  USART_Rx_ptr_out = out;
  if (out != in) {
   uint32_t length = out > in ? USART_RX_DATA_SIZE - out : in - out;
   uint16_t n = length > VIRTUAL_COM_PORT_DATA_SIZE ? VIRTUAL_COM_PORT_DATA_SIZE : (uint16_t)length;
   USART_Rx_ptr_out = out + n; USART_Rx_length = length - n; USB_Tx_State = 1;
   UserToPMABufferCopy(&USART_Rx_Buffer[out], ENDP1_TXADDR, n);
   SetEPTxCount(ENDP1, n); SetEPTxValid(ENDP1);
  } else USB_Tx_State = 0;
 }
 __set_PRIMASK(mask);
}

uint8_t USART_To_USB_Send_Data(uint8_t* data_buffer, uint16_t Nb_bytes)
{
#ifdef FULL_CDC_MODULE
  if (linecoding.datatype == 7)
  {
    USART_Rx_Buffer[USART_Rx_ptr_in] = USART_ReceiveData(EVAL_COM1) & 0x7F;
  }
  else if (linecoding.datatype == 8)
  {
    USART_Rx_Buffer[USART_Rx_ptr_in] = USART_ReceiveData(EVAL_COM1);
  }

  USART_Rx_ptr_in++;

  /* Чтобы не переполнить буфер */
  if(USART_Rx_ptr_in == USART_RX_DATA_SIZE)
  {
    USART_Rx_ptr_in = 0;
  }
  return 1;
#else
  // публикуем весь кадр одним снимком, без частичной записи при отказе
  if (Nb_bytes == 0) return 1;
  if (!data_buffer || Nb_bytes >= USART_RX_DATA_SIZE) return 0;
  uint32_t budget = 32000u;
  while (budget != 0) {
    uint32_t mask = __get_PRIMASK(); __disable_irq();
    if (bDeviceState != CONFIGURED) { __set_PRIMASK(mask); return 0; }
    uint32_t in = USART_Rx_ptr_in, out = USART_Rx_ptr_out;
    uint32_t space = out > in ? out - in - 1u : USART_RX_DATA_SIZE - in + out - 1u;
    if (space >= Nb_bytes) {
      for (uint16_t i = 0; i < Nb_bytes; ++i) {
        USART_Rx_Buffer[in++] = data_buffer[i];
        if (in == USART_RX_DATA_SIZE) in = 0;
      }
      __DMB(); USART_Rx_ptr_in = in; __set_PRIMASK(mask); return 1;
    }
    // очередь без активного IN тоже должна получать продвижение
    if (USB_Tx_State == 0) Handle_USBAsynchXfer();
    __set_PRIMASK(mask); --budget;
  }
  return 0;
#endif
}

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
