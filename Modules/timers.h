

#ifndef __TIMERS_H
#define __TIMERS_H

#include "includes.h"

#define delay_ms        timers_pause

void timers_init(void);
void timers_set(unsigned char t, unsigned int vl);
unsigned int timers_get(unsigned char t);
unsigned int timers_take(unsigned char t, unsigned int period);
void timers_handle(void);
void timers_pause(const unsigned int msec);
unsigned int timers_getMillis(void);

#endif