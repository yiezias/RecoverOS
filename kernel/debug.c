
#include "debug.h"
#include "../lib/kernel/print.h"
#include "intr.h"

void panic_spin(const char *file, int line, const char *func,
		const char *condition) {
	set_intr_stat(intr_off);
	put_str("\x1b\x0c\n\n!!!!!!!!   SOMETING WENT WRONG (ToT)   !!!!!!!!\n");
	put_str("file: \"");
	put_str((char *)file);
	put_str("\"\nline: 0x");
	put_num(line);
	put_str("\nfunction: \"");
	put_str((char *)func);
	put_str("\"\ncondition: \"");
	put_str((char *)condition);
	put_str("\"\n");
	while (1) {}
}
