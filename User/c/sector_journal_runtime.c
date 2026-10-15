// Доработка: Кряжев А., 2026
// Приложение использует общий журнал секторов из sector_journal.c.
// Размещение заголовка и восстановление совпадают с загрузчиком и моделью.
// Проверенные записи выполняет W25; CRC32 приложения считает ядро ASM_CRC.
#include "../counter.h"
#define SJ_AFTER_TARGET(address) counter_replaced(address)
#include "../sector_journal.c"
