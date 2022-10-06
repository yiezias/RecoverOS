#ifndef __FS_FS_H
#define __FS_FS_H
#include "../kernel/global.h"
#include "../lib/stdint.h"

#define MAX_FILES_PER_PART 4096
#define BITS_PER_SECTOR 4096
#define SECTOR_SIZE 512
#define BLOCK_SIZE SECTOR_SIZE

#define MAX_PATH_LEN 512

enum file_types {
	FT_UNKNOWN,
	FT_REGULAR,
	FT_DIRECTORY,
};

enum oflags {
	O_RDONLY = 0,
	O_WRONLY = 1,
	O_RDWR = 2,
	O_CREAT = 4,
};

enum whence {
	SEEK_SET,
	SEEK_CUR,
	SEEK_END,
};

struct stat {
	size_t inode_no;
	size_t size;
	enum file_types file_type;
};

extern struct partition *cur_part;
void filesys_init(void);
size_t path_depth_cnt(char *pathname);
struct inode *search_file(const char *pathname, bool is_parent);

int32_t sys_open(const char *pathname, enum oflags flag);
ssize_t sys_write(int32_t fd, const char *buf, size_t count);
int32_t fd_local2global(uint32_t local_fd);
int sys_close(int32_t fd);
ssize_t sys_read(int32_t fd, void *buf, size_t count);
ssize_t sys_lseek(int32_t fd, ssize_t offset, enum whence whence);
struct dir *sys_opendir(const char *path);
void sys_closedir(struct dir *pdir);
struct dir_entry *sys_readdir(struct dir *pdir);
int sys_stat(const char *path, struct stat *buf);
int sys_unlink(const char *pathname);
#endif
