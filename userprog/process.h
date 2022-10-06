#ifndef __USERPROG_PROCESS_H
#define __USERPROG_PROCESS_H
#include "../lib/stdint.h"

#define USER_START_ADDR 0x1000
#define USER_STACK_VADDR 0x800000000000
#define STACK_PAGE_CNT 2

struct task_struct *process_execute(void *filename, char *name);

size_t pml4_paddr(struct task_struct *thread);
uint64_t *create_pml4(void);

#endif
