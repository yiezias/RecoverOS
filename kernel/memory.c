#include "memory.h"
#include "../lib/kernel/print.h"
#include "../lib/stdio.h"
#include "../lib/string.h"
#include "../thread/thread.h"
#include "debug.h"
#include "intr.h"

/*pml4e的偏移*/
static inline uint64_t pml4e_idx(uint64_t addr) {
	return (addr & 0xff8000000000) >> 39;
}

static inline uint64_t pdpte_idx(uint64_t addr) {
	return (addr & 0x007fc0000000) >> 30;
}

static inline uint64_t pde_idx(uint64_t addr) {
	return (addr & 0x3fe00000) >> 21;
}

static inline uint64_t pte_idx(uint64_t addr) {
	return (addr & 0x1ff000) >> 12;
}

/*pml4e地址*/
static inline uint64_t *pml4e_ptr(void *vaddr) {
	return (uint64_t *)(0xfffffffffffff000
			    + pml4e_idx((uint64_t)vaddr) * 8);
}

static inline uint64_t *pdpte_ptr(void *vaddr) {
	return (uint64_t *)(0xffffffffffe00000
			    + (pml4e_idx((uint64_t)vaddr) << 12)
			    + pdpte_idx((uint64_t)vaddr) * 8);
}

static inline uint64_t *pde_ptr(void *vaddr) {
	return (uint64_t *)(0xffffffffc0000000
			    + (pml4e_idx((uint64_t)vaddr) << 21)
			    + (pdpte_idx((uint64_t)vaddr) << 12)
			    + pde_idx((uint64_t)vaddr) * 8);
}

static inline uint64_t *pte_ptr(void *vaddr) {
	return (uint64_t *)((0xffffff8000000000)
			    + (pml4e_idx((uint64_t)vaddr) << 30)
			    + (pdpte_idx((uint64_t)vaddr) << 21)
			    + (pde_idx((uint64_t)vaddr) << 12)
			    + pte_idx((uint64_t)vaddr) * 8);
}

uint64_t addr_v2p(uint64_t vaddr) {
	return (*pte_ptr((void *)vaddr) & 0xfffffffffffff000) + (vaddr & 0xfff);
}




#define MEM_BITMAP_BASE 0xffff80000008e000

struct phy_pool {
	size_t start;
	struct bitmap pool_bitmap;
} phy_pool;

static void *palloc(void) {
	enum intr_stat old_stat = set_intr_stat(intr_off);
	int64_t idx = bitmap_alloc_a_bit(&phy_pool.pool_bitmap);
	set_intr_stat(old_stat);

	if (idx == -1) {
		return NULL;
	}

	return (void *)(phy_pool.start + PG_SIZE * idx);
}


uint64_t *(*fu[4])(void *) = { pml4e_ptr, pdpte_ptr, pde_ptr, pte_ptr };

uint8_t map_stat(void *vaddr) {
	for (int i = 0; i != 4; ++i) {
		if (!(*fu[i](vaddr) & 1)) {
			return i;
		}
	}
	return 4;
}

static void page_map(void *vaddr, void *paddr) {
	int stat = map_stat(vaddr);
	ASSERT(stat != 4);

	uint64_t *pg_cur = fu[stat](vaddr);
	while (stat != 3) {
		void *pg_paddr = palloc();
		ASSERT(pg_paddr != NULL);
		*pg_cur = (uint64_t)pg_paddr + 7;
		pg_cur = fu[++stat](vaddr);
		memset((void *)((uint64_t)pg_cur & 0xfffffffffffff000), 0,
		       PG_SIZE);
	}
	*pg_cur = (uint64_t)paddr + 7;
}




struct arena {
	size_t cnt;
	struct mem_desc *desc;
};

static void mem_desc_init(struct mem_desc *desc_arr) {
	size_t block_size = 16;
	for (size_t i = 0; i != DESC_CNT; ++i) {
		desc_arr[i].block_size = block_size;
		desc_arr[i].blocks_per_arena =
			(PG_SIZE - sizeof(struct arena)) / block_size;
		list_init(&desc_arr[i].free_list);
		block_size *= 2;
	}
}

#define K_HEAP_START 0xffff800000100000
struct virt_pool k_v_pool;

void virt_pool_init(struct virt_pool *vpool, size_t start, size_t len) {
	vpool->start = start;
	vpool->pool_bitmap.bits = vpool->bits;
	vpool->pool_bitmap.bytes_len = len / PG_SIZE / 8;
	memset(vpool->bits, 1, V_POOL_BITS_MAX);
	bitmap_init(&vpool->pool_bitmap);
	mem_desc_init(vpool->descs);
}

static void *vpool_alloc(struct virt_pool *vpool, size_t cnt) {
	enum intr_stat old_stat = set_intr_stat(intr_off);
	int64_t idx = bitmap_alloc(&vpool->pool_bitmap, cnt);
	set_intr_stat(old_stat);
	if (idx != -1) {
		return (void *)(idx * PG_SIZE + vpool->start);
	} else {
		return NULL;
	}
}

void *alloc_pages(struct virt_pool *vpool, size_t cnt) {
	void *vaddr = vpool_alloc(vpool, cnt);
	if (vaddr == NULL) {
		return NULL;
	}
	for (size_t i = 0; i != cnt; ++i) {
		void *paddr = palloc();
		if (paddr == NULL) {
			return NULL;
		}
		page_map(vaddr + i * PG_SIZE, paddr);
	}
	return vaddr;
}

static void flush_tlb_force(void) {
	void *cr3 = (void *)0x100000;
	if (running_thread()->pml4) {
		cr3 = (void *)addr_v2p((size_t)running_thread()->pml4);
	}
	asm volatile("movq %0,%%cr3" ::"a"(cr3));
}

void free_pages(struct virt_pool *vpool, void *ptr, size_t cnt) {
	for (size_t i = 0; i != cnt; ++i) {
		size_t vaddr = (size_t)ptr + i * PG_SIZE;
		size_t paddr = addr_v2p(vaddr);
		size_t pidx = (paddr - phy_pool.start) / PG_SIZE;

		uint64_t *pte = pte_ptr((void *)vaddr);
		ASSERT(*pte & 1);
		--*pte;
		asm volatile("invlpg %0" ::"m"(vaddr) : "memory");

		flush_tlb_force();

		enum intr_stat old_stat = set_intr_stat(intr_off);
		bitmap_set(&phy_pool.pool_bitmap, pidx, 1);
		set_intr_stat(old_stat);
	}

	enum intr_stat old_stat = set_intr_stat(intr_off);
	bitmap_free(&vpool->pool_bitmap, ((size_t)ptr - vpool->start) / PG_SIZE,
		    cnt);
	set_intr_stat(old_stat);
}


void *get_pages(size_t vaddr, size_t cnt) {
	for (size_t i = 0; i != cnt; ++i) {
		void *paddr = palloc();
		if (paddr == NULL) {
			return NULL;
		}
		page_map((void *)(vaddr + i * PG_SIZE), paddr);
	}

	return (void *)vaddr;
}


// 这个函数写得不好
void release_prog_mem(struct task_struct *pthread) {
	ASSERT(pthread->pml4);

	//位图比较小，直接全部遍历
	for (size_t i = 0; i != pthread->u_v_pool.pool_bitmap.bytes_len * 8;
	     ++i) {
		size_t vaddr = pthread->u_v_pool.start + i * PG_SIZE;
		int map_status = map_stat((void *)vaddr);
		if (map_status == 4) {
			free_pages(&pthread->u_v_pool, (void *)vaddr, 1);
			--map_status;
		}
		while (map_status--) {
			uint64_t page =
				0xfffffffffffff000
				& (uint64_t)fu[map_status + 1]((void *)vaddr);
			bitmap_set(&phy_pool.pool_bitmap,
				   (addr_v2p(page) - phy_pool.start) / PG_SIZE,
				   0);
		}
	}
}




static struct arena *block2arena(struct mem_block *b) {
	return (struct arena *)((size_t)b & 0xfffffffffffff000);
}

static struct mem_block *arena2block(struct arena *a, size_t block_idx) {
	return (struct mem_block *)((size_t)(a + 1)
				    + block_idx * a->desc->block_size);
}

static void create_free_blocks(struct virt_pool *vpool, size_t desc_idx) {
	struct arena *a = alloc_pages(vpool, 1);
	ASSERT(a != NULL);
	a->desc = &vpool->descs[desc_idx];
	a->cnt = vpool->descs[desc_idx].blocks_per_arena;

	for (size_t i = 0; i != a->cnt; ++i) {
		struct mem_block *b = arena2block(a, i);
		ASSERT(!elem_find(&a->desc->free_list, &b->free_elem));
		list_append(&a->desc->free_list, &b->free_elem);
	}
}

void *sys_malloc(size_t size) {
	struct task_struct *cur = running_thread();
	struct virt_pool *vpool =
		cur->pml4 == NULL ? &k_v_pool : &cur->u_v_pool;

	void *ret = NULL;
	if (size > 1024) {
		size_t pg_cnt =
			DIV_ROUND_UP(size + sizeof(struct arena), PG_SIZE);
		struct arena *a = alloc_pages(vpool, pg_cnt);
		if (a == NULL) {
			ret = NULL;
			goto sys_malloc_done;
		}
		a->cnt = size;
		a->cnt = pg_cnt;
		a->desc = NULL;
		ret = a + 1;
	} else {
		enum intr_stat old_stat = set_intr_stat(intr_off);
		size_t desc_idx = 0;
		for (; desc_idx != DESC_CNT; ++desc_idx) {
			if (vpool->descs[desc_idx].block_size >= size) {
				break;
			}
		}
		if (list_empty(&vpool->descs[desc_idx].free_list)) {
			create_free_blocks(vpool, desc_idx);
		}
		ret = elem2entry(struct mem_block, free_elem,
				 list_pop(&vpool->descs[desc_idx].free_list));
		struct arena *a = block2arena(ret);
		--a->cnt;
		set_intr_stat(old_stat);
	}

sys_malloc_done:
	return ret;
}

void sys_free(void *ptr) {
	ASSERT(ptr != NULL);
	struct task_struct *cur = running_thread();
	struct virt_pool *vpool =
		cur->pml4 == NULL ? &k_v_pool : &cur->u_v_pool;
	struct arena *a = block2arena(ptr);

	if (a->desc == NULL) {
		free_pages(vpool, a, a->cnt);
	} else {
		enum intr_stat old_stat = set_intr_stat(intr_off);
		struct mem_block *b = ptr;
		ASSERT(!elem_find(&a->desc->free_list, &b->free_elem));
		list_append(&a->desc->free_list, &b->free_elem);
		if (++a->cnt == a->desc->blocks_per_arena) {
			for (size_t i = 0; i != a->desc->blocks_per_arena;
			     ++i) {
				struct mem_block *b = arena2block(a, i);
				ASSERT(elem_find(&a->desc->free_list,
						 &b->free_elem));
				list_remove(&b->free_elem);
			}
			free_pages(vpool, a, 1);
		}
		set_intr_stat(old_stat);
	}
}




static void mem_pool_init(size_t memsize) {
	put_str("mem_pool_init: start\n");
	size_t page_cnt = memsize / PG_SIZE;
	size_t used_pages = 0x100000 / PG_SIZE + 4;
	size_t bytes_len = (page_cnt - used_pages) / 8;

	phy_pool.start = used_pages * PG_SIZE;
	phy_pool.pool_bitmap.bytes_len = bytes_len;
	phy_pool.pool_bitmap.bits = (uint8_t *)MEM_BITMAP_BASE;

	put_info("phy_pool.start: ", phy_pool.start);

	put_info("phy_pool.pool_bitmap.bits: ",
		 (uint64_t)phy_pool.pool_bitmap.bits);

	put_info("phy_pool.pool_bitmap.bytes_len: ",
		 phy_pool.pool_bitmap.bytes_len);

	bitmap_init(&phy_pool.pool_bitmap);

	put_str("mem_pool_init: done\n");
}



void mem_init(void) {
	put_str("mem_init: start\n");
	mem_pool_init(0x40000000);
	virt_pool_init(&k_v_pool, K_HEAP_START, POOL_LEN_MAX);

	put_str("mem_init: done\n");
}
