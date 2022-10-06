#ifndef __LIB_USER_SYSCALL_H
#define __LIB_USER_SYSCALL_H
#include "../../fs/fs.h"
#include "../../thread/thread.h"
#include "../stdint.h"

enum SYSCALL_NR {
	SYS_GETPID,
	SYS_WRITE,
	SYS_MALLOC,
	SYS_FREE,
	SYS_FORK,
	SYS_WAIT,
	SYS_EXIT,
	SYS_PS,

	SYS_OPEN,
	SYS_CLOSE,
	SYS_READ,
	SYS_LSEEK,

	SYS_EXECV,

	SYS_CLEAR,

	SYS_OPENDIR,
	SYS_CLOSEDIR,
	SYS_READDIR,
	SYS_STAT,
	SYS_UNLINK,

	SYS_PIPE,
	SYS_FD_REDIRECT,

	SYS_GETTICKS,
	SYS_SCHED_YIELD,
};

pid_t getpid(void);
ssize_t write(int32_t fd, const char *buf, size_t count);

void *malloc(size_t size);
void free(void *ptr);

pid_t fork(void);

pid_t wait(int *status);
void exit(int status);

void ps(void);

int32_t open(const char *pathname, enum oflags flag);
int close(int32_t fd);
ssize_t read(int32_t fd, void *buf, size_t count);
ssize_t lseek(int32_t fd, ssize_t offset, enum whence whence);
int32_t execv(const char *path, const char *argv[]);

void clear(void);

struct dir *opendir(const char *path);
void closedir(struct dir *pdir);
struct dir_entry *readdir(struct dir *pdir);
int stat(const char *path, struct stat *buf);
int unlink(const char *pathname);
int32_t pipe(int32_t pipefd[2]);
void fd_redirect(uint32_t old_local_fd, uint32_t new_local_fd);
size_t getticks(void);
void sched_yield(void);
#endif
