#include "id_crc_cache.h"
#ifndef IDCRC_RAW_IO
#define w25_async_start idcrc_io_start
#define w25_async_poll idcrc_io_poll
#define w25_async_active idcrc_io_active
#define w25_write_data idcrc_write
#define w25_erase_sector idcrc_erase
#endif
