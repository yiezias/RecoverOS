#include "../device/ide.h"
#include "../fs/fs.h"
#include "../lib/kernel/print.h"
#include "../lib/stdio.h"
#include "../lib/user/syscall.h"
#include "../thread/thread.h"
#include "debug.h"
#include "init.h"


void init(void);
void write_file(const char *pathname, size_t file_size, size_t lba);

int main(void) {
	put_str("Kernel Start ...\n");
	init_all();

	/* 写入应用程序，正式发布时应删去 */
	write_file("/shell", 30720, 300);
	write_file("/ls", 15360, 360);
	write_file("/echo", 15360, 390);
	write_file("/cat", 15360, 420);
	write_file("/rm", 15360, 450);
	write_file("/2048", 20480, 480);
	write_file("/snake", 20480, 520);
	/* 写入应用程序结束 */

	thread_unblock(init_proc);

	thread_exit(running_thread(), true);
	return 0;
}

void write_file(const char *pathname, size_t file_size, size_t lba) {
	int fd = sys_open(pathname, O_CREAT | O_RDWR);
	ASSERT(fd != -1);
	void *file_buf = sys_malloc(file_size);
	ASSERT(file_buf);
	ide_read(cur_part->my_disk, lba, file_buf,
		 DIV_ROUND_UP(file_size, SECTOR_SIZE));
	ASSERT((ssize_t)file_size == sys_write(fd, file_buf, file_size));
	sys_close(fd);
	sys_free(file_buf);
}



void init(void) {
	int fd[2] = { -1, -1 };
	pipe(fd);

	pid_t ret_pid = fork();
	if (ret_pid) {
		int status;
		pid_t child_pid;
		while (1) {
			child_pid = wait(&status);
			if (status != 0) {
				printf("\x1b\x0cInit process has recieve a "
				       "child, child_pid is %d, status is %d\n",
				       child_pid, status);
			}
		}
	} else {
		const char *argv[] = { NULL };
		execv("/shell", argv);
	}
	while (1) {}
}
