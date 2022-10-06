#include "../lib/stdio.h"
#include "../lib/string.h"
#include "../lib/user/assert.h"
#include "../lib/user/syscall.h"

int main(int argc, char *argv[]) {
	if (argc == 1) {
		printf("rm: need at least 2 args\n");
		return -1;
	}
	struct stat *files_stat = malloc(sizeof(struct stat) * (argc - 1));
	assert(files_stat);
	for (int i = 0; i != argc - 1; ++i) {
		if (stat(argv[i + 1], &files_stat[i])) {
			printf("rm can't remove %s: "
			       "no such file or "
			       "directory\n",
			       argv[i + 1]);
		}
		//	printf("file %s\n", argv[i + 1]);
		if (unlink(argv[i + 1])) {
			printf("rm can't remove %s\n", argv[i]);
		}
	}
	free(files_stat);
}
