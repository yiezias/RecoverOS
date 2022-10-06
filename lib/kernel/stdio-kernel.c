#include "../../device/console.h"
#include "../stdio.h"
#include "../string.h"

/*与编译器优化相关*/
size_t printk(const char *format, ...) {
	void *args;
	asm volatile("movq %%rbp,%0" : "=g"(args)::"memory");
	args -= 0xb0;
	char buf[1024];
	vsprintf(buf, format, args);
	console_put_str(buf);
	return strlen(buf);
}
