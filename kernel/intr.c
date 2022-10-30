#include "intr.h"
#include "../lib/kernel/io.h"
#include "../lib/kernel/print.h"
#include "../lib/stdio.h"
#include "../thread/thread.h"
#include "global.h"

#define IDT_DESC_CNT 0x30
extern void *intr_ety_table[IDT_DESC_CNT];
void *intr_hdl_table[IDT_DESC_CNT];



#define RFLAGS_IF 0x200

enum intr_stat get_intr_stat() {
	uint64_t rflags = 0;
	asm volatile("pushfq;popq %0" : "=g"(rflags));
	return (RFLAGS_IF & rflags) ? intr_on : intr_off;
}

enum intr_stat set_intr_stat(enum intr_stat stat) {
	enum intr_stat old_stat = get_intr_stat();
	if (old_stat ^ stat) {
		switch (stat) {
		case intr_off:
			asm volatile("cli");
			break;
		case intr_on:
			asm volatile("sti");
			break;
		}
	}

	return old_stat;
}




#define PIC_M_CTRL 0x20	 //主片控制端口
#define PIC_M_DATA 0x21	 //主片数据端口
#define PIC_S_CTRL 0xa0	 //从片控制端口
#define PIC_S_DATA 0xa1	 //从片数据端口

static void pic_init(void) {
	put_str("pic_init: start\n");
	/*初始化主片*/
	outb(PIC_M_CTRL, 0x11);
	outb(PIC_M_DATA, 0x20);	 //初始中断向量号0x20

	outb(PIC_M_DATA, 0x04);	 // IR2接从片
	outb(PIC_M_DATA, 0x01);	 // 8086模式

	/*初始化从片*/
	outb(PIC_S_CTRL, 0x11);
	outb(PIC_S_DATA, 0x28);	 //初始中断向量号0x28

	outb(PIC_S_DATA, 0x02);	 //连接到主片IR2引脚
	outb(PIC_S_DATA, 0x01);

	/*打开主片IR0,只接受时钟中断*/
	outb(PIC_M_DATA, 0xfe);
	outb(PIC_S_DATA, 0xff);

	//接受来自从片中断
	outb(PIC_M_DATA, 0xf8);

	//接受硬盘控制器中断
	outb(PIC_S_DATA, 0xbf);

	put_str("pic_init: done\n");
}



static void general_intr_hdl(int intr_nr, uint64_t *rbp_ptr) {
	if (intr_nr == 0x27) {
		return;
	}
	put_info("\x1b\x0c\n\nintr_nr:\t", intr_nr);

	if (0x0e == intr_nr) {
		size_t page_fault_vaddr;
		asm("movq %%cr2, %0" : "=r"(page_fault_vaddr));
		// cr2是存放造成page_fault的地址
		put_info("page_fault_vaddr: ", page_fault_vaddr);
	}

	put_info("ss_old:\t", rbp_ptr[0]);
	put_info("rsp_old:\t", rbp_ptr[-1]);
	put_info("rflags_old:\t", rbp_ptr[-2]);
	put_info("cs_old:\t\t", rbp_ptr[-3]);
	put_info("rip_old:\t", rbp_ptr[-4]);
	printk("thread:\t%d, %s\n", current->pid, current->name);
	while (1) {}
}

void register_intr_hdl(int num, void *func) {
	intr_hdl_table[num] = func;
}

static void intr_hdl_init(void) {
	for (int i = 0; i != IDT_DESC_CNT; ++i) {
		register_intr_hdl(i, general_intr_hdl);
	}
}



struct idt_des {
	uint16_t off_low;
	uint16_t sct;
	uint8_t ist;
	uint8_t att;
	uint16_t off_mid;
	uint32_t off_high;
	uint32_t rsv;
} __attribute__((packed));

struct idt_des idt_table[IDT_DESC_CNT];

static void idt_init(void) {
	for (int i = 0; i != IDT_DESC_CNT; ++i) {
		uint64_t entry = (uint64_t)intr_ety_table[i];
		idt_table[i].off_high = (0xffffffff00000000 & entry) >> 32;
		idt_table[i].off_mid = (0xffff0000 & entry) >> 16;
		idt_table[i].off_low = 0xffff & entry;

		idt_table[i].ist = 1;
		idt_table[i].sct = CODE_SCT;
		idt_table[i].att = 0x8e;
	}

	uint64_t idt_ptr[2];
	idt_ptr[0] = (sizeof(idt_table) - 1) | ((uint64_t)idt_table << 16);
	idt_ptr[1] = (uint64_t)idt_table >> 48;
	asm volatile("lidt %0" ::"m"(idt_ptr));
}

void intr_init(void) {
	put_str("intr_init: start\n");

	pic_init();
	intr_hdl_init();
	idt_init();
	idt_table[0x20].ist = 2;

	put_str("intr_init: done\n");
}
