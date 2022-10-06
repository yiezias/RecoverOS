#include "../lib/stdio.h"
#include "../lib/user/assert.h"
#include "../lib/user/syscall.h"

int main(void) {
	while (1) {
		char buf[2] = { 0, '\n' };
		read(0, buf, 1);
		write(1, buf, 1);
	}
	pid_t ret = fork();
	if (ret) {
		int status;
		assert(-1 != wait(&status));
	} else {
		ps();
		exit(0);
	}
	//	while (1) {}
}
