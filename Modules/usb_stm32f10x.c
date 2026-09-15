#include "usb_stm32f10x.h"

#define USBBUFFERSIZE                   (uint16_t)2048

#pragma data_alignment=4
uint8_t usb_buffer[USBBUFFERSIZE]; // принятые данные по usb
static volatile uint16_t usb_buffer_count = 0;
static volatile uint8_t usb_rx_processing = 0;
static volatile uint8_t usb_rx_discard = 0;
static volatile uint16_t usb_rx_silence = 0;
/* Byte boundary present at expiry; later input must not revive that prefix. */
static volatile uint16_t usb_rx_expired = 0;
static void usb_rx_consume(uint16_t length);
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
  uint32_t mask = __get_PRIMASK();
  uint16_t count;
  /* USB IRQ and SysTick share the silence counter; preserve incoming PRIMASK. */
  __disable_irq();
  count = usb_buffer_count;
  if (usb_rx_discard || len == 0) { __set_PRIMASK(mask); return; }
  if (!usb_rx_processing) {
    uint16_t expired = usb_rx_expired;
    uint32_t total = 0;
    if (expired >= 6 && !memcmp(usb_buffer, "SLDN", 4))
      total = ((uint32_t)usb_buffer[4] | ((uint32_t)usb_buffer[5] << 8)) + 8;
    if (expired && (expired < 6 || (total > expired && total <= USBBUFFERSIZE))) {
      /* Make room before append, even if main has not serviced the timeout. */
      usb_rx_consume(expired);
      count = usb_buffer_count;
    }
  }
  if (len > USBBUFFERSIZE - count) {
    usb_rx_discard = 1;
    usb_rx_errors++;
    __set_PRIMASK(mask);
    return;
  }
  memcpy(usb_buffer + count, ptr, len);
  __DMB();
  usb_buffer_count = count + len;
  usb_rx_silence = 0;
  __set_PRIMASK(mask);
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
  uint32_t mask = __get_PRIMASK();
  uint16_t silence, count;
  __disable_irq();
  silence = usb_rx_silence;
  count = usb_buffer_count;
  if (count != 0 && silence < 200) {
    silence++;
    usb_rx_silence = silence;
    if (silence == 200) usb_rx_expired = count;
  }
  __set_PRIMASK(mask);
}

/* Called with interrupts masked, after a borrowed parser prefix is released. */
static void usb_rx_consume(uint16_t length)
{
  uint16_t count = usb_buffer_count, expired = usb_rx_expired;
  count -= length;
  memmove(usb_buffer, usb_buffer + length, count);
  usb_buffer_count = count;
  usb_rx_expired = expired > length ? expired - length : 0;
  if (count == 0) usb_rx_silence = 0;
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
    usb_rx_silence = 0;
    usb_rx_expired = 0;
  }
  count = usb_buffer_count;
  /* Retain up to three trailing bytes: they may start the next SLDN. */
  while (count - skip >= 4 && memcmp(usb_buffer + skip, "SLDN", 4)) skip++;
  if (skip) {
    count -= skip;
    usb_rx_consume(skip);
  }
  if (count < 6) {
    uint16_t expired = usb_rx_expired;
    if (expired) usb_rx_consume(expired);
    __set_PRIMASK(mask); return;
  }
  body = (uint16_t)usb_buffer[4] | ((uint16_t)usb_buffer[5] << 8);
  if (body < 3 || body > USBBUFFERSIZE - 8) {
    usb_rx_consume(1);
    usb_rx_errors++;
    __set_PRIMASK(mask); return;
  }
  total = body + 8;
  {
    uint16_t expired = usb_rx_expired;
    if (expired != 0 && expired < total) {
      /* Discard only the expired incomplete prefix, preserving fresh bytes. */
      usb_rx_consume(expired);
      usb_rx_errors++;
      __set_PRIMASK(mask); return;
    }
  }
  if (count < total) { __set_PRIMASK(mask); return; }
  /* ISR may append while the application parses, but cannot change this prefix.
   * A USB reset defers discard until the parser releases its borrowed buffer. */
  usb_rx_processing = 1;
  __set_PRIMASK(mask);
  crc = (uint16_t)usb_buffer[total - 2] | ((uint16_t)usb_buffer[total - 1] << 8);
  if (crc == crc16_calc(usb_buffer + 4, body + 2)) {
    if (!usb_rx_discard) usb_data_reading(usb_buffer, total);
  } else {
    usb_rx_errors++;
    total = 4; /* The advertised length is untrusted: search after this SLDN. */
  }
  mask = __get_PRIMASK();
  __disable_irq();
  if (usb_rx_discard) {
    usb_buffer_count = 0;
    usb_rx_discard = 0;
    usb_rx_silence = 0;
    usb_rx_expired = 0;
  } else {
    usb_rx_consume(total);
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
    usb_rx_silence = 0;
    usb_rx_expired = 0;
  }
  __set_PRIMASK(mask);
}

// разбор принятых данных определяется пользователем ---------------------------
__weak void usb_data_reading(uint8_t* ptr, uint16_t len)
{
}