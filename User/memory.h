// Доработка: Кряжев А., 2026
// Доступ к W25, журналируемым таблицам и атомарным настройкам A/B.
// Чтение настроек перенаправляется из логического адреса в активный слот.
// Снимок фонового сохранения живёт до завершения проверки W25.
#include "includes.h"

#ifndef MEM_H
#define MEM_H

#define MEMSECTORSIZE         4096
#define MEM_CONFIG_LEGACY     0x1CD400u
#define MEM_CONFIG_SLOT0      0x1F5000u
#define MEM_CONFIG_SLOT1      0x1F7000u
#define MEM_CONFIG_SLOT_SIZE 8192u
extern volatile uint32_t mem_config_errors;
// Копирует снимки config/device; освобождать их можно только после завершения.
bool mem_config_begin(const uint8_t *config512, const uint8_t *device512);
// Продвигает публикацию снимка A/B одной командой за проход.
void mem_config_poll(uint32_t now);
// Возвращает состояние публикации снимка A/B.
uint8_t mem_config_result(void); // 0 — нет операции, 1 — работа, 2 — проверено, 3 — ошибка.
// Освобождает снимок завершённой публикации.
void mem_config_reset(void);
// Даёт владельцу сохранённый снимок для обработки результата.
const uint8_t *mem_config_snapshot(void);

// Восстанавливает зафиксированный секторный журнал при старте.
bool mem_recover(void);
// Проверяет доступность таблиц без повторного восстановления.
#include "flash_exec.h"
typedef uint8_t (*mem_page_reader)(const void *context, uint16_t offset, uint8_t *page);
// Атомарно заменяет сектор из источника страниц.
bool mem_replace_pages(uint32_t adr, mem_page_reader reader, const void *context);
// Атомарно изменяет один фрагмент образа.
bool mem_patch(Int32U adr, const uint8_t *data, Int16U size);
// Атомарно изменяет два смежных фрагмента образа.
bool mem_patch2(Int32U adr, const uint8_t *first, Int16U nfirst, const uint8_t *second, Int16U nsecond);

// Чтение данных с памяти
bool mem_read (Int32U adr, pInt8U buff, Int16U size);

// Запись данных в память
bool mem_write (Int32U adr, pInt8U buff, Int16U size);

// Стереть сектор
bool mem_erase_sector (Int32U adr);

#endif
