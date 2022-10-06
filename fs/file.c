#include "file.h"
#include "../lib/stdio.h"
#include "../thread/thread.h"
#include "dir.h"

struct file file_table[MAX_FILES_OPEN];

int32_t get_free_slot_in_global(void) {
	for (uint32_t fd_idx = 3; fd_idx != MAX_FILES_OPEN; ++fd_idx) {
		if (file_table[fd_idx].fd_inode == NULL) {
			return fd_idx;
		}
	}
	printk("exceed max open files\n");
	return -1;
}

int32_t pcb_fd_install(int32_t globa_fd_idx) {
	struct task_struct *cur = running_thread();
	for (uint32_t local_fd_idx = 3; local_fd_idx != MAX_FILES_OPEN_PER_PROC;
	     ++local_fd_idx) {
		if (cur->fd_table[local_fd_idx] == -1) {
			cur->fd_table[local_fd_idx] = globa_fd_idx;
			return local_fd_idx;
		}
	}
	printk("exceed max open files per proc\n");
	return -1;
}

int32_t file_create(const char *pathname, enum oflags flag) {
	int32_t fd_idx = get_free_slot_in_global();
	if (fd_idx == -1) {
		return -1;
	}
	struct inode *inode = create_file_or_directory(pathname, FT_REGULAR);
	if (inode == NULL) {
		printk("file_create: create_file_or_directory failed\n");
		return -1;
	}
	file_table[fd_idx].fd_inode = inode;
	file_table[fd_idx].fd_pos = 0;
	file_table[fd_idx].flag = flag;

	return pcb_fd_install(fd_idx);
}

int32_t file_open(const char *pathname, enum oflags flag) {
	struct inode *inode;
	if ((inode = search_file(pathname, false)) == NULL) {
		printk("file_open: no such file or directory");
		return -1;
	}
	int32_t fd_idx = get_free_slot_in_global();
	if (fd_idx == -1) {
		printk("file_open: get_free_slot_in_global failed\n");
		return -1;
	}

	file_table[fd_idx].fd_pos = 0;
	file_table[fd_idx].flag = flag;
	file_table[fd_idx].fd_inode = inode;

	return pcb_fd_install(fd_idx);
}

int file_close(struct file *file) {
	if (file == NULL) {
		return -1;
	}

	inode_close(file->fd_inode);
	file->fd_inode = NULL;
	return 0;
}
