#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "../lib/kernel/bitmap.h"
#include "../lib/kernel/list.h"
#include "../lib/stdint.h"
#include "../thread/sync.h"

#define PG_SIZE 4096

struct mem_block {
	struct list_elem free_elem;
};

struct mem_desc {
	size_t block_size;
	size_t blocks_per_arena;
	struct list free_list;
};
#define DESC_CNT 7


#define V_POOL_BITS_MAX 256
#define POOL_LEN_MAX (V_POOL_BITS_MAX * 8 * PG_SIZE)
struct virt_pool {
	size_t start;
	uint8_t bits[V_POOL_BITS_MAX];
	struct bitmap pool_bitmap;
	struct mem_desc descs[DESC_CNT];
};

extern struct virt_pool k_v_pool;


void mem_init(void);
uint64_t addr_v2p(uint64_t vaddr);
void virt_pool_init(struct virt_pool *vpool, size_t start, size_t len);
void *alloc_pages(struct virt_pool *vpool, size_t cnt);
uint8_t map_stat(void *vaddr);
void free_pages(struct virt_pool *vpool, void *ptr, size_t cnt);
void *get_pages(size_t vaddr, size_t cnt);
void sys_free(void *ptr);
void *sys_malloc(size_t size);

void *sys_malloc(size_t size);
void sys_free(void *ptr);
void release_prog_mem(struct task_struct *pthread);
#endif
