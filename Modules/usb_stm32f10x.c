#include "usb_stm32f10x.h"

#define USBBUFFERSIZE                   (uint16_t)2048

#pragma data_alignment=4
uint8_t usb_buffer[USBBUFFERSIZE]; // принятые данные по usb
uint16_t usb_buffer_count = 0;
uint8_t usb_time_to_check_buffer = 0;
uint8_t usb_data_ready = 0;
uint8_t usb_enabled = 0;
volatile uint32_t usb_tx_errors = 0;
volatile uint32_t usb_debug_dropped = 0;


// Инициализация --------------------------------------------------------------
void usb_init(void)
{ 
  USB_PULLUP_ON; // включаем подтягивающий резистор
  
  Set_System();
  Set_USBClock();
  USB_Interrupts_Config();
  USB_Init();
  usb_enabled = 1;
}

// Копируем даные из USB в буфер ----------------------------------------------
void usb_datareceived (uint8_t* ptr, uint16_t len)
{
  if (len > (USBBUFFERSIZE - usb_buffer_count)){
    len = USBBUFFERSIZE - usb_buffer_count; 
  }

  if (len){
    while (len--) usb_buffer[usb_buffer_count++] = *ptr++;
    usb_time_to_check_buffer = 2;
  }
}

// Отправить данные -----------------------------------------------------------
/* Return complete-queue acceptance; failures are also retained for diagnostics. */
uint8_t usb_send (uint8_t* ptr, uint16_t len)
{
  if (usb_enabled && USB_DETECT && USART_To_USB_Send_Data(ptr, len)) return 1;
  usb_tx_errors++;
  return 0;
}

void usb_sendstr (char* str)
{
  uint16_t len = strlen(str), sent = 0;
  if (usb_enabled && USB_DETECT) sent = USB_Send_Debug((uint8_t*)str, len);
  usb_debug_dropped += len - sent;
}

// check time proccess, 1 ms --------------------------------------------------
void usb_timer1ms (void)
{
  if (usb_time_to_check_buffer)
   {
     usb_time_to_check_buffer--;
     if (usb_time_to_check_buffer == 0) usb_data_ready = 1;
   }
}

// usb check data --------------------------------------------------------------
void usb_check_data (void)
{
  if (usb_data_ready == 0) return;

  // разбираем что пришло
  usb_data_reading(usb_buffer, usb_buffer_count);
  usb_clear_buffer();
}

// активен usb или нет ---------------------------------------------------------
uint8_t usb_get_state (void)
{
  return usb_enabled;
}

// очистка буфера --------------------------------------------------------------
void usb_clear_buffer (void)
{
  usb_buffer_count = 0;
  usb_data_ready = 0;
}

// разбор принятых данных определяется пользователем ---------------------------
__weak void usb_data_reading(uint8_t* ptr, uint16_t len)
{
}