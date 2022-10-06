#include "../lib/stdio.h"
#include "../lib/string.h"
#include "../lib/user/assert.h"
#include "../lib/user/syscall.h"

int main(int argc, char *argv[]) {
	for (int i = 1; i != argc; ++i) {
		printf("%s ", argv[i]);
	}
	printf("\n");
}
