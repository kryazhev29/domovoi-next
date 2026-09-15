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
volatile uint32_t usb_recovery_count = 0;
static volatile uint8_t usb_recovery_pending = 0;
static volatile uint8_t usb_recovery_ms = 0;

/* USB watch. The ages count milliseconds and saturate. A stall is armed only
 * after the host writes, so data left for a closed COM port never re-enumerates.
 * Reasons: 1 ESOF reset timeout, 2 IN queue not taken, 3 OUT while unconfigured. */
#define USB_WATCH_MS 2000u
#define USB_AGE_MAX 0xFFFFu
volatile uint32_t usb_esof_forced = 0;
volatile uint32_t usb_esof_timeouts = 0;
volatile uint32_t usb_bus_resets = 0;
volatile uint32_t usb_recovery_reason = 0;
volatile uint32_t usb_watch_in_recoveries = 0;
volatile uint32_t usb_watch_unconf_recoveries = 0;
volatile uint32_t usb_out_unconfigured = 0;
static volatile uint32_t usb_last_recovery[7];
static volatile uint16_t usb_in_ago = USB_AGE_MAX;
static volatile uint16_t usb_out_ago = USB_AGE_MAX;
static volatile uint16_t usb_sof_ago = USB_AGE_MAX;
static volatile uint16_t usb_hung_ms = 0;
static volatile uint16_t usb_unconf_ms = 0;
static volatile uint8_t usb_hung_armed = 0;
static volatile uint8_t usb_unconf_armed = 0;

/* Command 111 v2: USB registers at the last recovery and now. EP1 TX NAK with
 * USB_Tx_State 1 means a lost CTR_TX interrupt; TX VALID means the host stopped
 * polling. Reads only, no register side effects. */
#define USB_REGS 9u
extern volatile uint8_t USB_Tx_State;
extern volatile uint32_t usb_in_armed;
static volatile uint32_t usb_in_taken = 0;
static uint32_t usb_last_regs[USB_REGS];
/* ERR interrupts: no answer, CRC, bit stuffing or framing error on the bus. */
static volatile uint32_t usb_bus_errors = 0;
static uint32_t usb_last_bus_errors = 0;
/* Device address register and EP0R: a device that lost its address still sees
 * SOF (broadcast) but no IN token addressed to it. */
static uint32_t usb_last_addr[2];

/* EP1R, EP3R, EP1 TX count, ISTR, CNTR, FNR, USB_Tx_State, IN armed, IN taken. */
static void usb_regs_read(uint32_t *r)
{
  r[0] = _GetENDPOINT(ENDP1);
  r[1] = _GetENDPOINT(ENDP3);
  r[2] = _GetEPTxCount(ENDP1);
  r[3] = _GetISTR();
  r[4] = _GetCNTR();
  r[5] = _GetFNR();
  r[6] = USB_Tx_State;
  r[7] = usb_in_armed;
  r[8] = usb_in_taken;
}

/* USB IRQ: the host took an IN packet. */
void usb_in_done(void)
{
  usb_in_ago = 0;
  usb_hung_ms = 0;
  usb_in_taken++;
}

/* USB IRQ: bus error. */
void usb_bus_error_seen(void)
{
  usb_bus_errors++;
}

/* USB IRQ: start of frame. */
void usb_sof_seen(void)
{
  usb_sof_ago = 0;
}

/* Called on an unrecoverable USB reset timeout. Re-enumerate from main,
 * never wait for a host or SysTick inside the USB interrupt. */
void usb_request_recovery(void)
{
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  usb_last_recovery[0] = usb_recovery_reason;
  usb_last_recovery[1] = bDeviceState;
  usb_last_recovery[2] = USB_Tx_Pending();
  usb_last_recovery[3] = USB_Tx_Queued();
  usb_last_recovery[4] = usb_in_ago;
  usb_last_recovery[5] = usb_out_ago;
  usb_last_recovery[6] = usb_sof_ago;
  usb_regs_read(usb_last_regs); /* before FRES and detach change them */
  usb_last_bus_errors = usb_bus_errors;
  usb_last_addr[0] = _GetDADDR();
  usb_last_addr[1] = _GetENDPOINT(ENDP0);
  usb_hung_armed = usb_unconf_armed = 0;
  usb_hung_ms = usb_unconf_ms = 0;
  usb_out_ago = USB_AGE_MAX;
  _SetCNTR(CNTR_FRES);
  USB_PULLUP_OFF;
  usb_enabled = 0;
  USB_Tx_Reset();
  usb_clear_buffer();
  bDeviceState = UNCONNECTED;
  usb_recovery_ms = 20;
  usb_recovery_pending = 1;
  usb_recovery_count++;
  __set_PRIMASK(mask);
}


// Инициализация --------------------------------------------------------------
void usb_init(void)
{ 
  USB_PULLUP_OFF; /* Publish the pull-up only after USB initialization. */
  
  Set_System();
  Set_USBClock();
  USB_Interrupts_Config();
  USB_Init();
  usb_enabled = 1;
  USB_PULLUP_ON;
}

// Копируем даные из USB в буфер ----------------------------------------------
void usb_datareceived (uint8_t* ptr, uint16_t len)
{
  uint32_t mask = __get_PRIMASK();
  uint16_t count;
  /* USB IRQ and SysTick share the silence counter; preserve incoming PRIMASK. */
  __disable_irq();
  count = usb_buffer_count;
  if (len != 0) {
    usb_out_ago = 0;
    if (bDeviceState != CONFIGURED) { usb_out_unconfigured++; usb_unconf_armed = 1; }
    else if (USB_Tx_Pending()) usb_hung_armed = 1;
  }
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
  if (usb_recovery_ms != 0) --usb_recovery_ms;
  if (usb_in_ago < USB_AGE_MAX) usb_in_ago++;
  if (usb_out_ago < USB_AGE_MAX) usb_out_ago++;
  if (usb_sof_ago < USB_AGE_MAX) usb_sof_ago++;
  if (!USB_Tx_Pending()) { usb_hung_armed = 0; usb_hung_ms = 0; }
  else if (usb_hung_armed && usb_hung_ms < USB_AGE_MAX) usb_hung_ms++;
  if (bDeviceState == CONFIGURED) { usb_unconf_armed = 0; usb_unconf_ms = 0; }
  else if (usb_unconf_armed && usb_unconf_ms < USB_AGE_MAX) usb_unconf_ms++;
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
  if (usb_recovery_pending) {
    if (usb_recovery_ms == 0 && USB_DETECT) {
      usb_recovery_pending = 0;
      usb_init();
    }
    __set_PRIMASK(mask);
    return;
  }
  if (usb_hung_ms >= USB_WATCH_MS || usb_unconf_ms >= USB_WATCH_MS) {
    if (usb_hung_ms >= USB_WATCH_MS) { usb_recovery_reason = 2; usb_watch_in_recoveries++; }
    else { usb_recovery_reason = 3; usb_watch_unconf_recoveries++; }
    usb_request_recovery();
    __set_PRIMASK(mask);
    return;
  }
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

/* Command 111: 23 USB values, in the order documented at user_send_usbDiag. */
void usb_diag_fill(uint32_t *v)
{
  uint32_t mask = __get_PRIMASK();
  uint8_t i;
  __disable_irq();
  v[0] = usb_esof_forced;
  v[1] = usb_esof_timeouts;
  v[2] = usb_bus_resets;
  v[3] = usb_recovery_count;
  v[4] = usb_watch_in_recoveries;
  v[5] = usb_watch_unconf_recoveries;
  for (i = 0; i < 7; i++) v[6 + i] = usb_last_recovery[i];
  v[13] = bDeviceState;
  v[14] = USB_Tx_Pending();
  v[15] = USB_Tx_Queued();
  v[16] = usb_in_ago;
  v[17] = usb_out_ago;
  v[18] = usb_sof_ago;
  v[19] = usb_tx_errors;
  v[20] = usb_rx_errors;
  v[21] = usb_debug_dropped;
  v[22] = usb_out_unconfigured;
  __set_PRIMASK(mask);
}

/* Command 111 v2 tail: 9 register values at the last recovery, then 9 now. */
void usb_diag_fill_regs(uint32_t *v)
{
  uint32_t mask = __get_PRIMASK();
  uint8_t i;
  __disable_irq();
  for (i = 0; i < USB_REGS; i++) v[i] = usb_last_regs[i];
  usb_regs_read(&v[USB_REGS]);
  __set_PRIMASK(mask);
}

/* Command 111 v3 tail: bus errors now, bus errors at the last recovery. */
void usb_diag_fill_errors(uint32_t *v)
{
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  v[0] = usb_bus_errors;
  v[1] = usb_last_bus_errors;
  __set_PRIMASK(mask);
}

/* Command 111 v4 tail: DADDR and EP0R at the last recovery, then now. */
void usb_diag_fill_addr(uint32_t *v)
{
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  v[0] = usb_last_addr[0];
  v[1] = usb_last_addr[1];
  v[2] = _GetDADDR();
  v[3] = _GetENDPOINT(ENDP0);
  __set_PRIMASK(mask);
}
