#include "thread.h"
#include "../kernel/debug.h"
#include "../kernel/intr.h"
#include "../kernel/memory.h"
#include "../kernel/tss.h"
#include "../lib/kernel/print.h"
#include "../lib/stdio.h"
#include "../lib/string.h"
#include "../userprog/process.h"
#include "sync.h"

struct list all_threads_list;
struct list ready_threads_list;

extern void switch_to(struct task_struct *pre, struct task_struct *next);

struct task_struct *current = (struct task_struct *)0xffff80000009d000;
struct task_struct *main_thread = (struct task_struct *)0xffff80000009d000;

struct task_struct *idle_thread;


struct task_struct *init_proc;

extern void init(void);


static void idle(void *arg UNUSED) {
	while (1) {
		thread_block(TASK_BLOCKED);
		asm volatile("sti; hlt" : : : "memory");
	}
}


static void kernel_thread(thread_func func, void *arg) {
	set_intr_stat(intr_on);
	func(arg);
}

struct task_struct *running_thread(void) {
	enum intr_stat old_stat = set_intr_stat(intr_off);
	struct task_struct *running = current;
	set_intr_stat(old_stat);

	return running;
}


uint8_t pid_bitmap_bits[128] = { 0 };
struct pid_pool {
	struct bitmap pid_bitmap;
	pid_t pid_start;
	struct lock pid_lock;
} pid_pool;

static void pid_pool_init(void) {
	pid_pool.pid_start = 1;
	pid_pool.pid_bitmap.bits = pid_bitmap_bits;
	pid_pool.pid_bitmap.bytes_len = 128;
	bitmap_init(&pid_pool.pid_bitmap);
	lock_init(&pid_pool.pid_lock);
}

pid_t alloc_pid(void) {
	lock_acquire(&pid_pool.pid_lock);
	size_t bit_idx = bitmap_alloc_a_bit(&pid_pool.pid_bitmap);
	lock_release(&pid_pool.pid_lock);
	return (bit_idx + pid_pool.pid_start);
}

void release_pid(pid_t pid) {
	lock_acquire(&pid_pool.pid_lock);
	bitmap_set(&pid_pool.pid_bitmap, pid - pid_pool.pid_start, 0);
	lock_release(&pid_pool.pid_lock);
}


void thread_exit(struct task_struct *thread_over, bool need_schedule) {
	enum intr_stat old_stat = set_intr_stat(intr_off);

	thread_over->stat = TASK_DIED;
	if (elem_find(&all_threads_list, &thread_over->general_tag)) {
		list_remove(&thread_over->general_tag);
	}
	if (thread_over->pml4) {
		free_pages(&k_v_pool, thread_over->pml4, 1);
	}
	list_remove(&thread_over->all_list_tag);
	release_pid(thread_over->pid);

	if (thread_over != main_thread) {
		free_pages(&k_v_pool, thread_over, 1);
	}

	if (need_schedule) {
		schedule();
		PANIC("thread_exit: should not be here\n");
	}

	set_intr_stat(old_stat);
}


static bool pid_check(struct list_elem *elem, void *pid_ptr) {
	struct task_struct *thread =
		elem2entry(struct task_struct, all_list_tag, elem);

	if (thread->pid == *(pid_t *)pid_ptr) {
		return true;
	} else {
		return false;
	}
}

struct task_struct *pid2thread(pid_t pid) {
	struct list_elem *elem =
		list_traversal(&all_threads_list, pid_check, &pid);
	if (elem == NULL) {
		return NULL;
	}
	return elem2entry(struct task_struct, all_list_tag, elem);
}


static bool print_thread_info(struct list_elem *elem, void *arg UNUSED) {
	struct task_struct *thread =
		elem2entry(struct task_struct, all_list_tag, elem);
	char stat_name[16] = { 0 };
	switch (thread->stat) {
	case TASK_READY:
		strcpy(stat_name, "READY");
		break;
	case TASK_DIED:
		strcpy(stat_name, "DIED");
		break;
	case TASK_BLOCKED:
		strcpy(stat_name, "BLOCKED");
		break;
	case TASK_HANGING:
		strcpy(stat_name, "HANGING");
		break;
	case TASK_RUNNING:
		strcpy(stat_name, "RUNNING");
		break;
	case TASK_WAITING:
		strcpy(stat_name, "WAITING");
		break;
	}
	printk("%d\t%d\t%s\t%d\t%s\n", thread->pid, thread->parent_pid,
	       stat_name, thread->ticks, thread->name);
	return false;
}

void sys_ps(void) {
	printk("PID\tPPID\tSTAT\tTICKS\tCOMMAND\n\n");
	list_traversal(&all_threads_list, print_thread_info, NULL);
}




static void init_thread(struct task_struct *thread, char *name, uint8_t prio) {
	thread->pid = alloc_pid();
	strcpy(thread->name, name);
	thread->prio = thread->ticks = prio;
	thread->stat = TASK_READY;
	thread->pml4 = NULL;
	thread->stack_magic = STACK_MAGIC;
	thread->istack_magic = STACK_MAGIC;
	thread->cwd_i_no = 0;

	thread->fd_table[0] = 0;
	thread->fd_table[1] = 1;
	thread->fd_table[2] = 2;
	for (int fd_idx = 3; fd_idx != MAX_FILES_OPEN_PER_PROC; ++fd_idx) {
		thread->fd_table[fd_idx] = -1;
	}
}

static void create_thread(struct task_struct *thread, thread_func func,
			  void *arg) {
	struct thread_stack *thread_stack =
		(void *)thread + PG_SIZE - sizeof(struct thread_stack);
	thread->rbp_ptr = &thread_stack->rbp;
	thread_stack->rsi = (uint64_t)arg;
	thread_stack->rdi = (uint64_t)func;
	thread_stack->rip = (uint64_t)kernel_thread;
}


struct task_struct *thread_start(char *name, uint8_t prio, thread_func func,
				 void *arg) {
	struct task_struct *thread = alloc_pages(&k_v_pool, 1);
	ASSERT(thread != NULL);

	init_thread(thread, name, prio);
	create_thread(thread, func, arg);

	ASSERT(!elem_find(&all_threads_list, &thread->all_list_tag));
	list_append(&all_threads_list, &thread->all_list_tag);
	ASSERT(!elem_find(&ready_threads_list, &thread->general_tag));
	list_append(&ready_threads_list, &thread->general_tag);

	return thread;
}


void schedule(void) {
	ASSERT(intr_off == get_intr_stat());

	if (current->stat == TASK_RUNNING) {
		current->stat = TASK_READY;
		current->ticks = current->prio;
		ASSERT(!elem_find(&ready_threads_list, &current->general_tag));
		list_append(&ready_threads_list, &current->general_tag);
	}

	if (list_empty(&ready_threads_list)) {
		thread_unblock(idle_thread);
	}

	struct task_struct *next = elem2entry(struct task_struct, general_tag,
					      list_pop(&ready_threads_list));

	next->stat = TASK_RUNNING;
	struct task_struct *prev = current;
	current = next;

	tss.ist2 = (uint64_t)&next->intr_stack + sizeof(struct intr_stack);
	switch_to(prev, next);
}


void thread_block(enum task_stat stat) {
	ASSERT(stat == TASK_BLOCKED || stat == TASK_HANGING
	       || stat == TASK_WAITING);

	enum intr_stat old_stat = set_intr_stat(intr_off);

	current->stat = stat;
	schedule();

	set_intr_stat(old_stat);
}

void thread_unblock(struct task_struct *pthread) {
	ASSERT(pthread->stat == TASK_BLOCKED || pthread->stat == TASK_HANGING
	       || pthread->stat == TASK_WAITING);
	enum intr_stat old_stat = set_intr_stat(intr_off);

	ASSERT(!elem_find(&ready_threads_list, &pthread->general_tag));
	list_push(&ready_threads_list, &pthread->general_tag);
	pthread->stat = TASK_READY;

	set_intr_stat(old_stat);
}

void thread_yield(void) {
	struct task_struct *cur = running_thread();
	enum intr_stat old_stat = set_intr_stat(intr_off);

	ASSERT(!elem_find(&ready_threads_list, &cur->general_tag));
	list_append(&ready_threads_list, &cur->general_tag);
	cur->stat = TASK_READY;
	schedule();
	set_intr_stat(old_stat);
}


static void make_main_thread(void) {
	init_thread(main_thread, "main", 31);
	main_thread->stat = TASK_RUNNING;
	ASSERT(!elem_find(&all_threads_list, &main_thread->all_list_tag));
	list_append(&all_threads_list, &main_thread->all_list_tag);
	tss.ist2 = (size_t)&main_thread->intr_stack + sizeof(struct intr_stack);
}


void thread_init() {
	put_str("thread_init: start\n");
	list_init(&all_threads_list);
	list_init(&ready_threads_list);
	pid_pool_init();

	init_proc = process_execute(init, "init");

	make_main_thread();
	idle_thread = thread_start("idle", 10, idle, NULL);
	tss.ist1 = (size_t)alloc_pages(&k_v_pool, 1) + PG_SIZE;

	put_str("thread_init: done\n");
}
