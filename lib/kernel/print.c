#include "print.h"
#include "../../kernel/global.h"
#include "../string.h"
#include "io.h"

#define D_PROPERTY 0x07	 // 0x07红底白字 0x0c红字黑底高亮
#define D_BASE_ADDR 0xffff8000000b8000

uint8_t d_property = D_PROPERTY;

uint16_t get_cur(void) {
	unsigned char curl, curr;

	outb(0x3d4, 0x0e);
	curl = inb(0x3d5);

	outb(0x3d4, 0x0f);
	curr = inb(0x3d5);

	return (curl << 8) + curr;
}

void set_cur(uint16_t cur) {
	if (cur >= (80 * 25)) {
		memcpy((void *)(D_BASE_ADDR - 160), (void *)D_BASE_ADDR,
		       2 * 80 * 26);
		cur -= 80;
	}

	outb(0x3d4, 0x0f);
	outb(0x3d5, cur);

	cur >>= 8;

	outb(0x3d4, 0x0e);
	outb(0x3d5, cur);
}

void put_char(char c) {
	static bool change_property = false;
	if (change_property) {
		d_property = c;
		change_property = false;
		return;
	}

	uint16_t cur = get_cur();

	switch (c) {
	case '\x1b':
		change_property = true;
		return;
	case '\r':
		set_cur(cur / 80 * 80);
		break;
	case '\n':
		set_cur((cur / 80 + 1) * 80);
		break;
	case '\b':
		set_cur(--cur);
		*(uint16_t *)(D_BASE_ADDR + cur * 2) = ' ' + (d_property << 8);
		break;
	case '\t': {
		uint16_t new_cur = (cur / 8 + 1) * 8;
		while (cur <= new_cur)
			*(uint16_t *)(D_BASE_ADDR + cur++ * 2) =
				' ' + (d_property << 8);
		set_cur(new_cur);
		break;
	}
	default:
		*(uint16_t *)(D_BASE_ADDR + cur * 2) = c + (d_property << 8);
		set_cur(++cur);
		break;
	}
}


void put_str(char *str) {
	while (*str) {
		put_char(*str++);
	}
}

void put_num(uint64_t num) {
	char dbuf[16];
	for (int i = 0; i != 16; ++i) {
		char c = num % 16 + '0';
		if (c > '9') {
			c += ('a' - '9' - 1);
		}
		dbuf[i] = c;
		num = num / 16;
	}
	int i = 16, suf = 0;
	while (--i > 0) {
		if (dbuf[i] != '0' || suf) {
			put_char(dbuf[i]);
			suf = 1;
		}
	}
	put_char(dbuf[0]);
}

void put_info(char *message, size_t num) {
	put_str(message);
	put_num(num);
	put_char('\n');
}


void cls_screen(void) {
	short *dis_addr = (short *)D_BASE_ADDR;

	for (int i = 0; i != 80 * 25; ++i) {
		dis_addr[i] = ' ' + (d_property << 8);
	}

	set_cur(0);
}
