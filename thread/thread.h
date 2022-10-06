#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "../kernel/memory.h"
#include "../lib/kernel/list.h"
#include "../lib/stdint.h"


typedef void thread_func(void *);


enum task_stat {
	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED,
	TASK_HANGING,
	TASK_WAITING,
	TASK_DIED
};

struct intr_stack {
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rbx;
	uint64_t rax;
	uint64_t sregs;
	uint64_t rbp;
	uint64_t err_code;
	void (*rip)(void);
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
};

struct thread_stack {
	uint64_t rsi;
	uint64_t rdi;
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t rbx;
	uint64_t rbp;
	uint64_t rip;
};

#define TASK_NAME_LEN 20
#define STACK_MAGIC 0x474d575a5a46594c
#define MAX_FILES_OPEN_PER_PROC 8

typedef int16_t pid_t;

struct task_struct {
	uint64_t *rbp_ptr;
	char name[TASK_NAME_LEN];
	enum task_stat stat;
	uint8_t prio;
	uint8_t ticks;
	struct list_elem general_tag;
	struct list_elem all_list_tag;

	pid_t pid;
	pid_t parent_pid;

	uint64_t *pml4;
	struct virt_pool u_v_pool;

	int8_t exit_status;

	int fd_table[MAX_FILES_OPEN_PER_PROC];
	size_t cwd_i_no;

	uint64_t istack_magic;
	uint8_t extern_stack[256];
	struct intr_stack intr_stack;
	uint64_t stack_magic;
};

extern struct task_struct *current;

extern struct task_struct *init_proc;

extern struct list all_threads_list;
extern struct list ready_threads_list;

void thread_init(void);
void schedule(void);
struct task_struct *thread_start(char *name, uint8_t prio, thread_func func,
				 void *arg);
struct task_struct *running_thread(void);
void thread_block(enum task_stat stat);
void thread_unblock(struct task_struct *pthread);

pid_t alloc_pid(void);
void release_pid(pid_t pid);

void thread_exit(struct task_struct *thread_over, bool need_schedule);
struct task_struct *pid2thread(pid_t pid);

void sys_ps(void);
void thread_yield(void);
#endif
