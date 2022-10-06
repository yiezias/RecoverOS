#ifndef __FS_FILE_H
#define __FS_FILE_H
#include "../lib/stdint.h"
#include "fs.h"
#include "inode.h"

struct file {
	size_t fd_pos;
	enum oflags flag;
	struct inode *fd_inode;
};

enum std_fd {
	stdin_no,
	stdout_no,
	stderr_no,
};

#define MAX_FILES_OPEN 32
extern struct file file_table[MAX_FILES_OPEN];

int32_t get_free_slot_in_global(void);
int32_t pcb_fd_install(int32_t globa_fd_idx);
int32_t file_create(const char *pathname, enum oflags flag);
int file_close(struct file *file);
int32_t file_open(const char *pathname, enum oflags flag);
#endif
