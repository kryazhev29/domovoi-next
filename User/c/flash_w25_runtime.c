// Доработка: Кряжев А., 2026
// Синхронный и фоновый доступ к W25 через SPI.
// Одна команда находится в работе; источник записи сохраняется до проверки.
// Драйвер читает назад каждую запись и стирание, а при старте может ждать BUSY.
#include "flash_w25.h"

// Ожидание завершения предыдущей команды
static uint8_t w25_busy (void);
// встраиваемая задержка
static inline void w25_delay_ON (void);
// встраиваемая задержка
static inline void w25_delay_OFF (void);
// разрешить запись
static void w25_WREN(void);
// запретить запись
static void w25_WRDI(void);
// Запись страницы памяти
static bool w25_page_program (Int32U address, pInt8U src, Int16U len);

static bool w25_verify(Int32U address, const uint8_t *src, Int16U len);
static void w25_command(uint8_t command, uint32_t address);
#include <intrinsics.h>

// В работе одна команда NOR, без DMA и обратных вызовов главного цикла.
w25_job_s w25_job;
// Сообщает, принадлежит ли драйверу незавершённая команда.
bool w25_async_active(void) { return w25_job.active != 0; }
// Читает BUSY один раз и проверяет работоспособность SPI после обмена.
bool w25_read_available(void) { return !(w25_read_status() & 1u) && spi_ready(); }

// Запускает проверенную внутренним владельцем запись или стирание.
bool w25_async_start(uint32_t address, const uint8_t *src, uint16_t len, uint32_t now)
{
  if (w25_job.active || !w25_read_available()) return false;
  w25_WREN();
  if (!(w25_read_status() & 2u) || !spi_ready()) return false;
  w25_command(src ? 0x02 : 0x20, address);
  if (src) spi_send((uint8_t*)src, len);
  w25_chip_OFF(); w25_delay_OFF();
  if (!spi_ready()) return false;
  w25_job.data = src; w25_job.address = address;
  w25_job.length = src ? len : 4096u; w25_job.checked = 0;
  w25_job.started = now; w25_job.active = 1;
  return true;
}
// Ждёт BUSY без блокировки и проверяет до 64 Б записанного результата.
uint8_t w25_async_poll(uint32_t now)
{
  uint8_t b[64], status;
  uint16_t n;
  if (!w25_job.active) return 0;
  status = w25_read_status();
  if (!spi_ready()) goto failed;
  if (status & 1u) {
    // Таймаут ограничивает BUSY, а не задержки между вызовами автомата.
    if ((uint32_t)(now - w25_job.started) >= (w25_job.data ? 20u : 1000u)) goto failed;
    return 1;
  }
  n = w25_job.length - w25_job.checked;
  if (n > sizeof(b)) n = sizeof(b);
  if (!w25_read_data(w25_job.address + w25_job.checked, b, n)) goto failed;
  for (uint16_t i = 0; i < n; ++i)
    if (b[i] != (w25_job.data ? w25_job.data[w25_job.checked + i] : 255u)) goto failed;
  w25_job.checked += n;
  if (w25_job.checked != w25_job.length) return 1;
  w25_job.active = 0;
  return 2;
failed:
  w25_job.active = 0;
  return 3;
}

// Чтение данных с памяти -----------------------------------------------------
bool w25_read_data (Int32U address, pInt8U dst, Int16U len)
{
  // При старте допустимо ожидание; в работе BUSY опрашивается один раз,
  // включая повторное обращение после ошибки обмена.
  extern uint8_t store_runtime;
  if (store_runtime ? !w25_read_available() : (w25_busy() & 1u)) return false;

  w25_command(0x03, address);
  spi_read(dst, len);

  w25_chip_OFF();
  w25_delay_OFF();
  return spi_ready() != 0;
}

// Запись данных в память -----------------------------------------------------
bool w25_write_data (Int32U address, pInt8U src, Int16U len)
{
  if (w25_job.active) return false;
  while (len) {
    uint16_t n = FLASH_W25_PAGE_SIZE - (address & (FLASH_W25_PAGE_SIZE - 1u));
    if (n > len) n = len;
    wdt_reload();
    if (!w25_page_program(address, src, n)) return false;
    address += n; src += n; len -= n;
  }
  return true;
}

// Общий синхронный шаг: стирание сектора (src NULL) или запись страницы; проверка у вызывающего.
static bool w25_sync(Int32U address, pInt8U src, Int16U len)
{
  if (w25_busy() & 1u) return false;

  w25_WREN();
  if (!(w25_read_status() & 2u)) return false;

  w25_command(src ? 0x02 : 0x20, address);
  if (src) spi_send(src, len);

  w25_chip_OFF();
  w25_delay_OFF();

  bool ready = !(w25_busy() & 1u);
  w25_WRDI();
  return ready;
}

// Стереть сектор -------------------------------------------------------------
bool w25_erase_sector (Int32U address)
{
  if (w25_job.active) return false;
  return w25_sync(address, NULL, 0) && w25_verify(address & ~4095u, NULL, 4096);
}

// Чтение Id памяти -----------------------------------------------------------
Int16U w25_read_devID (void)
{
  Int16U result = 0xFFFF;

  if (w25_busy() & 1u) return 0xFFFF;

  w25_command(0x90, 0);
  spi_read ((pInt8U)&result, 2);

  w25_chip_OFF();
  w25_delay_OFF();

  return spi_ready() ? result : 0xFFFF;
}

// Чтение статуса -------------------------------------------------------------
Int8U w25_read_status (void)
{
  Int8U data;

  w25_chip_ON();
  w25_delay_ON();

  spi_send_byte(0x05);  // отправляем команду чтения статус регистра
  data = spi_read_byte();

  w25_chip_OFF();
  w25_delay_OFF();

  return data;
}

// Ожидание завершения предыдущей команды -------------------------------------
static uint8_t w25_busy (void)
{
  uint8_t data;
  if (w25_job.active) return w25_read_status();
  uint32_t count = 1000000;

  w25_chip_ON();
  w25_delay_ON();

  spi_send_byte(0x05);  // отправляем команду чтения статус регистра
  // в цикле считываем и ждем, пока память не освободится
  do
   {
   wdt_reload();
   data = spi_read_byte();
   if (!spi_ready()) { data = 0xff; break; }
   }while ((data & 1) && (count--));

  w25_chip_OFF();
  w25_delay_OFF();

  return data;
}

// Выдерживает время переключения выбора чипа W25.
static inline void w25_delay_ON (void)
{
  __no_operation();
}

// Выдерживает время переключения выбора чипа W25.
static inline void w25_delay_OFF (void)
{
  __no_operation();
  __no_operation();
  __no_operation();
  __no_operation();
  __no_operation();
}

// разрешить запись -----------------------------------------------------------
static void w25_WREN(void)
{
  w25_chip_ON();
  w25_delay_ON();

  spi_send_byte(0x06); // разрешаем запись данных

  w25_chip_OFF();
  w25_delay_OFF();
}

// запретить запись -----------------------------------------------------------
static void w25_WRDI(void)
{
  w25_chip_ON();
  w25_delay_ON();

  spi_send_byte(0x04); // запретить запись данных

  w25_chip_OFF();
  w25_delay_OFF();
}

// Запись страницы памяти не более 256 байт за один прогон --------------------
static bool w25_page_program (Int32U address, pInt8U src, Int16U len)
{
  return w25_sync(address, src, len) && w25_verify(address, src, len);
}
// Проверяем результат без буфера сектора в ОЗУ.
static bool w25_verify(Int32U address, const uint8_t *src, Int16U len)
{
  uint8_t b[64];
  while (len) {
    uint16_t n = len > sizeof(b) ? sizeof(b) : len;
    wdt_reload();
    if (!w25_read_data(address, b, n)) return false;
    for (uint16_t i = 0; i < n; i++)
      if (b[i] != (src != NULL ? src[i] : 0xFF)) return false;
    address += n;
    if (src != NULL) src += n;
    len -= n;
  }
  return true;
}

// Начинает адресную SPI-команду; вызывающий передаёт данные и закрывает выбор чипа.
static void w25_command(uint8_t command, uint32_t address)
{
  w25_chip_ON(); w25_delay_ON();
  spi_send_byte(command);
  spi_send_byte((uint8_t)(address >> 16));
  spi_send_byte((uint8_t)(address >> 8));
  spi_send_byte((uint8_t)address);
}
