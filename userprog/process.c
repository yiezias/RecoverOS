#include "process.h"
#include "../kernel/debug.h"
#include "../kernel/intr.h"
#include "../lib/string.h"
#include "../thread/thread.h"


static void start_process(void *filename) {
	void *function = filename;
	struct task_struct *cur = running_thread();
	struct intr_stack *intr_stack = (struct intr_stack *)(cur + 1);
	memset(intr_stack, 0, sizeof(struct intr_stack));
	intr_stack->rip = function;
	intr_stack->cs = SCT_U_CODE;
	intr_stack->ss = SCT_U_DATA;
	intr_stack->rflags = 0x202;
	ASSERT(get_pages(USER_STACK_VADDR - STACK_PAGE_CNT * PG_SIZE,
			 STACK_PAGE_CNT)
	       != NULL);
	intr_stack->rsp = USER_STACK_VADDR;
	intr_stack->sregs = 0;

	if (cur == init_proc) {
		thread_block(TASK_BLOCKED);
	}

	asm volatile("movq %0,%%rbp;jmp intr_exit" ::"g"(&intr_stack->rbp)
		     : "memory");
}

size_t pml4_paddr(struct task_struct *thread) {
	size_t p_pml4 = 0x100000;
	if (thread->pml4) {
		p_pml4 = addr_v2p((size_t)thread->pml4);
	}
	return p_pml4;
}

uint64_t *create_pml4(void) {
	uint64_t *pml4 = alloc_pages(&k_v_pool, 1);
	if (pml4 == NULL) {
		return NULL;
	}
	memset(pml4, 0, PG_SIZE);
	memcpy(pml4 + 256, (uint64_t *)0xfffffffffffff000 + 256, 2048);
	size_t paddr = addr_v2p((size_t)pml4);
	pml4[511] = paddr + 7;
	return pml4;
}

struct task_struct *process_execute(void *filename, char *name) {
	enum intr_stat old_stat = set_intr_stat(intr_off);
	struct task_struct *thread =
		thread_start(name, default_prio, start_process, filename);

	thread->pml4 = create_pml4();
	ASSERT(thread->pml4);
	virt_pool_init(&thread->u_v_pool, USER_START_ADDR, POOL_LEN_MAX);

	set_intr_stat(old_stat);

	return thread;
}
