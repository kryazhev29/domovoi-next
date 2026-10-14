// Доработка: Кряжев А., 2026
// USB CDC: приём одного проверенного кадра и очередь двоичной передачи.
// Сторож восстанавливает зависший USB из главного цикла после записи хоста.
// Рабочие таймауты есть всегда; счётчики и снимки доступны только DOMOVOI_DIAG.

#include "usb_log.h"
#include "domovoi_diag.h"
// Доработка: Кряжев А., 2026
#include "usb_stm32f10x.h"
#include "../usb_ring_size.h"

#define USBBUFFERSIZE                   (uint16_t)2048

#pragma data_alignment=4
uint8_t usb_buffer[USBBUFFERSIZE]; // принятые данные по USB
volatile uint16_t usb_buffer_count = 0;
volatile uint8_t usb_rx_processing = 0;
volatile uint8_t usb_rx_discard = 0;
volatile uint16_t usb_rx_silence = 0;
/* Граница байта на момент истечения; поздний ввод не должен оживить этот префикс. */
volatile uint16_t usb_rx_expired = 0;
static void usb_rx_consume(uint16_t length);
// Полный кадр, ожидающий Flash, не перекрывает USB дольше 500 мс.
static uint32_t usb_command_wait;
#if DOMOVOI_DIAG
volatile uint32_t usb_rx_errors = 0;
#endif
// Эпоха USB-сессии нужна владельцам флеш для отмены ответа после сброса USB.
volatile uint32_t usb_bus_resets = 0;
uint8_t usb_enabled = 0;
#if DOMOVOI_DIAG
volatile uint32_t usb_tx_errors = 0;
volatile uint32_t usb_debug_dropped = 0;
volatile uint32_t usb_recovery_count = 0;
#endif
volatile uint8_t usb_recovery_pending = 0;
volatile uint8_t usb_recovery_ms = 0;

/* Сторож USB. Возраст считается в миллисекундах и не переполняется. Зависание взводится только
 * после записи хоста, поэтому данные для закрытого COM-порта никогда не вызывают
 * переподключение. Причины: 1 таймаут сброса ESOF, 2 очередь IN не забирается, 3 OUT при
 * ненастроенном USB. */
#define USB_WATCH_MS 2000u
#define USB_AGE_MAX 0xFFFFu
#if DOMOVOI_DIAG
volatile uint32_t usb_esof_forced = 0;
volatile uint32_t usb_esof_timeouts = 0;
volatile uint32_t usb_recovery_reason = 0;
volatile uint32_t usb_watch_in_recoveries = 0;
volatile uint32_t usb_watch_unconf_recoveries = 0;
volatile uint32_t usb_out_unconfigured = 0;
volatile uint32_t usb_last_recovery[7];
volatile uint16_t usb_in_ago = USB_AGE_MAX;
volatile uint16_t usb_out_ago = USB_AGE_MAX;
volatile uint16_t usb_sof_ago = USB_AGE_MAX;
#endif
volatile uint16_t usb_hung_ms = 0;
volatile uint16_t usb_unconf_ms = 0;
volatile uint8_t usb_hung_armed = 0;
volatile uint8_t usb_unconf_armed = 0;

/* Команда 111 v2: регистры USB на момент последнего восстановления и сейчас. EP1 TX NAK при
 * USB_Tx_State 1 означает потерянное прерывание CTR_TX; TX VALID - хост перестал опрашивать.
 * Только чтение, без побочных действий над регистрами. */
#define USB_REGS 9u
extern volatile uint8_t USB_Tx_State;
extern volatile uint32_t usb_in_armed;
#if DOMOVOI_DIAG
volatile uint32_t usb_in_taken = 0;
uint32_t usb_last_regs[USB_REGS];
#endif
/* Прерывания ERR: нет ответа, ошибка CRC, битстаффинга или кадра на шине. */
#if DOMOVOI_DIAG
volatile uint32_t usb_bus_errors = 0;
uint32_t usb_last_bus_errors = 0;
#endif
/* Регистр адреса устройства и EP0R: устройство, потерявшее адрес, по-прежнему видит SOF
 * (широковещательный), но не получает адресованных ему IN. */
#if DOMOVOI_DIAG
uint32_t usb_last_addr[2];
#endif

/* EP1R, EP3R, счётчик TX EP1, ISTR, CNTR, FNR, USB_Tx_State, IN выставлен, IN забран. */
#if DOMOVOI_DIAG
// Снять регистры очереди для диагностики восстановления.
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
#endif

/* Прерывание USB: хост забрал пакет IN. */
// Отметить подтверждённый пакет IN и обнулить время зависания.
void usb_in_done(void)
{
#if DOMOVOI_DIAG
  usb_in_ago = 0;
#endif
  usb_hung_ms = 0;
#if DOMOVOI_DIAG
  usb_in_taken++;
#endif
}

/* Прерывание USB: ошибка шины. */
// Посчитать ошибку USB для стендовой диагностики.
void usb_bus_error_seen(void)
{
#if DOMOVOI_DIAG
  usb_bus_errors++;
#endif
}

/* Прерывание USB: начало кадра. */
// Отметить свежий кадр USB для диагностического возраста.
void usb_sof_seen(void)
{
#if DOMOVOI_DIAG
  usb_sof_ago = 0;
#endif
}

/* Вызывается при неустранимом таймауте сброса USB. Переподключение делается из главного цикла,
 * внутри прерывания USB никогда не ждать хоста или SysTick. */
// Отсоединить зависший USB на 20 мс, освободив очередь и приём.
void usb_request_recovery(void)
{
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
#if DOMOVOI_DIAG
  usb_last_recovery[0] = usb_recovery_reason;
  usb_last_recovery[1] = bDeviceState;
  usb_last_recovery[2] = USB_Tx_Pending();
  usb_last_recovery[3] = USB_Tx_Queued();
  usb_last_recovery[4] = usb_in_ago;
  usb_last_recovery[5] = usb_out_ago;
  usb_last_recovery[6] = usb_sof_ago;
  usb_regs_read(usb_last_regs); /* до того, как FRES и отключение их изменят */
  usb_last_bus_errors = usb_bus_errors;
  usb_last_addr[0] = _GetDADDR();
  usb_last_addr[1] = _GetENDPOINT(ENDP0);
#endif
  usb_hung_armed = usb_unconf_armed = 0;
  usb_hung_ms = usb_unconf_ms = 0;
#if DOMOVOI_DIAG
  usb_out_ago = USB_AGE_MAX;
#endif
  _SetCNTR(CNTR_FRES);
  USB_PULLUP_OFF;
  usb_enabled = 0;
  USB_Tx_Reset();
  usb_clear_buffer();
  bDeviceState = UNCONNECTED;
  usb_recovery_ms = 20;
  usb_recovery_pending = 1;
#if DOMOVOI_DIAG
  usb_recovery_count++;
#endif
  __set_PRIMASK(mask);
}

// Подготовить USB и подключить подтяжку после настройки периферии.
void usb_init(void)
{
  USB_PULLUP_OFF; /* Подтяжка включается только после инициализации USB. */

  Set_System();
  Set_USBClock();
  USB_Interrupts_Config();
  USB_Init();
  usb_enabled = 1;
  USB_PULLUP_ON;
}

// Атомарно добавить внешние байты OUT без изменения разбираемого префикса.
void usb_datareceived (uint8_t* ptr, uint16_t len)
{
  uint32_t mask = __get_PRIMASK();
  uint16_t count;
  /* Прерывание USB и SysTick делят счётчик тишины; входящий PRIMASK сохраняется. */
  __disable_irq();
  count = usb_buffer_count;
  if (len != 0) {
#if DOMOVOI_DIAG
    usb_out_ago = 0;
#endif
    if (bDeviceState != CONFIGURED) {
#if DOMOVOI_DIAG
      usb_out_unconfigured++;
#endif
      usb_unconf_armed = 1;
    }
    else if (USB_Tx_Pending()) usb_hung_armed = 1;
  }
  if (usb_rx_discard || len == 0) { __set_PRIMASK(mask); return; }
  if (!usb_rx_processing) {
    uint16_t expired = usb_rx_expired;
    uint32_t total = 0;
    if (expired >= 6 && !memcmp(usb_buffer, "SLDN", 4))
      total = ((uint32_t)usb_buffer[4] | ((uint32_t)usb_buffer[5] << 8)) + 8;
    if (expired && (expired < 6 || (total > expired && total <= USBBUFFERSIZE))) {
      /* Место освобождается до добавления, даже если главный цикл ещё не обработал таймаут. */
      usb_rx_consume(expired);
      count = usb_buffer_count;
    }
  }
  if (len > USBBUFFERSIZE - count) {
    usb_rx_discard = 1;
#if DOMOVOI_DIAG
    usb_rx_errors++;
#endif
    __set_PRIMASK(mask);
    return;
  }
  memcpy(usb_buffer + count, ptr, len);
  __DMB();
  usb_buffer_count = count + len;
  usb_rx_silence = 0;
  __set_PRIMASK(mask);
}

/* Возвращает, принята ли очередь целиком; отказы также сохраняются для диагностики. */
// Обработать usb_send с прежним аппаратным контрактом.
uint8_t usb_send (uint8_t* ptr, uint16_t len)
{
  if (usb_enabled && USB_DETECT && USART_To_USB_Send_Data(ptr, len)) return 1;
#if DOMOVOI_DIAG
  usb_tx_errors++;
#endif
  return 0;
}

#if DOMOVOI_DIAG
// Передать отладочную строку только в диагностическом профиле.

#endif

// Отсчитать таймауты USB и тишину незаконченного кадра.
void usb_timer1ms (void)
{
  uint32_t mask = __get_PRIMASK();
  uint16_t silence, count;
  __disable_irq();
  if (usb_recovery_ms != 0) --usb_recovery_ms;
#if DOMOVOI_DIAG
  if (usb_in_ago < USB_AGE_MAX) usb_in_ago++;
  if (usb_out_ago < USB_AGE_MAX) usb_out_ago++;
  if (usb_sof_ago < USB_AGE_MAX) usb_sof_ago++;
#endif
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

/* Вызывается с запрещёнными прерываниями, после освобождения префикса, взятого разборщиком. */
// Освободить префикс при закрытых IRQ, сохраняя свежий хвост.
static void usb_rx_consume(uint16_t length)
{
  uint16_t count = usb_buffer_count, expired = usb_rx_expired;
  count -= length;
  memmove(usb_buffer, usb_buffer + length, count);
  usb_buffer_count = count;
  usb_rx_expired = expired > length ? expired - length : 0;
  if (count == 0) usb_rx_silence = 0;
}

// Проверить один целый кадр, передать его приложению и освободить префикс.
void usb_check_data (void)
{
  usb_log_session();
  uint8_t store_ready = USB_Tx_CommandReady();
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
#if DOMOVOI_DIAG
    if (usb_hung_ms >= USB_WATCH_MS) { usb_recovery_reason = 2; usb_watch_in_recoveries++; }
    else { usb_recovery_reason = 3; usb_watch_unconf_recoveries++; }
#endif
    usb_request_recovery();
    __set_PRIMASK(mask);
    return;
  }
  if (!store_ready) { __set_PRIMASK(mask); return; }
  if (usb_rx_discard) {
    usb_command_wait = 0;
    usb_buffer_count = 0;
    usb_rx_discard = 0;
    usb_rx_silence = 0;
    usb_rx_expired = 0;
  }
  count = usb_buffer_count;
  /* Оставить до трёх последних байт: с них может начинаться следующий SLDN. */
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
#if DOMOVOI_DIAG
    usb_rx_errors++;
#endif
    __set_PRIMASK(mask); return;
  }
  total = body + 8;
  {
    uint16_t expired = usb_rx_expired;
    if (expired != 0 && expired < total) {
      /* Отбрасывается только истёкший незаконченный префикс, свежие байты сохраняются. */
      usb_rx_consume(expired);
#if DOMOVOI_DIAG
      usb_rx_errors++;
#endif
      __set_PRIMASK(mask); return;
    }
  }
  if (count < total) { __set_PRIMASK(mask); return; }
  /* Прерывание может дописывать, пока прошивка разбирает кадр, но не может изменить этот
   * префикс. Сброс USB откладывает отбрасывание, пока разборщик не вернёт буфер. */
  usb_rx_processing = 1;
  __set_PRIMASK(mask);
  crc = (uint16_t)usb_buffer[total - 2] | ((uint16_t)usb_buffer[total - 1] << 8);
  if (crc == crc16_calc(usb_buffer + 4, body + 2)) {
    if (!user_command_ready(usb_buffer + 4, true)) {
      uint32_t now = timers_getMillis();
      if (!usb_command_wait) usb_command_wait = now | 1u;
      if ((int32_t)(now - usb_command_wait) < 500) total = 0;
    }
    // По истечении разборщик возвращает существующий CMD_RUNERROR и снимает кадр.
    if (total && !usb_rx_discard) usb_data_reading(usb_buffer, total);
  } else {
#if DOMOVOI_DIAG
    usb_rx_errors++;
#endif
    total = 4; /* Заявленной длине доверять нельзя: поиск продолжается после этого SLDN. */
  }
  mask = __get_PRIMASK();
  __disable_irq();
  if (usb_rx_discard) {
    usb_command_wait = 0;
    usb_buffer_count = 0;
    usb_rx_discard = 0;
    usb_rx_silence = 0;
    usb_rx_expired = 0;
  } else {
    usb_rx_consume(total);
  }
  usb_rx_processing = 0;
  if (total) usb_command_wait = 0;
  __set_PRIMASK(mask);
}

// Вернуть состояние программного подключения USB.
uint8_t usb_get_state (void)
{
  return usb_enabled;
}

// Сбросить приём сразу либо после возврата разборщика.
void usb_clear_buffer (void)
{
  usb_command_wait = 0;
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

/* Команда 111: 23 значения USB в порядке, описанном у user_send_usbDiag. */
#if DOMOVOI_DIAG
// Снять 23 счётчика USB для команды 111.
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
#endif

/* Хвост команды 111 v2: 9 значений регистров на момент последнего восстановления, затем 9
 * текущих. */
#if DOMOVOI_DIAG
// Снять текущие регистры и регистры последнего восстановления.
void usb_diag_fill_regs(uint32_t *v)
{
  uint32_t mask = __get_PRIMASK();
  uint8_t i;
  __disable_irq();
  for (i = 0; i < USB_REGS; i++) v[i] = usb_last_regs[i];
  usb_regs_read(&v[USB_REGS]);
  __set_PRIMASK(mask);
}
#endif

/* Хвост команды 111 v3: ошибки шины сейчас и на момент последнего восстановления. */
#if DOMOVOI_DIAG
// Выдать число ошибок шины сейчас и при последнем восстановлении.
void usb_diag_fill_errors(uint32_t *v)
{
  uint32_t mask = __get_PRIMASK();
  __disable_irq();
  v[0] = usb_bus_errors;
  v[1] = usb_last_bus_errors;
  __set_PRIMASK(mask);
}
#endif

/* Хвост команды 111 v4: DADDR и EP0R на момент последнего восстановления, затем текущие. */
#if DOMOVOI_DIAG
// Выдать адрес и EP0 сейчас и при последнем восстановлении.
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
#endif

/* Очередь передачи сохраняет владение источником ответа во всех вариантах. */
extern uint8_t USB_Tx_Try(const uint8_t *,uint16_t);
// Принять неизменяемый источник ответа в единственную очередь USB.
uint8_t usb_try_send(const uint8_t *ptr,uint16_t len) { return USB_Tx_Try(ptr,len); }
