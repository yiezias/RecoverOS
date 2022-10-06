
#include "../lib/stdio.h"
#include "../lib/string.h"
#include "../lib/user/assert.h"
#include "../lib/user/syscall.h"

static int read_file(const char *file_name) {
	struct stat file_stat;
	if (stat(file_name, &file_stat) == -1) {
		printf("no such file or directory\n");
		return -1;
	}
	int fd = open(file_name, O_RDONLY);
	if (fd == -1) {
		printf("read_file: open failed\n");
		return -1;
	}
	char *buf = malloc(file_stat.size);
	read(fd, buf, file_stat.size);
	printf("%s", buf);
	free(buf);
	close(fd);
	return 0;
}

static int write_file(const char *pathname, int argc, char **argv) {
	struct stat file_stat;
	int ret = stat(pathname, &file_stat);
	enum oflags flag = ret == -1 ? O_CREAT | O_WRONLY : O_WRONLY;
	int fd = open(pathname, flag);
	if (fd == -1) {
		return -1;
	}
	while (argc--) {
		size_t len = strlen(*argv);
		void *buf = malloc(len + 1);
		sprintf(buf, "%s\n", *argv);
		write(fd, buf, len + 1);
		free(buf);
		++argv;
	}
	close(fd);
	return 0;
}

static int input_file(const char *pathname, const char *end) {
	struct stat file_stat;
	int ret = stat(pathname, &file_stat);
	enum oflags flag = ret == -1 ? O_CREAT | O_WRONLY : O_WRONLY;
	int fd = open(pathname, flag);
	if (fd == -1) {
		return -1;
	}

	char str_buf[512];
	do {
		int pos = 0;
		char buf;
		do {
			read(0, &buf, 1);
			write(1, &buf, 1);
			str_buf[pos++] = buf;
		} while (buf != '\r');
		printf("\n");
		str_buf[pos - 1] = 0;
		if (!strcmp(str_buf, end)) {
			break;
		}
		memcpy(str_buf + pos - 1, "\n", 2);
		write(fd, str_buf, strlen(str_buf));
	} while (1);

	close(fd);
	return 0;
}

static int input(void) {
	char byte;
	do {
		read(0, &byte, 1);
		byte = byte == '\r' ? '\n' : byte;
		write(1, &byte, 1);
	} while (byte != '\n');
	return 0;
}

int main(int argc, char **argv) {
	switch (argc) {
	case 1:
		return input();
	case 2:
		return read_file(argv[1]);
	case 3:
		return input_file(argv[1], argv[2]);
	default:
		return write_file(argv[1], argc - 2, argv + 2);
	}
}
