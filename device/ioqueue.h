#ifndef __DEVICE_IOQUEUE_H
#define __DEVICE_IOQUEUE_H
#include "../thread/sync.h"

#define bufsize 2048

struct ioqueue {
	struct lock lock;
	struct task_struct *producer;
	struct task_struct *consumer;
	uint8_t buf[bufsize];
	ssize_t head;
	ssize_t tail;
};

void ioqueue_init(struct ioqueue *ioq);
bool ioq_full(struct ioqueue *ioq);
bool ioq_empty(struct ioqueue *ioq);
char ioq_getchar(struct ioqueue *ioq);
void ioq_putchar(struct ioqueue *ioq, char byte);
size_t ioq_length(struct ioqueue *ioq);

#endif
