/******************************************************************************
*    SPI functions
*    Author      : Sergey Koryakin
******************************************************************************/
#include "flash_w25.h"

// „тение данных с пам€ти -----------------------------------------------------
bool w25_read_data (Int32U address, pInt8U dst, Int16U len)
{
  if (w25_busy() & 1u) return false;

  w25_chip_ON();
  w25_delay_ON();

  spi_send_byte(0x03); // команда чтени€ данных
  spi_send_byte((address>>16) & 0xff); // адрес
  spi_send_byte((address>>8) & 0xff);
  spi_send_byte(address & 0xff);
  spi_read(dst, len);

  w25_chip_OFF();
  w25_delay_OFF();
  return true;
}

// «апись данных в пам€ть -----------------------------------------------------
bool w25_write_data (Int32U address, pInt8U src, Int16U len)
{
  uint32_t k;
  uint16_t s;
  
  k = div(address, FLASH_W25_PAGE_SIZE).rem; // с какого адреса пишем на странице размером 256 байт
  if (k != 0) // не сначала страницы = пишем остаток до заполнени€ страницы
  {
    k = FLASH_W25_PAGE_SIZE - k; // остаток от страницы
    
    if (len > k) s = k; // записываемые данные больше остатка?
    else s = len;
    
    if (!w25_page_program(address, src, s)) return false;
    address += s;
    src += s;
    len -= s;    
  }
  
  while (len)
  {
    if (len > FLASH_W25_PAGE_SIZE) s = FLASH_W25_PAGE_SIZE; // записываемые данные больше размера страницы?
    else s = len;
    
    wdt_reload();
    if (!w25_page_program(address, src, s)) return false;
    address += s;
    src += s;
    len -= s;   
  }
  return true;
}

// —тереть сектор -------------------------------------------------------------
bool w25_erase_sector (Int32U address)
{
  if (w25_busy() & 1u) return false;

  w25_WREN();
  if (!(w25_read_status() & 2u)) return false;

  w25_chip_ON();
  w25_delay_ON();

  spi_send_byte(0x20);
  spi_send_byte((address>>16) & 0xff); // адрес
  spi_send_byte((address>>8) & 0xff);
  spi_send_byte(address & 0xff);

  w25_chip_OFF();
  w25_delay_OFF();

  bool ready = !(w25_busy() & 1u);
  w25_WRDI();
  return ready && w25_verify(address & ~4095u, NULL, 4096);
}

// „тение Id пам€ти -----------------------------------------------------------
Int16U w25_read_devID (void)
{
  Int16U result;

  if (w25_busy() & 1u) return 0xFFFF;

  w25_chip_ON();
  w25_delay_ON();

  spi_send_byte (0x90); // команда запроса Device ID
  spi_send_byte (0x00); // адрес..00
  spi_send_byte (0x00);
  spi_send_byte (0x00);
  spi_read ((pInt8U)&result, 2);

  w25_chip_OFF();
  w25_delay_OFF();

  return result;  
}

// „тение статуса -------------------------------------------------------------
Int8U w25_read_status (void)
{
  Int8U data;

  w25_chip_ON();
  w25_delay_ON();

  spi_send_byte(0x05);  // отправл€ем команду чтени€ статус регистра
  data = spi_read_byte();

  w25_chip_OFF();
  w25_delay_OFF();
  
  return data;
}

// —брос регистра статуса -----------------------------------------------------
void w25_clear_status (void)
{
  w25_busy();  
    
  w25_chip_ON();
  w25_delay_ON();

  spi_send_byte(0x30);  // отправл€ем команду сброса статус регистра

  w25_chip_OFF();
  w25_delay_OFF();
}

// —тереть микросхему ---------------------------------------------------------
void w25_erase_chip (void)
{
  w25_busy();

  w25_WREN();

  w25_chip_ON();
  w25_delay_ON();

  spi_send_byte(0x60);

  w25_chip_OFF();
  w25_delay_OFF();

  w25_clear_status();
  w25_WRDI();
}

// ќжидание завершени€ предыдущей команды -------------------------------------
static uint8_t w25_busy (void)
{
  uint8_t data;
  uint32_t count = 1000000;

  w25_chip_ON();
  w25_delay_ON();

  spi_send_byte(0x05);  // отправл€ем команду чтени€ статус регистра
  // в цикле считываем и ждем, пока пам€ть не освободитс€
  do
   {
   wdt_reload();
   data = spi_read_byte();
   }while ((data & 1) && (count--));

  w25_chip_OFF();
  w25_delay_OFF();
  
  return data;
}

// встраиваема€ задержка ------------------------------------------------------
static inline void w25_delay_ON (void)
{
  asm("nop");
}

// встраиваема€ задержка ------------------------------------------------------
static inline void w25_delay_OFF (void)
{
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");
  asm("nop");  
}

// разрешить запись -----------------------------------------------------------
static void w25_WREN()
{
  w25_chip_ON();
  w25_delay_ON();

  spi_send_byte(0x06); // разрешаем запись данных

  w25_chip_OFF();
  w25_delay_OFF();
}

// запретить запись -----------------------------------------------------------
static void w25_WRDI()
{
  w25_chip_ON();
  w25_delay_ON();

  spi_send_byte(0x04); // запретить запись данных

  w25_chip_OFF();
  w25_delay_OFF();
}

// «апись страницы пам€ти не более 256 байт за один прогон --------------------
static bool w25_page_program (Int32U address, pInt8U src, Int16U len)
{
  if (w25_busy() & 1u) return false;

  w25_WREN();
  if (!(w25_read_status() & 2u)) return false;
  
  w25_chip_ON();
  w25_delay_ON();

  spi_send_byte(0x02);
  spi_send_byte((address>>16) & 0xff); // адрес
  spi_send_byte((address>>8) & 0xff);
  spi_send_byte(address & 0xff);
  spi_send(src, len);

  w25_chip_OFF();
  w25_delay_OFF();
  
  bool ready = !(w25_busy() & 1u);
  w25_WRDI();
  return ready && w25_verify(address, src, len);
}
// ѕровер€ем результат без буфера сектора в ќ«”.
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
