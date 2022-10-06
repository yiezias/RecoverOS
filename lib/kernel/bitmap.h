#ifndef __LIB_KERNEL_BITMAP_H
#define __LIB_KERNEL_BITMAP_H
#include "../../kernel/global.h"
#include "../stdint.h"

struct bitmap {
	size_t bytes_len;
	uint8_t *bits;
};

void bitmap_init(struct bitmap *btmp);
bool bitmap_read(struct bitmap *btmp, uint64_t idx);
void bitmap_set(struct bitmap *btmp, uint64_t idx, bool value);
int64_t bitmap_alloc_a_bit(struct bitmap *btmp);
int64_t bitmap_alloc(struct bitmap *btmp, uint64_t cnt);
void bitmap_free(struct bitmap *btmp, size_t idx, size_t cnt);

#endif
