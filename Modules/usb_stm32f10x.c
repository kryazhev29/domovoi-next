#include "usb_stm32f10x.h"

#define USBBUFFERSIZE                   (uint16_t)2048

#pragma data_alignment=4
uint8_t usb_buffer[USBBUFFERSIZE]; // принятые данные по usb
static volatile uint16_t usb_buffer_count = 0;
static volatile uint8_t usb_rx_processing = 0;
static volatile uint8_t usb_rx_discard = 0;
volatile uint32_t usb_rx_errors = 0;
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
  uint16_t count = usb_buffer_count;
  if (usb_rx_discard) return;
  if (len > USBBUFFERSIZE - count) {
    /* A missing byte must never join the two sides of a broken stream. */
    usb_rx_discard = 1;
    usb_rx_errors++;
    return;
  }
  memcpy(usb_buffer + count, ptr, len);
  __DMB();
  usb_buffer_count = count + len;
}

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
  /* Packet completeness is determined by SLDN length and CRC, not silence. */
}

// usb check data --------------------------------------------------------------
void usb_check_data (void)
{
  uint32_t mask = __get_PRIMASK();
  uint16_t count, skip = 0, body, total, crc;
  __disable_irq();
  if (usb_rx_processing) { __set_PRIMASK(mask); return; }
  if (usb_rx_discard) {
    usb_buffer_count = 0;
    usb_rx_discard = 0;
  }
  count = usb_buffer_count;
  /* Retain up to three trailing bytes: they may start the next SLDN. */
  while (count - skip >= 4 && memcmp(usb_buffer + skip, "SLDN", 4)) skip++;
  if (skip) {
    count -= skip;
    memmove(usb_buffer, usb_buffer + skip, count);
    usb_buffer_count = count;
  }
  if (count < 6) { __set_PRIMASK(mask); return; }
  body = (uint16_t)usb_buffer[4] | ((uint16_t)usb_buffer[5] << 8);
  if (body < 3 || body > USBBUFFERSIZE - 8) {
    memmove(usb_buffer, usb_buffer + 1, count - 1);
    usb_buffer_count = count - 1;
    usb_rx_errors++;
    __set_PRIMASK(mask); return;
  }
  total = body + 8;
  if (count < total) { __set_PRIMASK(mask); return; }
  /* ISR may append while the application parses, but cannot change this prefix.
   * A USB reset defers discard until the parser releases its borrowed buffer. */
  usb_rx_processing = 1;
  __set_PRIMASK(mask);
  crc = (uint16_t)usb_buffer[total - 2] | ((uint16_t)usb_buffer[total - 1] << 8);
  if (crc == crc16_calc(usb_buffer + 4, body + 2)) {
    if (!usb_rx_discard) usb_data_reading(usb_buffer, total);
  } else usb_rx_errors++;
  mask = __get_PRIMASK();
  __disable_irq();
  if (usb_rx_discard) {
    usb_buffer_count = 0;
    usb_rx_discard = 0;
  } else {
    count = usb_buffer_count;
    count -= total;
    memmove(usb_buffer, usb_buffer + total, count);
    usb_buffer_count = count;
  }
  usb_rx_processing = 0;
  __set_PRIMASK(mask);
}

// активен usb или нет ---------------------------------------------------------
uint8_t usb_get_state (void)
{
  return usb_enabled;
}

// очистка буфера --------------------------------------------------------------
void usb_clear_buffer (void)
{
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  if (usb_rx_processing) usb_rx_discard = 1;
  else {
    usb_buffer_count = 0;
    usb_rx_discard = 0;
  }
  __set_PRIMASK(mask);
}

// разбор принятых данных определяется пользователем ---------------------------
__weak void usb_data_reading(uint8_t* ptr, uint16_t len)
{
}