#include "ide.h"
#include "../kernel/debug.h"
#include "../kernel/intr.h"
#include "../kernel/memory.h"
#include "../lib/kernel/io.h"
#include "../lib/stdio.h"
#include "../lib/string.h"
#include "timer.h"

/* 定义硬盘各寄存器的端口号 */
#define reg_data(channel) (channel->port_base + 0)
#define reg_error(channel) (channel->port_base + 1)
#define reg_sect_cnt(channel) (channel->port_base + 2)
#define reg_lba_l(channel) (channel->port_base + 3)
#define reg_lba_m(channel) (channel->port_base + 4)
#define reg_lba_h(channel) (channel->port_base + 5)
#define reg_dev(channel) (channel->port_base + 6)
#define reg_status(channel) (channel->port_base + 7)
#define reg_cmd(channel) (reg_status(channel))
#define reg_alt_status(channel) (channel->port_base + 0x206)
#define reg_ctl(channel) reg_alt_status(channel)

/* reg_status寄存器的一些关键位 */
#define BIT_STAT_BSY 0x80   // 硬盘忙
#define BIT_STAT_DRDY 0x40  // 驱动器准备好
#define BIT_STAT_DRQ 0x8    // 数据传输准备好了

/* device寄存器的一些关键位 */
#define BIT_DEV_MBS 0xa0  // 第7位和第5位固定为1
#define BIT_DEV_LBA 0x40
#define BIT_DEV_DEV 0x10

/* 一些硬盘操作的指令 */
#define CMD_IDENTIFY 0xec      // identify指令
#define CMD_READ_SECTOR 0x20   // 读扇区指令
#define CMD_WRITE_SECTOR 0x30  // 写扇区指令

struct list partition_list;
struct ide_channel channels[2];
uint8_t channel_cnt;

static void select_disk(struct disk *hd) {
	uint8_t reg_device = BIT_DEV_MBS | BIT_DEV_LBA;
	if (hd->dev_no == 1) {
		reg_device |= BIT_DEV_DEV;
	}
	outb(reg_dev(hd->my_channel), reg_device);
}

static void select_sector(struct disk *hd, uint32_t lba, uint8_t sec_cnt) {
	struct ide_channel *channel = hd->my_channel;

	outb(reg_sect_cnt(channel), sec_cnt);

	outb(reg_lba_l(channel), lba);
	outb(reg_lba_m(channel), lba >> 8);
	outb(reg_lba_h(channel), lba >> 16);
	outb(reg_dev(channel), BIT_DEV_MBS | BIT_DEV_LBA
				       | (hd->dev_no == 1 ? BIT_DEV_DEV : 0)
				       | lba >> 24);
}

static bool busy_wait(struct disk *hd) {
	struct ide_channel *channel = hd->my_channel;
	uint16_t time_limit = 30 * 1000;

	while (time_limit -= 10 > 0) {
		if (!(inb(reg_status(channel)) & BIT_STAT_BSY)) {
			return (inb(reg_status(channel)) & BIT_STAT_DRQ);
		} else {
			mtime_sleep(10);
		}
	}
	return false;
}

static void cmd_out(struct ide_channel *channel, uint8_t cmd) {
	channel->expecting_intr = true;
	outb(reg_cmd(channel), cmd);
}

static void read_from_sector(struct disk *hd, void *buf, uint8_t sec_cnt) {
	uint32_t size_in_byte;
	if (sec_cnt == 0) {
		size_in_byte = 256 * 512;
	} else {
		size_in_byte = sec_cnt * 512;
	}
	insw(reg_data(hd->my_channel), buf, size_in_byte / 2);
}

static void write2sector(struct disk *hd, void *buf, uint8_t sec_cnt) {
	uint32_t size_in_byte;
	if (sec_cnt == 0) {
		size_in_byte = 256 * 512;
	} else {
		size_in_byte = sec_cnt * 512;
	}
	outsw(reg_data(hd->my_channel), buf, size_in_byte / 2);
}


void ide_read(struct disk *hd, uint32_t lba, void *buf, uint32_t sec_cnt) {
	ASSERT(sec_cnt > 0);
	lock_acquire(&hd->my_channel->lock);

	select_disk(hd);

	uint32_t secs_op;
	uint32_t secs_done = 0;
	while (secs_done < sec_cnt) {
		if ((secs_done + 256) <= sec_cnt) {
			secs_op = 256;
		} else {
			secs_op = sec_cnt - secs_done;
		}

		select_sector(hd, lba + secs_done, secs_op);
		cmd_out(hd->my_channel, CMD_READ_SECTOR);
		sema_down(&hd->my_channel->disk_done);

		if (!busy_wait(hd)) {
			char error[64];
			sprintf(error, "%s read sector %d failed!!!!!\n",
				hd->name, lba);
			PANIC(error);
		}
		read_from_sector(hd, (void *)((size_t)buf + secs_done * 512),
				 secs_op);
		secs_done += secs_op;
	}
	lock_release(&hd->my_channel->lock);
}

void ide_write(struct disk *hd, uint32_t lba, void *buf, uint32_t sec_cnt) {
	ASSERT(sec_cnt > 0);
	lock_acquire(&hd->my_channel->lock);

	select_disk(hd);

	uint32_t secs_op;
	uint32_t secs_done = 0;
	while (secs_done < sec_cnt) {
		if ((secs_done + 256) <= sec_cnt) {
			secs_op = 256;
		} else {
			secs_op = sec_cnt - secs_done;
		}

		select_sector(hd, lba + secs_done, secs_op);
		cmd_out(hd->my_channel, CMD_WRITE_SECTOR);

		if (!busy_wait(hd)) {
			char error[64];
			sprintf(error, "%s write sector %d failed!!!!!\n",
				hd->name, lba);
			PANIC(error);
		}
		write2sector(hd, (void *)((size_t)buf + secs_done * 512),
			     secs_op);
		sema_down(&hd->my_channel->disk_done);
		secs_done += secs_op;
	}
	lock_release(&hd->my_channel->lock);
}


static void swap_pairs_bytes(const char *dst, char *buf, size_t len) {
	uint8_t idx;
	for (idx = 0; idx < len; idx += 2) {
		buf[idx + 1] = *dst++;
		buf[idx] = *dst++;
	}

	buf[idx] = '\0';
}

static void identify_disk(struct disk *hd) {
	char id_info[512];
	select_disk(hd);
	cmd_out(hd->my_channel, CMD_IDENTIFY);
	sema_down(&hd->my_channel->disk_done);

	if (!busy_wait(hd)) {
		char error[64];
		sprintf(error, "%s identify failed!!!!!\n ", hd->name);
		PANIC(error);
	}
	read_from_sector(hd, id_info, 1);

	char buf[64];
	uint8_t sn_start = 10 * 2, sn_len = 20, md_start = 27 * 2, md_len = 40;
	swap_pairs_bytes(&id_info[sn_start], buf, sn_len);
	printk("disk %s info:\nSN:\t%s\n", hd->name, buf);
	memset(buf, 0, sizeof(buf));
	swap_pairs_bytes(&id_info[md_start], buf, md_len);
	printk("MODULE:\t%s\n", buf);
	uint32_t sectors = *(uint32_t *)&id_info[60 * 2];
	printk("SECTORS:\t%d\n", sectors);
	printk("CAPACITY:\t%dMB\n", sectors * 512 / 1024 / 1024);
}


struct partition_table_entry {
	uint8_t bootable;
	uint8_t start_head;
	uint8_t start_sec;
	uint8_t start_chs;
	uint8_t fs_type;
	uint8_t end_head;
	uint8_t end_sec;
	uint8_t end_chs;

	uint32_t start_lba;
	uint32_t sec_cnt;
} __attribute__((packed));

struct boot_sector {
	uint8_t code[446];
	struct partition_table_entry partition_table[4];
	uint16_t signature;
} __attribute__((packed));

static void partition_scan(struct disk *hd) {
	struct boot_sector *bs_buf = sys_malloc(512);
	ide_read(hd, 0, bs_buf, 1);
	struct partition_table_entry *p = bs_buf->partition_table;
	for (int i = 0; i != 4; ++i) {
		if (p[i].fs_type != 0) {
			hd->prim_parts[i].start_lba = p[i].start_lba;
			hd->prim_parts[i].sec_cnt = p[i].sec_cnt;
			hd->prim_parts[i].my_disk = hd;
			list_append(&partition_list,
				    &hd->prim_parts[i].part_tag);
			sprintf(hd->prim_parts[i].name, "%s%d", hd->name,
				i + 1);
		}
	}
	sys_free(bs_buf);
}


static bool partition_info(struct list_elem *elem, void *arg UNUSED) {
	struct partition *part = elem2entry(struct partition, part_tag, elem);
	printk("%s start_lba: 0x%x, sec_cnt: 0x%x\n", part->name,
	       part->start_lba, part->sec_cnt);
	return false;
}


void intr_hd_hdl(uint8_t irq_no) {
	ASSERT(irq_no == 0x2e || irq_no == 0x2f);
	uint8_t ch_no = irq_no - 0x2e;
	struct ide_channel *channel = &channels[ch_no];
	ASSERT(channel->irq_no == irq_no);
	if (channel->expecting_intr) {
		channel->expecting_intr = false;
		sema_up(&channel->disk_done);
		inb(reg_status(channel));
	}
}

static void channels_init(void) {
	uint8_t hd_cnt = *((uint8_t *)(0x475));
	ASSERT(hd_cnt > 0);
	list_init(&partition_list);
	channel_cnt = DIV_ROUND_UP(hd_cnt, 2);

	// 为什么作者不用for循环？
	for (int i = 0; i != channel_cnt; ++i) {
		switch (i) {
		case 0:
			channels[i].port_base = 0x1f0;
			channels[i].irq_no = 0x20 + 14;
			break;
		case 1:
			channels[i].port_base = 0x170;
			channels[i].irq_no = 0x20 + 15;
			break;
		}
		channels[i].expecting_intr = false;
		lock_init(&channels[i].lock);
		sema_init(&channels[i].disk_done, 0);
		register_intr_hdl(channels[i].irq_no, intr_hd_hdl);
		for (int j = 0; j != 2; ++j) {
			uint8_t dev_no = i * 2 + j;
			if (dev_no >= hd_cnt) {
				return;
			}
			sprintf(channels[i].devices[j].name, "sd%c",
				'a' + dev_no);
			channels[i].devices[j].my_channel = &channels[i];
			channels[i].devices[j].dev_no = dev_no;
			identify_disk(&channels[i].devices[j]);
			partition_scan(&channels[i].devices[j]);
		}
	}
}

void ide_init(void) {
	printk("ide_init: start\n");

	channels_init();
	printk("\nall partition info\n");
	list_traversal(&partition_list, partition_info, NULL);

	printk("ide_init: done\n");
}
