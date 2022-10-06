#ifndef __USERPROG_WAIT_EXIT_H
#define __USERPROG_WAIT_EXIT_H
#include "../thread/thread.h"

pid_t sys_wait(int *status);
void sys_exit(int status);

#endif
