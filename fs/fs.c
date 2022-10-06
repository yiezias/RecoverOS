#include "fs.h"
#include "../device/console.h"
#include "../device/ide.h"
#include "../device/ioqueue.h"
#include "../device/keyboard.h"
#include "../kernel/debug.h"
#include "../kernel/memory.h"
#include "../lib/stdio.h"
#include "../lib/string.h"
#include "../pipe/pipe.h"
#include "../thread/thread.h"
#include "dir.h"
#include "file.h"
#include "inode.h"
#include "super_block.h"

struct partition *cur_part;

static const char *path_parse(const char *pathname, char *name_store) {
	if (pathname[0] == '/') {
		while (*(++pathname) == '/') {}
	}
	while (*pathname != '/' && *pathname != 0) {
		*name_store++ = *pathname++;
	}
	*name_store = 0;
	if (pathname[0] == 0) {
		return NULL;
	}

	return pathname;
}

/*
size_t path_depth_cnt(char *pathname) {
	ASSERT(pathname != NULL);
	char *p = pathname;
	char name[MAX_FILE_NAME_LEN];

	size_t depth = 0;
	p = path_parse(p, name);
	while (name[0]) {
		++depth;
		memset(name, 0, MAX_FILE_NAME_LEN);
		if (p) {
			p = path_parse(p, name);
		}
	}
	return depth;
}
*/

struct inode *search_file(const char *pathname, bool is_parent) {
	struct inode *pinode = inode_open(
		cur_part, pathname[0] == '/' ? 0 : running_thread()->cwd_i_no);
	char name[MAX_FILE_NAME_LEN] = { 0 };
	const char *path_remain = pathname;
	size_t off;
	struct inode *cinode;
	while ((path_remain = path_parse(path_remain, name)) != NULL) {
		if ((cinode = dir_look_up(pinode, name, &off)) == NULL) {
			printk("no such file or directory\n");
			inode_close(pinode);
			return NULL;
		}
		if (cinode->f_type != FT_DIRECTORY) {
			printk("mid dir isn't a directory\n");
			inode_close(pinode);
			inode_close(cinode);
			return NULL;
		}
		inode_close(pinode);
		pinode = cinode;
	}
	// name为空必为起始目录
	if (is_parent || name[0] == 0) {
		return pinode;
	}
	if ((cinode = dir_look_up(pinode, name, &off)) == NULL) {
		printk("search_file: file search is not exist\n");
		inode_close(pinode);
		return NULL;
	}
	inode_close(pinode);
	return cinode;
}



int32_t sys_open(const char *pathname, enum oflags flag) {
	int32_t fd = -1;
	switch (flag & O_CREAT) {
	case O_CREAT:
		fd = file_create(pathname, flag);
		break;
	default:
		fd = file_open(pathname, flag);
	}
	return fd;
}

int32_t fd_local2global(uint32_t local_fd) {
	struct task_struct *cur = running_thread();
	int32_t global_fd = cur->fd_table[local_fd];
	ASSERT(global_fd >= 0 && global_fd < MAX_FILES_OPEN);
	return global_fd;
}

int sys_close(int32_t fd) {
	int ret = -1;
	if (fd > 2) {
		uint32_t fd_idx = fd_local2global(fd);
		if (is_pipe(fd)) {
			if (--file_table[fd_idx].fd_pos == 0) {
				free_pages(&k_v_pool,
					   file_table[fd_idx].fd_inode, 1);
				file_table[fd_idx].fd_inode = NULL;
				ret = 0;
			}
		} else {
			ret = file_close(&file_table[fd_idx]);
		}
		running_thread()->fd_table[fd] = -1;
	} else {
		PANIC("sys_close: can't close std fd\n");
	}
	return ret;
}

ssize_t sys_write(int32_t fd, const char *buf, size_t count) {
	if (fd < 0) {
		printk("sys_write: fd error\n");
		return -1;
	}

	if (is_pipe(fd)) {
		return pipe_write(fd, buf, count);
	}

	if (fd == stdout_no) {
		char *write_buf = sys_malloc(count + 1);
		if (write_buf == NULL) {
			PANIC("alloc write_buf failed\n");
		}
		memcpy(write_buf, buf, count);
		write_buf[count] = 0;
		console_put_str(write_buf);
		sys_free(write_buf);
		return count;
	} else {
		int32_t fd_idx = fd_local2global(fd);
		if (file_table[fd_idx].flag & O_WRONLY
		    || file_table[fd_idx].flag & O_RDWR) {
			size_t bytes_written = inode_write(
				file_table[fd_idx].fd_inode, (void *)buf, count,
				file_table[fd_idx].fd_pos);
			file_table[fd_idx].fd_pos += bytes_written;
			return bytes_written;
		} else {
			printk("sys_write: not allowed to write\n");
			return -1;
		}
	}
}

ssize_t sys_read(int32_t fd, void *buf, size_t count) {
	ASSERT(buf != NULL);
	ssize_t ret = -1;
	if (fd < 0 || fd == stdout_no || fd == stderr_no) {
		PANIC("sys_read: fd error\n");
	} else if (is_pipe(fd)) {
		ret = pipe_read(fd, buf, count);
	} else if (fd == stdin_no) {
		char *buffer = buf;
		size_t bytes_read = 0;
		while (bytes_read < count) {
			*buffer = ioq_getchar(&kbd_buf);
			bytes_read++;
			buffer++;
		}
		ret = (bytes_read == 0 ? -1 : (ssize_t)bytes_read);
	} else {
		size_t fd_idx = fd_local2global(fd);
		ret = inode_read(file_table[fd_idx].fd_inode, buf, count,
				 file_table[fd_idx].fd_pos);
		file_table[fd_idx].fd_pos += ret;
	}
	return ret;
}

ssize_t sys_lseek(int32_t fd, ssize_t offset, enum whence whence) {
	ssize_t new_pos = 0;
	if (fd < 0 || fd > MAX_FILES_OPEN) {
		printk("sys_lseek: fd error\n");
		return -1;
	}
	uint32_t fd_idx = fd_local2global(fd);
	struct file *pf = &file_table[fd_idx];
	ssize_t file_size = pf->fd_inode->i_size;
	switch (whence) {
	case SEEK_SET:
		new_pos = offset;
		break;
	case SEEK_CUR:
		new_pos = pf->fd_pos + offset;
		break;
	case SEEK_END:
		new_pos = file_size + offset;
		break;
	}
	if (new_pos < 0 || new_pos > file_size) {
		printk("sys_lseek: pos exceed file size\n");
		return -1;
	}
	pf->fd_pos = new_pos;
	return pf->fd_pos;
}

struct dir *sys_opendir(const char *path) {
	struct inode *inode = search_file(path, false);
	if (inode == NULL) {
		printk("sys_opendir:%s no such file or directory\n", path);
		return NULL;
	}
	if (inode->f_type != FT_DIRECTORY) {
		printk("sys_opendir: path is not a directory");
		return NULL;
	};
	struct dir *pdir = (struct dir *)sys_malloc(sizeof(struct dir));
	pdir->inode = inode;
	pdir->dir_pos = 0;
	return pdir;
}

void sys_closedir(struct dir *pdir) {
	if (pdir->inode == root_dir_inode) {
		return;
	}
	inode_close(pdir->inode);
	sys_free(pdir);
}

struct dir_entry *sys_readdir(struct dir *pdir) {
	do {
		pdir->dir_pos += sizeof(struct dir_entry);
		if (pdir->dir_pos > pdir->inode->i_size) {
			return NULL;
		}
		inode_read(pdir->inode, &pdir->dir_buf,
			   sizeof(struct dir_entry),
			   pdir->dir_pos - sizeof(struct dir_entry));
	} while (pdir->dir_buf.f_type == FT_UNKNOWN);
	return &pdir->dir_buf;
}

int sys_stat(const char *path, struct stat *buf) {
	struct inode *inode = search_file(path, false);
	if (inode == NULL) {
		return -1;
	}
	buf->file_type = inode->f_type;
	buf->size = inode->i_size;
	buf->inode_no = inode->i_no;
	ASSERT(inode->i_open_cnts >= 1);
	inode_close(inode);
	return 0;
}

int sys_unlink(const char *pathname) {
	struct inode *inode = search_file(pathname, false);
	if (inode == NULL) {
		printk("sys_unlink: no such file or directory\n");
		return -1;
	}
	for (size_t i = 0; i != MAX_FILES_OPEN; ++i) {
		if (file_table[i].fd_inode == inode) {
			file_table[i].fd_inode = NULL;
		}
	}
	inode_close(inode);
	if (delete_file_or_directory(pathname)) {
		return 0;
	} else {
		printf("sys_unlink: delete_file_or_directory failed\n");
		return -1;
	}
}



static void sb_init(struct super_block *sb, uint32_t sec_cnt,
		    uint32_t start_lba) {
	sb->inode_bitmap_sects =
		DIV_ROUND_UP(MAX_FILES_PER_PART, BITS_PER_SECTOR);
	sb->inode_table_sects = DIV_ROUND_UP(
		(MAX_FILES_PER_PART * sizeof(struct inode)), SECTOR_SIZE);
	sb->block_bitmap_sects = DIV_ROUND_UP(sec_cnt, BITS_PER_SECTOR);

	//	sb->magic = SB_MAGIC;
	//	保证每次运行都格式化硬盘，正式发布时应删去注释
	sb->sec_cnt = sec_cnt;
	sb->inode_cnt = MAX_FILES_PER_PART;
	sb->lba_base = start_lba;

	sb->block_bitmap_lba = sb->lba_base + 2;
	sb->inode_bitmap_lba = sb->block_bitmap_lba + sb->block_bitmap_sects;
	sb->inode_table_lba = sb->inode_bitmap_lba + sb->inode_bitmap_sects;

	sb->data_start_lba = sb->inode_table_lba + sb->inode_table_sects;
	sb->root_inode_no = 0;
	sb->dir_entry_size = sizeof(struct dir_entry);
	printk("block_bitmap\n\tlba: 0x%x\tsects: 0x%x\n", sb->block_bitmap_lba,
	       sb->block_bitmap_sects);
	printk("inode_bitmap\n\tlba: 0x%x\tsects: 0x%x\n", sb->inode_bitmap_lba,
	       sb->inode_bitmap_sects);
	printk("inode_table\n\tlba: 0x%x\tsects: 0x%x\n", sb->inode_table_lba,
	       sb->inode_table_sects);
	printk("data_start_lba:\t%x\n", sb->data_start_lba);
}

static void partition_format(struct partition *part) {
	printk("Format part %s\n", part->name);
	struct super_block sb;
	sb_init(&sb, part->sec_cnt, part->start_lba);
	ide_write(part->my_disk, part->start_lba + 1, &sb, 1);
	// i节点表明显是最大的
	size_t buf_size = sb.inode_table_sects * SECTOR_SIZE;
	uint8_t *buf = sys_malloc(buf_size);

	struct bitmap btmp = { sb.block_bitmap_sects * SECTOR_SIZE, buf };
	memset(buf, 0xff, btmp.bytes_len);
	bitmap_init(&btmp);
	bitmap_alloc(&btmp, 1);
	ide_write(part->my_disk, sb.block_bitmap_lba, buf,
		  sb.block_bitmap_sects);

	btmp.bytes_len = sb.inode_bitmap_sects * SECTOR_SIZE;
	memset(buf, 0xff, btmp.bytes_len);
	bitmap_init(&btmp);
	bitmap_set(&btmp, 0, 1);
	ide_write(part->my_disk, sb.inode_bitmap_lba, buf,
		  sb.inode_bitmap_sects);

	memset(buf, 0, buf_size);
	struct inode *i = (struct inode *)buf;
	i->i_no = 0;
	i->i_size = sb.dir_entry_size * 2;
	i->f_type = FT_DIRECTORY;
	i->i_sectors[0] = sb.data_start_lba;
	ide_write(part->my_disk, sb.inode_table_lba, buf, sb.inode_table_sects);

	struct dir_entry *p_de = (struct dir_entry *)buf;
	strcpy(p_de->filename, ".");
	p_de->i_no = 0;
	p_de->f_type = FT_DIRECTORY;
	++p_de;
	strcpy(p_de->filename, "..");
	p_de->i_no = 0;
	p_de->f_type = FT_DIRECTORY;
	ide_write(part->my_disk, sb.data_start_lba, buf, 1);
	sys_free(buf);
}

static void mount_partition(struct partition *part) {
	printk("mount %s start\n", part->name);
	struct super_block *sb_buf = sys_malloc(SECTOR_SIZE);
	ASSERT(sb_buf);
	size_t sb_size = sizeof(struct super_block) - 444;
	part->sb = (struct super_block *)sys_malloc(sb_size);
	ASSERT(part->sb);
	ide_read(part->my_disk, part->start_lba + 1, sb_buf, 1);
	memcpy(part->sb, sb_buf, sb_size);
	sys_free(sb_buf);

	part->block_bitmap.bits =
		sys_malloc(part->sb->block_bitmap_sects * SECTOR_SIZE);
	ASSERT(part->block_bitmap.bits);
	part->block_bitmap.bytes_len =
		part->sb->block_bitmap_sects * SECTOR_SIZE;
	ide_read(part->my_disk, part->sb->block_bitmap_lba,
		 part->block_bitmap.bits, part->sb->block_bitmap_sects);

	part->inode_bitmap.bits =
		sys_malloc(part->sb->inode_bitmap_sects * SECTOR_SIZE);
	ASSERT(part->inode_bitmap.bits);
	part->inode_bitmap.bytes_len =
		part->sb->inode_bitmap_sects * SECTOR_SIZE;
	ide_read(part->my_disk, part->sb->inode_bitmap_lba,
		 part->inode_bitmap.bits, part->sb->inode_bitmap_sects);
	list_init(&part->open_inodes);
	printk("mount %s done\n", part->name);
}

static bool find_fs(struct list_elem *elem, void *buf) {
	struct partition *part = elem2entry(struct partition, part_tag, elem);
	ide_read(part->my_disk, part->start_lba + 1, buf, 1);
	struct super_block *sb = buf;
	if (sb->magic == SB_MAGIC) {
		return true;
	}
	return false;
}

void filesys_init() {
	printk("filesys_init: start\n");

	struct super_block sb;
	struct list_elem *part_elem =
		list_traversal(&partition_list, find_fs, &sb);
	if (part_elem == NULL) {
		cur_part = elem2entry(struct partition, part_tag,
				      partition_list.head.next);
		partition_format(cur_part);
	} else {
		cur_part = elem2entry(struct partition, part_tag, part_elem);
	}

	mount_partition(cur_part);

	root_dir_inode = inode_open(cur_part, 0);

	for (size_t fd_idx = 0; fd_idx != MAX_FILES_OPEN; ++fd_idx) {
		file_table[fd_idx].fd_inode = NULL;
	}

	printk("filesys_init: done\n");
}
