#include "fork.h"
#include "../fs/file.h"
#include "../kernel/debug.h"
#include "../kernel/intr.h"
#include "../lib/stdio.h"
#include "../lib/string.h"
#include "../pipe/pipe.h"
#include "process.h"

extern void system_ret(void);

static int copy_pcb(struct task_struct *child_thread,
		    struct task_struct *parent_thread) {
	memcpy(child_thread, parent_thread, PG_SIZE);
	child_thread->pid = alloc_pid();
	child_thread->stat = TASK_READY;
	child_thread->ticks = child_thread->prio;
	child_thread->parent_pid = parent_thread->pid;

	child_thread->u_v_pool.pool_bitmap.bits = child_thread->u_v_pool.bits;
	for (int i = 0; i != DESC_CNT; ++i) {
		if (list_empty(&parent_thread->u_v_pool.descs[i].free_list)) {
			list_init(&child_thread->u_v_pool.descs[i].free_list);
		}
	}

	//	ASSERT(strlen(child_thread->name) < 11);
	//	strcat(child_thread->name, "_fork");
	return 0;
}

uint64_t rbp;
static int build_child_environment(struct task_struct *child_thread) {
	struct thread_stack *thread_stack =
		(struct thread_stack *)((size_t)child_thread + PG_SIZE
					- sizeof(struct thread_stack));
	memset(thread_stack, 0, sizeof(struct thread_stack));
	child_thread->rbp_ptr = &thread_stack->rbp;
	thread_stack->rbp = rbp;
	thread_stack->rip = (uint64_t)system_ret;

	//修改返回值
	*(uint64_t *)(rbp - 8) = 0;

	return 0;
}

static void update_inode_open_cnts(struct task_struct *thread) {
	for (int local_fd = 3; local_fd != MAX_FILES_OPEN_PER_PROC;
	     ++local_fd) {
		int global_fd = thread->fd_table[local_fd];
		ASSERT(global_fd < MAX_FILES_OPEN);
		if (global_fd != -1) {
			if (is_pipe(local_fd)) {
				++file_table[global_fd].fd_pos;
			} else {
				++file_table[global_fd].fd_inode->i_open_cnts;
			}
		}
	}
	return;
}

uint64_t rsp;
//全局变量防止栈切换时被覆盖
static void copy_body_stack(struct task_struct *child_thread,
			    struct task_struct *parent_thread, void *buf_page) {
	memcpy(buf_page, (void *)USER_STACK_VADDR - STACK_PAGE_CNT * PG_SIZE,
	       STACK_PAGE_CNT * PG_SIZE);
	asm volatile(
		"movq %0,%%cr3" ::"a"(addr_v2p((size_t)child_thread->pml4)));
	get_pages(USER_STACK_VADDR - STACK_PAGE_CNT * PG_SIZE, STACK_PAGE_CNT);
	memcpy((void *)USER_STACK_VADDR - STACK_PAGE_CNT * PG_SIZE, buf_page,
	       STACK_PAGE_CNT * PG_SIZE);
	asm volatile(
		"movq %0,%%cr3" ::"a"(addr_v2p((size_t)parent_thread->pml4)));

	for (size_t i = 0;
	     i != child_thread->u_v_pool.pool_bitmap.bytes_len * 8; ++i) {
		if (bitmap_read(&child_thread->u_v_pool.pool_bitmap, i)) {
			size_t vaddr =
				i * PG_SIZE + child_thread->u_v_pool.start;
			memcpy((void *)buf_page, (void *)vaddr, PG_SIZE);
			asm volatile("movq %0,%%cr3" ::"a"(
				addr_v2p((size_t)child_thread->pml4)));
			get_pages(vaddr, 1);
			memcpy((void *)vaddr, buf_page, PG_SIZE);
			asm volatile("movq %0,%%cr3" ::"a"(
				addr_v2p((size_t)parent_thread->pml4)));
		}
	}
}

static int copy_process(struct task_struct *child_thread,
			struct task_struct *parent_thread) {
	void *buf_page = alloc_pages(&k_v_pool, STACK_PAGE_CNT);
	if (buf_page == NULL) {
		return -1;
	}
	if (copy_pcb(child_thread, parent_thread) == -1) {
		return -1;
	}
	child_thread->pml4 = create_pml4();
	if (child_thread->pml4 == NULL) {
		return -1;
	}

	void *tmp_stack = alloc_pages(&k_v_pool, 1);
	if (tmp_stack == NULL) {
		return -1;
	}
	asm volatile("movq %%rsp,%0" : "=a"(rsp));
	asm volatile("movq %0,%%rsp" : : "g"(tmp_stack + PG_SIZE));

	build_child_environment(child_thread);

	copy_body_stack(child_thread, parent_thread, buf_page);
	asm volatile("movq %0,%%rsp" ::"g"(rsp));
	free_pages(&k_v_pool, tmp_stack, 1);

	update_inode_open_cnts(child_thread);

	free_pages(&k_v_pool, buf_page, STACK_PAGE_CNT);
	return 0;
}

pid_t sys_fork(void) {
	asm volatile("movq (%%rbp),%0" : "=a"(rbp));
	struct task_struct *parent_thread = running_thread();
	struct task_struct *child_thread = alloc_pages(&k_v_pool, 1);

	if (child_thread == NULL) {
		return -1;
	}
	ASSERT(intr_off == get_intr_stat() && parent_thread->pml4 != NULL);

	if (copy_process(child_thread, parent_thread) == -1) {
		return -1;
	}

	ASSERT(!elem_find(&ready_threads_list, &child_thread->general_tag));
	list_append(&ready_threads_list, &child_thread->general_tag);
	ASSERT(!elem_find(&all_threads_list, &child_thread->all_list_tag));
	list_append(&all_threads_list, &child_thread->all_list_tag);

	return child_thread->pid;
}
