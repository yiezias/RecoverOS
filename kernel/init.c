#include "init.h"
#include "../device/console.h"
#include "../device/ide.h"
#include "../device/keyboard.h"
#include "../device/timer.h"
#include "../fs/fs.h"
#include "../lib/kernel/print.h"
#include "../thread/thread.h"
#include "../userprog/syscall-init.h"
#include "intr.h"
#include "memory.h"
#include "tss.h"

void init_all(void) {
	put_str("init_all: start\n");

	mem_init();
	tss_init();
	intr_init();
	timer_init();
	thread_init();
	console_init();
	keyboard_init();
	syscall_init();
	set_intr_stat(intr_on);
	ide_init();
	filesys_init();

	put_str("init_all: done\n");
}
