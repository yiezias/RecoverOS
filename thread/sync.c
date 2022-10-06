#include "sync.h"
#include "../kernel/debug.h"
#include "../kernel/intr.h"
#include "thread.h"


void sema_init(struct semaphore *psema, uint8_t value) {
	ASSERT(value == 0 || value == 1);
	psema->value = value;
	list_init(&psema->waiters);
}

void lock_init(struct lock *plock) {
	plock->holder = NULL;
	plock->holder_repeat_nr = 0;
	sema_init(&plock->semaphore, 1);
}

void sema_down(struct semaphore *psema) {
	enum intr_stat old_stat = set_intr_stat(intr_off);
	while (psema->value == 0) {
		ASSERT(!elem_find(&psema->waiters,
				  &current->general_tag));
		list_append(&psema->waiters, &current->general_tag);
		thread_block(TASK_BLOCKED);
	}
	--psema->value;
	ASSERT(psema->value == 0);
	set_intr_stat(old_stat);
}

void sema_up(struct semaphore *psema) {
	enum intr_stat old_stat = set_intr_stat(intr_off);
	ASSERT(psema->value == 0);
	if (!list_empty(&psema->waiters)) {
		struct task_struct *thread_blocked =
			elem2entry(struct task_struct, general_tag,
				   list_pop(&psema->waiters));
		thread_unblock(thread_blocked);
	}
	++psema->value;
	ASSERT(psema->value == 1);
	set_intr_stat(old_stat);
}

void lock_acquire(struct lock *plock) {
	if (plock->holder != running_thread()) {
		sema_down(&plock->semaphore);
		plock->holder = running_thread();
		ASSERT(plock->holder_repeat_nr == 0);
	}
	++plock->holder_repeat_nr;
}

void lock_release(struct lock *plock) {
	ASSERT(plock->holder == running_thread());
	if (plock->holder_repeat_nr > 1) {
		--plock->holder_repeat_nr;
		return;
	}
	ASSERT(plock->holder_repeat_nr == 1);
	plock->holder = NULL;
	plock->holder_repeat_nr = 0;
	sema_up(&plock->semaphore);
}
