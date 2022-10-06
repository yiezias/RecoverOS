#ifndef __KERNEL_TSS_H
#define __KERNEL_TSS_H

#include "../lib/stdint.h"

struct tss_des {
	uint16_t len_low;
	uint16_t base0;
	uint8_t base1;
	uint8_t addr;
	uint8_t len_high;
	uint8_t base2;
	uint32_t base3;
	uint32_t rsv;
};

struct tss {
	uint32_t rsv0;
	uint64_t rsp0;
	uint64_t rsp1;
	uint64_t rsp2;
	uint64_t rsv1;
	uint64_t ist1;
	uint64_t ist2;
	uint64_t ist3;
	uint64_t ist4;
	uint64_t ist5;
	uint64_t ist6;
	uint64_t ist7;
	uint64_t rsv2;
	uint16_t rsv3;
	uint16_t io_base;
} __attribute__((packed));

void tss_init(void);

extern struct tss tss;

#endif
