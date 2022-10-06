#ifndef __PIPE_PIPE_H
#define __PIPE_PIPE_H
#include "../kernel/global.h"
#include "../lib/stdint.h"

#define PIPE_FLAG 0xffff

bool is_pipe(uint32_t local_fd);
int32_t sys_pipe(int32_t pipefd[2]);
size_t pipe_read(int32_t fd, void *buf, size_t count);
size_t pipe_write(int32_t fd, const void *buf, size_t count);
void sys_fd_redirect(uint32_t old_local_fd, uint32_t new_local_fd);

#endif
