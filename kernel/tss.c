#include "tss.h"
#include "../lib/kernel/print.h"
#include "memory.h"

struct tss tss;

void tss_init() {
	put_str("tss_init start\n");

	struct tss_des *tss_des = (struct tss_des *)0x518;

	uint64_t tss_base = (uint64_t)&tss;

	tss_des->base0 = tss_base & 0xffff;
	tss_des->base1 = (tss_base & 0xff0000) >> 16;
	tss_des->base2 = (tss_base & 0xff000000) >> 24;
	tss_des->base3 = (tss_base & 0xffffffff00000000) >> 32;

	tss_des->len_low = sizeof(struct tss) & 0xffff;
	tss_des->len_high = sizeof(struct tss) & 0xf0000;
	tss_des->addr = 0x89;

	put_info("tss.ist1: ", tss.ist1);

	asm volatile("ltr	%w0" ::"r"(0x18));

	put_str("tss_init done\n");
}
