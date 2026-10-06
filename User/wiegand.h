
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


