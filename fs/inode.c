#include "inode.h"
#include "../kernel/debug.h"
#include "../kernel/intr.h"
#include "../lib/stdio.h"
#include "../lib/string.h"
#include "../thread/thread.h"
#include "dir.h"

struct inode_position {
	uint32_t lba;
	size_t off_size;
};

static struct inode_position inode_locate(struct partition *part,
					  size_t inode_no) {
	ASSERT(inode_no < MAX_FILES_PER_PART);
	size_t off_size = inode_no * sizeof(struct inode);
	uint32_t off_sec = off_size / SECTOR_SIZE;
	uint32_t off_size_in_sec = off_size % SECTOR_SIZE;
	struct inode_position inode_pos = { part->sb->inode_table_lba + off_sec,
					    off_size_in_sec };
	return inode_pos;
}

void inode_sync(struct partition *part, struct inode *inode) {
	struct inode_position inode_pos = inode_locate(part, inode->i_no);
	struct inode pure_inode;
	memcpy(&pure_inode, inode, sizeof(struct inode));

	void *io_buf = sys_malloc(SECTOR_SIZE * 2);
	if (io_buf == NULL) {
		PANIC("inode_sync: io_buf==NULL");
	}
	ide_read(part->my_disk, inode_pos.lba, io_buf, 2);
	memcpy((io_buf + inode_pos.off_size), &pure_inode,
	       sizeof(struct inode));
	ide_write(part->my_disk, inode_pos.lba, io_buf, 2);
	sys_free(io_buf);
}

static void inode_init(struct inode *inode, size_t inode_no,
		       enum file_types f_type) {
	inode->i_no = inode_no;
	inode->i_open_cnts = 1;
	inode->f_type = f_type;
	inode->i_size = 0;
	list_push(&cur_part->open_inodes, &inode->inode_tag);
	lock_init(&inode->inode_lock);
	memset(inode->i_sectors, 0, (NDIRECT + 1) * sizeof(uint32_t));
}

static bool find_inode(struct list_elem *elem, void *p_no) {
	return (elem2entry(struct inode, inode_tag, elem))->i_no
	       == *(size_t *)p_no;
}

static void *kernel_alloc(size_t size) {
	struct task_struct *cur = running_thread();
	uint64_t *pml4_bak = cur->pml4;
	cur->pml4 = NULL;
	void *ret = sys_malloc(size);
	cur->pml4 = pml4_bak;
	return ret;
}

struct inode *inode_open(struct partition *part, size_t inode_no) {
	struct list_elem *inode_elem =
		list_traversal(&part->open_inodes, find_inode, &inode_no);
	if (inode_elem != NULL) {
		struct inode *inode_found =
			elem2entry(struct inode, inode_tag, inode_elem);
		++inode_found->i_open_cnts;
		return inode_found;
	}

	struct inode_position inode_pos = inode_locate(part, inode_no);

	struct inode *inode_found = kernel_alloc(sizeof(struct inode));
	ASSERT(inode_found);

	void *inode_buf = sys_malloc(1024);
	ASSERT(inode_buf);
	ide_read(part->my_disk, inode_pos.lba, inode_buf, 2);
	memcpy(inode_found, inode_buf + inode_pos.off_size,
	       sizeof(struct inode));
	sys_free(inode_buf);

	list_push(&part->open_inodes, &inode_found->inode_tag);
	lock_init(&inode_found->inode_lock);
	inode_found->i_open_cnts = 1;
	return inode_found;
}

void inode_close(struct inode *inode) {
	if (inode == root_dir_inode) {
		return;
	}
	enum intr_stat old_stat = set_intr_stat(intr_off);
	if (--inode->i_open_cnts == 0) {
		list_remove(&inode->inode_tag);
		struct task_struct *cur = running_thread();
		uint64_t *pml4_bak = cur->pml4;
		cur->pml4 = NULL;
		sys_free(inode);
		cur->pml4 = pml4_bak;
	}
	set_intr_stat(old_stat);
}

static bool get_blocks_lba(uint32_t *all_blocks, struct inode *inode) {
	for (int block_idx = 0; block_idx != 12; ++block_idx) {
		all_blocks[block_idx] = inode->i_sectors[block_idx];
	}

	if (inode->i_sectors[12] != 0) {
		ide_read(cur_part->my_disk, inode->i_sectors[12],
			 all_blocks + 12, 1);
		return true;
	}
	return false;
}

ssize_t inode_read(struct inode *inode, void *buf, size_t count, size_t pos) {
	ssize_t real_count = count;
	if (pos + count > inode->i_size) {
		real_count = inode->i_size - pos;
		if (real_count <= 0) {
			printk("inode_read: read_pos %d exceed file size\n",
			       real_count);
			return -1;
		}
	}
	size_t block_idx_start = pos / BLOCK_SIZE;
	size_t block_idx_end = DIV_ROUND_UP((pos + real_count), BLOCK_SIZE);
	size_t read_buf_size = (block_idx_end - block_idx_start) * BLOCK_SIZE;
	void *read_buf = sys_malloc(read_buf_size);
	if (read_buf == NULL) {
		printk("inode_read: alloc read_buf failed\n");
		return -1;
	}
	lock_acquire(&inode->inode_lock);
	uint32_t all_blocks[MAX_FILE_BLOCKS_CNT] = { 0 };
	get_blocks_lba(all_blocks, inode);
	for (size_t block_idx = block_idx_start; block_idx != block_idx_end;
	     ++block_idx) {
		uint32_t block_lba = all_blocks[block_idx];
		if (block_lba != 0) {
			ide_read(cur_part->my_disk, block_lba, read_buf, 1);
		} else {
			memset(read_buf, 0, BLOCK_SIZE);
		}
		read_buf += BLOCK_SIZE;
	}
	size_t block_off = pos % BLOCK_SIZE;
	read_buf -= read_buf_size;
	memcpy(buf, read_buf + block_off, real_count);
	sys_free(read_buf);
	lock_release(&inode->inode_lock);
	return real_count;
}

int64_t block_bitmap_alloc(struct partition *part) {
	enum intr_stat old_stat = set_intr_stat(intr_off);
	int64_t bit_idx = bitmap_alloc_a_bit(&part->block_bitmap);
	set_intr_stat(old_stat);
	if (bit_idx == -1) {
		return -1;
	}
	size_t off_sec = bit_idx / BITS_PER_SECTOR;
	uint32_t sec_lba = part->sb->block_bitmap_lba + off_sec;
	void *bitmap_off = part->block_bitmap.bits + off_sec * BLOCK_SIZE;
	ide_write(cur_part->my_disk, sec_lba, bitmap_off, 1);
	return part->sb->data_start_lba + bit_idx;
}

void block_bitmap_free(struct partition *part, size_t block_lba) {
	size_t bit_idx = block_lba - part->sb->data_start_lba;
	enum intr_stat old_stat = set_intr_stat(intr_off);
	bitmap_set(&cur_part->block_bitmap, bit_idx, 0);
	uint8_t buf[BLOCK_SIZE] = { 0 };
	ide_write(part->my_disk, block_lba, buf, 1);
	set_intr_stat(old_stat);

	size_t off_sec = bit_idx / BITS_PER_SECTOR;
	uint32_t sec_lba = part->sb->block_bitmap_lba + off_sec;
	void *bitmap_off = part->block_bitmap.bits + off_sec * BLOCK_SIZE;
	ide_write(cur_part->my_disk, sec_lba, bitmap_off, 1);
}


ssize_t inode_write(struct inode *inode, void *buf, size_t count, size_t pos) {
	if (pos + count > (MAX_FILE_SIZE)) {
		printk("exceed max file size\n");
		return -1;
	}
	void *write_buf = sys_malloc(BLOCK_SIZE * 2);
	if (write_buf == NULL) {
		printk("inode_write: alloc write_buf failed\n");
		return -1;
	}

	uint32_t all_blocks[MAX_FILE_BLOCKS_CNT] = { 0 };
	lock_acquire(&inode->inode_lock);
	if (inode->i_sectors[12] == 0 && pos + count > 12 * BLOCK_SIZE) {
		int32_t indire_block_idx = block_bitmap_alloc(cur_part);
		if (indire_block_idx == -1) {
			printk("inode_write: failed to alloc indire_block");
			lock_release(&inode->inode_lock);
			return -1;
		}
		ide_write(cur_part->my_disk, indire_block_idx, all_blocks, 1);
		inode->i_sectors[12] = indire_block_idx;
	}
	get_blocks_lba(all_blocks, inode);

	size_t block_idx_start = pos / BLOCK_SIZE;
	size_t block_idx_end = DIV_ROUND_UP((pos + count), BLOCK_SIZE);
	size_t block_off = pos % BLOCK_SIZE;
	size_t block_end_off = (pos + count) % BLOCK_SIZE;
	for (size_t block_idx = block_idx_start; block_idx != block_idx_end;
	     ++block_idx) {
		uint32_t block_lba = all_blocks[block_idx];
		if (block_lba == 0) {
			int32_t block = block_bitmap_alloc(cur_part);
			if (block == -1) {
				printk("inode_write: failed to alloc block");
				lock_release(&inode->inode_lock);
				return -1;
			}
			block_lba = all_blocks[block_idx] = block;
		}
		if (block_idx == block_idx_start) {
			ide_read(cur_part->my_disk, block_lba, write_buf, 1);
			bool a_sec = block_idx == block_idx_end - 1;
			size_t cpy_cnt = a_sec ? count : BLOCK_SIZE - block_off;
			memcpy(write_buf + block_off, buf, cpy_cnt);
			ide_write(cur_part->my_disk, block_lba, write_buf, 1);
			buf += cpy_cnt;
			if (a_sec) {
				break;
			}
			continue;
		}
		if (block_idx == block_idx_end - 1) {
			ide_read(cur_part->my_disk, block_lba, write_buf, 1);
			memcpy(write_buf, buf, block_end_off);
			ide_write(cur_part->my_disk, block_lba, write_buf, 1);
			break;
		}
		ide_write(cur_part->my_disk, block_lba, buf, 1);
		buf += BLOCK_SIZE;
	}
	if (pos + count > inode->i_size) {
		inode->i_size = pos + count;
	}
	memcpy(&inode->i_sectors, all_blocks, 12 * 4);
	inode_sync(cur_part, inode);
	if (inode->i_sectors[12]) {
		ide_write(cur_part->my_disk, inode->i_sectors[12],
			  all_blocks + 12, 1);
	}
	sys_free(write_buf);
	lock_release(&inode->inode_lock);
	return count;
}

struct inode *inode_alloc(struct partition *part, enum file_types file_type) {
	enum intr_stat old_stat = set_intr_stat(intr_off);
	ssize_t bit_idx = bitmap_alloc_a_bit(&part->inode_bitmap);
	set_intr_stat(old_stat);
	if (bit_idx == -1) {
		return NULL;
	}
	size_t off_sec = bit_idx / BITS_PER_SECTOR;
	uint32_t sec_lba = part->sb->inode_bitmap_lba + off_sec;
	void *bitmap_off = part->inode_bitmap.bits + off_sec * BLOCK_SIZE;
	ide_write(cur_part->my_disk, sec_lba, bitmap_off, 1);

	struct inode *inode = kernel_alloc(sizeof(struct inode));
	if (inode == NULL) {
		return NULL;
	}

	inode_init(inode, bit_idx, file_type);

	inode_sync(part, inode);

	return inode;
}

void inode_free(struct partition *part, struct inode *inode) {
	enum intr_stat old_stat = set_intr_stat(intr_off);
	printk("i_no: %d\n", inode->i_no);
	bitmap_set(&part->inode_bitmap, inode->i_no, 0);
	uint32_t all_blocks[MAX_FILE_BLOCKS_CNT] = { 0 };
	get_blocks_lba(all_blocks, inode);
	for (int block_idx = 0; block_idx != MAX_FILE_BLOCKS_CNT; ++block_idx) {
		if (all_blocks[block_idx] != 0) {
			block_bitmap_free(part, all_blocks[block_idx]);
		}
	}
	if (inode->i_open_cnts != 1) {
		char error[64];
		sprintf(error, "inode->i_no: %ld,inode->i_open_cnts: %ld\n",
			inode->i_no, inode->i_open_cnts);
		PANIC(error);
	}
	ASSERT(inode->i_open_cnts == 1);
	inode_close(inode);
	set_intr_stat(old_stat);
}

struct inode *create_file_or_directory(const char *pathname,
				       enum file_types file_type) {
	struct inode *inode = inode_alloc(cur_part, file_type);
	if (inode == NULL) {
		printk("create_file_or_directory: inode_alloc failed\n");
		return NULL;
	}
	struct dir_entry dir_e = { "", inode->i_no, file_type };
	char *str = strrchr(pathname, '/');
	strcpy(dir_e.filename, str ? str + 1 : pathname);
	struct inode *pinode = search_file(pathname, true);
	if (pinode == NULL) {
		inode_free(cur_part, inode);
		printk("create_file_or_directory: "
		       "parent directory is not "
		       "exist\n");
		return NULL;
	}
	if (dir_link(pinode, dir_e) == false) {
		inode_free(cur_part, inode);
		inode_close(pinode);
		printk("create_file_or_directory: dir_link failed\n");
		return NULL;
	}
	return inode;
}

bool delete_file_or_directory(const char *pathname) {
	struct inode *pinode = search_file(pathname, true);
	const char *name = strrchr(pathname, '/');
	name = name ? name + 1 : pathname;
	if (pinode == NULL) {
		printk("parent directory isn't exist\n");
		return false;
	}
	size_t off;
	struct inode *inode = dir_look_up(pinode, name, &off);
//	printk("i_no: %d\n", inode->i_no);
//	printk("i_no: %d\n", inode->i_no);
	if (inode == NULL) {
		printk("no such file or directory\n");
		inode_close(pinode);
		return false;
	}
	inode_free(cur_part, inode);
	inode_close(inode);
	if (dir_unlink(pinode, name) == false) {
		PANIC("delete_file_or_directory: dir_unlink failed\n");
	}
	return true;
}
