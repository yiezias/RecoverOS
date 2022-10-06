#ifndef __DEVICE_TIMER_H
#define __DEVICE_TIMER_H


#include "../lib/stdint.h"


void mtime_sleep(size_t m_seconds);
void timer_init(void);

size_t sys_getticks(void);
#endif
