#ifndef __FS_SUPER_BLOCK_H
#define __FS_SUPER_BLOCK_H
#include "../lib/stdint.h"
#define SB_MAGIC 0x474d575a5a46594c

struct super_block {
	uint64_t magic;

	uint32_t sec_cnt;
	uint32_t lba_base;

	size_t inode_cnt;

	uint32_t block_bitmap_lba;
	uint32_t block_bitmap_sects;

	uint32_t inode_bitmap_lba;
	uint32_t inode_bitmap_sects;

	uint32_t inode_table_lba;
	uint32_t inode_table_sects;


	uint32_t data_start_lba;
	size_t root_inode_no;
	size_t dir_entry_size;

	uint8_t pad[444];

} __attribute__((packed));

#endif
