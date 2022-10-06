#include "console.h"
#include "../lib/kernel/print.h"
#include "../thread/sync.h"

static struct lock console_lock;


void console_init() {
	lock_init(&console_lock);
}

void console_acquire() {
	lock_acquire(&console_lock);
}

void console_release() {
	lock_release(&console_lock);
}

void console_put_char(char char_asci) {
	console_acquire();
	put_char(char_asci);
	console_release();
}

void console_put_str(char *str) {
	console_acquire();
	put_str(str);
	console_release();
}

void console_put_num(size_t num) {
	console_acquire();
	put_num(num);
	console_release();
}

void console_put_info(char *message, size_t num) {
	console_acquire();
	put_info(message, num);
	console_release();
}

void sys_clear(void){
	console_acquire();
	cls_screen();
	console_release();
}
