// Доработка: Кряжев А., 2026
// Доступ к W25 через SPI: чтение, проверенная запись и стирание.
// Фоновая команда сохраняет источник до завершения обратного чтения.
// Синхронные операции нужны при старте и восстановлении после обрыва.
#include "spi_stm32f10x.h"
#include "arm_comm.h"
#include "includes.h"

#ifndef FLASH_W25
#define FLASH_W25

#define w25_chip_ON()   CSEL_LOW
#define w25_chip_OFF()  CSEL_HIGH

#define FLASH_W25_PAGE_SIZE     256

// Общий дескриптор текущей команды: CRC сначала читает прежние байты.
typedef struct {
  const uint8_t *data;
  uint32_t address, started;
  uint16_t length, checked;
  uint8_t active;
} w25_job_s;
extern w25_job_s w25_job;

// Удерживает источник до результата 2 или 3; NULL задаёт стирание сектора.
bool w25_async_start(uint32_t address, const uint8_t *src, uint16_t len, uint32_t now);
uint8_t w25_async_poll(uint32_t now); // 0 — нет команды, 1 — работа, 2 — проверено, 3 — ошибка.
// Сообщает о незавершённой команде записи или стирания.
bool w25_async_active(void);
// Проверяет готовность W25 и SPI без ожидания.
bool w25_read_available(void);

// Чтение данных с памяти
bool w25_read_data (Int32U address, pInt8U dst, Int16U len);
// Запись данных в память
bool w25_write_data (Int32U address, pInt8U src, Int16U len);
// Стереть сектор
bool w25_erase_sector (Int32U address);
// Чтение Id памяти
Int16U w25_read_devID (void);
// Чтение статуса
Int8U w25_read_status (void);

#endif
