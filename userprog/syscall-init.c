#include "syscall-init.h"
#include "../device/console.h"
#include "../device/timer.h"
#include "../fs/fs.h"
#include "../lib/string.h"
#include "../lib/user/syscall.h"
#include "../pipe/pipe.h"
#include "exec.h"
#include "fork.h"
#include "wait_exit.h"

#define syscall_nr 32

typedef void *syscall;
syscall syscall_table[syscall_nr];

extern void system_call(void);

pid_t sys_getpid() {
	return running_thread()->pid;
}

// size_t sys_write(char *str) {
//	console_put_str(str);
//	return strlen(str);
// }

void syscall_init() {
	console_put_str("syscall_init: start\n");
	asm volatile("rdmsr" ::"c"(0xc0000082) :);
	asm volatile("wrmsr" ::"a"(system_call),
		     "d"((uint64_t)system_call >> 32)
		     :);
	asm volatile("rdmsr" ::"c"(0xc0000081) :);
	asm volatile("wrmsr" ::"a"(0), "d"(0x00200008) :);

	syscall_table[SYS_GETPID] = sys_getpid;
	syscall_table[SYS_WRITE] = sys_write;
	syscall_table[SYS_MALLOC] = sys_malloc;
	syscall_table[SYS_FREE] = sys_free;
	syscall_table[SYS_FORK] = sys_fork;
	syscall_table[SYS_WAIT] = sys_wait;
	syscall_table[SYS_EXIT] = sys_exit;
	syscall_table[SYS_PS] = sys_ps;
	syscall_table[SYS_OPEN] = sys_open;
	syscall_table[SYS_CLOSE] = sys_close;
	syscall_table[SYS_READ] = sys_read;
	syscall_table[SYS_LSEEK] = sys_lseek;
	syscall_table[SYS_EXECV] = sys_execv;
	syscall_table[SYS_CLEAR] = sys_clear;
	syscall_table[SYS_OPENDIR] = sys_opendir;
	syscall_table[SYS_CLOSEDIR] = sys_closedir;
	syscall_table[SYS_READDIR] = sys_readdir;
	syscall_table[SYS_STAT] = sys_stat;
	syscall_table[SYS_UNLINK] = sys_unlink;
	syscall_table[SYS_PIPE] = sys_pipe;
	syscall_table[SYS_FD_REDIRECT] = sys_fd_redirect;
	syscall_table[SYS_GETTICKS] = sys_getticks;
	syscall_table[SYS_SCHED_YIELD] = thread_yield;

	console_put_str("syscall_init: done\n");
}
