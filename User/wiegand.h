// Доработка: Кряжев А., 2026
// Интерфейс считывателя Wiegand W26 платы v5.
// Главный цикл передаёт завершённую карту через WG_readedID за один проход.
// Чётность управляется WG_PARITY, обработчики фронтов защищают снимок кадра.

#ifndef WIEGAND_H
#define WIEGAND_H

#include "includes.h"
#ifndef WG_PARITY
#define WG_PARITY 1
#endif


void WG_init(void);
void WG_process(void);
void WG_readedID(uint32_t id);

#endif


