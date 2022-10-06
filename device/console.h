#ifndef __DEVICE_CONSOLE_H
#define __DEVICE_CONSOLE_H
#include "../lib/stdint.h"

void console_init(void);
void console_acquire(void);
void console_release(void);
void console_put_str(char *str);
void console_put_char(char char_asci);
void console_put_num(size_t num);
void console_put_info(char *message, size_t num);
void sys_clear(void);

#endif
