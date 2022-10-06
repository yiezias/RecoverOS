#ifndef __USERPROG_SYSCALL_INIT_H
#define __USERPROG_SYSCALL_INIT_H
#include "../lib/stdint.h"
#include "../thread/thread.h"

void syscall_init(void);
pid_t sys_getpid(void);
//size_t sys_write(char *str);

#endif
