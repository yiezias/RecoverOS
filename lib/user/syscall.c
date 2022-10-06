#include "syscall.h"

#define _syscall(NUMBER)                    \
	({                                  \
		uint64_t retval;            \
		asm volatile("syscall"      \
			     : "=a"(retval) \
			     : "a"(NUMBER)  \
			     : "memory");   \
		retval;                     \
	})

pid_t getpid() {
	return _syscall(SYS_GETPID);
}

ssize_t write(int32_t fd UNUSED, const char *buf UNUSED, size_t count UNUSED) {
	return _syscall(SYS_WRITE);
}


void *malloc(size_t size UNUSED) {
	return (void *)_syscall(SYS_MALLOC);
}
void free(void *ptr UNUSED) {
	_syscall(SYS_FREE);
}



pid_t fork(void) {
	return (pid_t)_syscall(SYS_FORK);
}

pid_t wait(int *status UNUSED) {
	return (pid_t)_syscall(SYS_WAIT);
}

void exit(int status UNUSED) {
	_syscall(SYS_EXIT);
}

void ps(void) {
	_syscall(SYS_PS);
}

int32_t open(const char *pathname UNUSED, enum oflags flag UNUSED) {
	return _syscall(SYS_OPEN);
}

int close(int32_t fd UNUSED) {
	return _syscall(SYS_CLOSE);
}

ssize_t read(int32_t fd UNUSED, void *buf UNUSED, size_t count UNUSED) {
	return _syscall(SYS_READ);
}

ssize_t lseek(int32_t fd UNUSED, ssize_t offset UNUSED,
	      enum whence whence UNUSED) {
	return (ssize_t)_syscall(SYS_LSEEK);
}

int32_t execv(const char *path UNUSED, const char *argv[] UNUSED) {
	return _syscall(SYS_EXECV);
}

void clear() {
	_syscall(SYS_CLEAR);
}

struct dir *opendir(const char *path UNUSED) {
	return (struct dir *)_syscall(SYS_OPENDIR);
}

void closedir(struct dir *pdir UNUSED) {
	_syscall(SYS_CLOSEDIR);
}

struct dir_entry *readdir(struct dir *pdir UNUSED) {
	return (struct dir_entry *)_syscall(SYS_READDIR);
}

int stat(const char *path UNUSED, struct stat *buf UNUSED) {
	return _syscall(SYS_STAT);
}

int unlink(const char *pathname UNUSED) {
	return _syscall(SYS_UNLINK);
}

int32_t pipe(int32_t pipefd[2] UNUSED) {
	return _syscall(SYS_PIPE);
}

void fd_redirect(uint32_t old_local_fd UNUSED, uint32_t new_local_fd UNUSED) {
	_syscall(SYS_FD_REDIRECT);
}

size_t getticks(void) {
	return _syscall(SYS_GETTICKS);
}

void sched_yield(void) {
	_syscall(SYS_SCHED_YIELD);
}
