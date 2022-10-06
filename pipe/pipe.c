#include "pipe.h"
#include "../device/ioqueue.h"
#include "../fs/file.h"
#include "../fs/fs.h"
#include "../kernel/memory.h"
#include "../thread/thread.h"

bool is_pipe(uint32_t local_fd) {
	int32_t global_fd = fd_local2global(local_fd);
	return file_table[global_fd].flag == PIPE_FLAG;
}

int32_t sys_pipe(int32_t pipefd[2]) {
	int32_t global_fd = get_free_slot_in_global();
	file_table[global_fd].fd_inode = alloc_pages(&k_v_pool, 1);
	if (file_table[global_fd].fd_inode == NULL) {
		return -1;
	}
	ioqueue_init((struct ioqueue *)file_table[global_fd].fd_inode);
	file_table[global_fd].flag = PIPE_FLAG;

	file_table[global_fd].fd_pos = 2;
	pipefd[0] = pcb_fd_install(global_fd);
	pipefd[1] = pcb_fd_install(global_fd);
	return 0;
}

size_t pipe_read(int32_t fd, void *buf, size_t count) {
	char *buffer = buf;
	size_t bytes_read = 0;
	size_t global_fd = fd_local2global(fd);

	struct ioqueue *ioq = (struct ioqueue *)file_table[global_fd].fd_inode;

	size_t ioq_len = ioq_length(ioq);
	size_t size = ioq_len > count ? count : ioq_len;
	while (bytes_read < size) {
		*buffer = ioq_getchar(ioq);
		++bytes_read;
		++buffer;
	}
	return bytes_read;
}

size_t pipe_write(int32_t fd, const void *buf, size_t count) {
	size_t bytes_write = 0;
	uint32_t global_fd = fd_local2global(fd);
	struct ioqueue *ioq = (struct ioqueue *)file_table[global_fd].fd_inode;

	size_t ioq_left = bufsize - ioq_length(ioq);
	size_t size = ioq_left > count ? count : ioq_left;

	const char *buffer = buf;
	while (bytes_write < size) {
		ioq_putchar(ioq, *buffer);
		++bytes_write;
		++buffer;
	}
	return bytes_write;
}

void sys_fd_redirect(uint32_t old_local_fd, uint32_t new_local_fd) {
	struct task_struct *cur = running_thread();
	if (new_local_fd < 3) {
		cur->fd_table[old_local_fd] = new_local_fd;
	} else {
		uint32_t new_global_fd = cur->fd_table[new_local_fd];
		cur->fd_table[old_local_fd] = new_global_fd;
	}
}
