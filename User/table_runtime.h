#ifndef DOMOVOI_TABLE_RUNTIME_H
#define DOMOVOI_TABLE_RUNTIME_H
#include "includes.h"
bool table_runtime_begin(const uint8_t *,dataChanel_e,uint8_t);
void table_runtime_poll(uint32_t),table_runtime_cancel(void),table_runtime_quiet(dataChanel_e);
#endif
