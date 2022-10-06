#include "ioqueue.h"
#include "../kernel/debug.h"
#include "../kernel/intr.h"
#include "../thread/thread.h"

void ioqueue_init(struct ioqueue *ioq) {
	lock_init(&ioq->lock);
	ioq->producer = ioq->consumer = NULL;
	ioq->head = ioq->tail = 0;
}

static ssize_t next_pos(ssize_t pos) {
	return (pos + 1) % bufsize;
}

bool ioq_full(struct ioqueue *ioq) {
	ASSERT(intr_off == get_intr_stat());
	return next_pos(ioq->head) == ioq->tail;
}

bool ioq_empty(struct ioqueue *ioq) {
	ASSERT(intr_off == get_intr_stat());
	return ioq->head == ioq->tail;
}

static void ioq_wait(struct task_struct **waiter) {
	ASSERT(*waiter == NULL && waiter != NULL);
	*waiter = running_thread();
	thread_block(TASK_BLOCKED);
}

static void wakeup(struct task_struct **waiter) {
	ASSERT(*waiter != NULL);
	thread_unblock(*waiter);
	*waiter = NULL;
}

char ioq_getchar(struct ioqueue *ioq) {
	ASSERT(intr_off == get_intr_stat());
	while (ioq_empty(ioq)) {
		lock_acquire(&ioq->lock);
		ioq_wait(&ioq->consumer);
		lock_release(&ioq->lock);
	}
	char byte = ioq->buf[ioq->tail];
	ioq->tail = next_pos(ioq->tail);
	if (ioq->producer != NULL) {
		wakeup(&ioq->producer);
	}

	return byte;
}

void ioq_putchar(struct ioqueue *ioq, char byte) {
	ASSERT(intr_off == get_intr_stat());
	while (ioq_full(ioq)) {
		lock_acquire(&ioq->lock);
		ioq_wait(&ioq->producer);
		lock_release(&ioq->lock);
	}
	ioq->buf[ioq->head] = byte;
	ioq->head = next_pos(ioq->head);
	if (ioq->consumer != NULL) {
		wakeup(&ioq->consumer);
	}
}

size_t ioq_length(struct ioqueue *ioq) {
	size_t len = 0;
	if (ioq->head >= ioq->tail) {
		len = ioq->head - ioq->tail;
	} else {
		len = bufsize - (ioq->tail - ioq->head);
	}
	return len;
}
