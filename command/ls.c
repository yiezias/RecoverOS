#include "../fs/dir.h"
#include "../lib/stdio.h"
#include "../lib/string.h"
#include "../lib/user/assert.h"
#include "../lib/user/syscall.h"

char *type_str[] = { "unknown", "regular", "directory" };

static int list_root_dir(bool is_stat) {
	struct dir *pdir = opendir("/");
	assert(pdir);
	struct dir_entry *dir_e;
	while ((dir_e = readdir(pdir)) != NULL) {
		if (is_stat) {
			struct stat buf;
			stat(dir_e->filename, &buf);
			printf("%s:\tinode_no %d\tfile_type %s\t"
			       "file_size %d\n",
			       dir_e->filename, dir_e->i_no,
			       type_str[dir_e->f_type], buf.size);
		} else {
			printf("%s\t", dir_e->filename);
		}
	}
	printf("\n");

	closedir(pdir);
	return 0;
}

int main(int argc, char **argv UNUSED) {
	if (argc == 1) {
		return list_root_dir(false);
	}
	bool argidx = 0;
	struct stat *files = malloc(sizeof(struct stat) * (argc - 1));
	assert(files);
	for (int i = 1; i != argc; ++i) {
		if (!strcmp("-l", argv[i])) {
			argidx = i;
		} else if (stat(argv[i], &files[i - 1])) {
			files[i - 1].inode_no = 4097;
		}
	}
	if (argidx && argc == 2) {
		return list_root_dir(true);
	}

	for (int i = 1; i != argc; ++i) {
		if (i != argidx && files[i - 1].inode_no != 4097) {
			if (argidx) {
				printf("%s:\tinode_no %d\tfile_type %s\t"
				       "file_size %d\n",
				       argv[i], files[i - 1].inode_no,
				       type_str[files[i - 1].file_type],
				       files[i - 1].size);
			} else {
				printf("%s\t", argv[i]);
			}
		}
	}
	printf("\n");
	free(files);
}
