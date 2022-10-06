#ifndef __DEVICE_IDE_H
#define __DEVICE_IDE_H
#include "../fs/super_block.h"
#include "../lib/kernel/bitmap.h"
#include "../lib/kernel/list.h"
#include "../lib/stdint.h"
#include "../thread/sync.h"

struct partition {
	char name[8];
	uint32_t start_lba;
	uint32_t sec_cnt;
	struct disk *my_disk;
	struct list_elem part_tag;
	struct super_block *sb;
	struct bitmap block_bitmap;
	struct bitmap inode_bitmap;
	struct list open_inodes;
};

struct disk {
	char name[8];
	struct ide_channel *my_channel;
	uint8_t dev_no;
	struct partition prim_parts[4];
	// 不支持逻辑分区
};

struct ide_channel {
	uint16_t port_base;
	uint8_t irq_no;
	struct lock lock;
	bool expecting_intr;
	struct semaphore disk_done;
	struct disk devices[2];
};

extern struct list partition_list;
extern struct ide_channel channels[2];
void ide_init(void);
void intr_hd_hdl(uint8_t irq_no);
void ide_read(struct disk *hd, uint32_t lba, void *buf, uint32_t sec_cnt);
void ide_write(struct disk *hd, uint32_t lba, void *buf, uint32_t sec_cnt);

#endif
