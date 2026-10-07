// Доработка: Кряжев А., 2026
// Подключает учёт CRC к внутренним операциям W25.
// Обычные писатели проходят через кэш; сам кэш вызывает исходный драйвер.
// IDCRC_RAW_IO отделяет низкий уровень от перехвата имён операций.
#include "id_crc_cache.h"
#ifndef IDCRC_RAW_IO
#define w25_async_start idcrc_io_start
#define w25_async_poll idcrc_io_poll
#define w25_async_active idcrc_io_active
#define w25_write_data idcrc_write
#define w25_erase_sector idcrc_erase
#endif
