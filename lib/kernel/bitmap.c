#include "bitmap.h"
#include "../../kernel/debug.h"
#include "../string.h"

void bitmap_init(struct bitmap *btmp) {
	memset(btmp->bits, 0, btmp->bytes_len);
}

bool bitmap_read(struct bitmap *btmp, size_t idx) {
	size_t byte_idx = idx / 8;
	size_t bit_idx = idx % 8;
	return (btmp->bits[byte_idx] >> bit_idx) & 1;
}

void bitmap_set(struct bitmap *btmp, size_t idx, bool value) {
	ASSERT(value == 0 || value == 1);
	size_t byte_idx = idx / 8;
	size_t bit_idx = idx % 8;
	if (value) {
		btmp->bits[byte_idx] |= (1 << bit_idx);
	} else {
		btmp->bits[byte_idx] &= ~(1 << bit_idx);
	}
}

int64_t bitmap_alloc_a_bit(struct bitmap *btmp) {
	for (size_t i = 0; i != 8 * btmp->bytes_len; ++i) {
		if (0 == bitmap_read(btmp, i)) {
			bitmap_set(btmp, i, 1);
			return i;
		}
	}
	return -1;
}

int64_t bitmap_alloc(struct bitmap *btmp, size_t cnt) {
	for (size_t i = 0; i < 8 * btmp->bytes_len; ++i) {
		if (0 == bitmap_read(btmp, i)) {
			size_t j = 0;
			for (; j != cnt; ++j) {
				if (1 == bitmap_read(btmp, i + j)) {
					break;
				}
			}
			if (j == cnt) {
				for (size_t k = 0; k != cnt; ++k) {
					bitmap_set(btmp, i + k, 1);
				}
				return i;
			} else {
				i += j;
			}
		}
	}
	return -1;
}

void bitmap_free(struct bitmap *btmp, size_t idx, size_t cnt) {
	for (size_t i = 0; i != cnt; ++i) {
		ASSERT(bitmap_read(btmp, idx + i) == 1);
		bitmap_set(btmp, idx + i, 0);
	}
}
