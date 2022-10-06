#ifndef __LIB_STDIO_H
#define __LIB_STDIO_H
#include "stdint.h"

size_t vsprintf(char *str, const char *format, void *args);
size_t printk(const char *format, ...);
size_t printf(const char *format, ...);
size_t sprintf(char* buf, const char* format, ...);

#endif
