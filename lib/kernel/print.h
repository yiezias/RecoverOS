#ifndef __LIB_KERNEL_PRINT_H
#define __LIB_KERNEL_PRINT_H

#include "../stdint.h"

uint16_t get_cur(void);
void set_cur(uint16_t cur);

void put_char(char c);
void put_str(char *str);
void put_num(uint64_t num);
void put_info(char *message, size_t num);

void cls_screen(void);
#endif
