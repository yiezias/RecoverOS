#ifndef __THREAD_SYNC_H
#define __THREAD_SYNC_H

#include "../lib/kernel/list.h"
#include "../lib/stdint.h"

struct semaphore {
	uint8_t value;
	struct list waiters;
};

struct lock {
	struct task_struct *holder;
	struct semaphore semaphore;
	uint64_t holder_repeat_nr;
};

void sema_init(struct semaphore *psema, uint8_t value);
void sema_down(struct semaphore *psema);
void sema_up(struct semaphore *psema);
void lock_init(struct lock *plock);
void lock_acquire(struct lock *plock);
void lock_release(struct lock *plock);

#endif
