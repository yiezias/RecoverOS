#ifndef __FS_INODE_H
#define __FS_INODE_H
#include "../device/ide.h"
#include "../lib/kernel/list.h"
#include "../lib/stdint.h"
#include "fs.h"

#define NDIRECT 12
#define MAX_FILE_BLOCKS_CNT (NDIRECT + BLOCK_SIZE / 4)
#define MAX_FILE_SIZE (MAX_FILE_BLOCKS_CNT * BLOCK_SIZE)

struct inode {
	size_t i_no;
	size_t i_size;
	size_t i_open_cnts;
	enum file_types f_type;
	struct lock inode_lock;
	uint32_t i_sectors[NDIRECT + 1];
	struct list_elem inode_tag;
};


void inode_sync(struct partition *part, struct inode *inode);
struct inode *inode_open(struct partition *part, size_t inode_no);
void inode_close(struct inode *inode);
ssize_t inode_read(struct inode *inode, void *buf, size_t count, size_t pos);
int64_t block_bitmap_alloc(struct partition *part);
ssize_t inode_write(struct inode *inode, void *buf, size_t count, size_t pos);
struct inode *inode_alloc(struct partition *part, enum file_types file_type);
struct inode *create_file_or_directory(const char *pathname,
				       enum file_types file_type);
void inode_free(struct partition *part, struct inode *inode);
void block_bitmap_free(struct partition *part, size_t block_lba);
bool delete_file_or_directory(const char *pathname);
#endif
