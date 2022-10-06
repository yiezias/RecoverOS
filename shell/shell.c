#include "../lib/stdio.h"
#include "../lib/string.h"
#include "../lib/user/assert.h"
#include "../lib/user/syscall.h"

char cwd_cache[MAX_PATH_LEN] = { 0 };

static char cmd_line[MAX_PATH_LEN] = { 0 };

static void print_prompt(void) {
	printf("\x1b\x09[yu@recover %s]$ \x1b\x07", cwd_cache);
}

static void readline(char *buf, size_t count) {
	assert(buf != NULL);
	char *pos = buf;

	while (read(0, pos, 1) != -1 && (size_t)(pos - buf) < count) {
		switch (*pos) {
		case '\n':
		case '\r':
			*pos = 0;
			write(1, "\n", 1);
			return;
		case '\b':
			if (buf[0] != '\b') {
				--pos;
				write(1, "\b", 1);
			}
			break;
		case 'l' - 'a':
			*pos = 0;
			clear();
			print_prompt();
			printf("%s", buf);
			break;
		case 'u' - 'a':
			while (buf != pos) {
				write(1, "\b", 1);
				*(pos--) = 0;
			}
			break;
		default:
			write(1, pos, 1);
			++pos;
		}
	}
	printf("readline: exeed max command len\n");
}

#define MAX_ARG_NR 16

static int32_t cmd_parse(char *cmd_str, char **argv, char token) {
	assert(cmd_str != NULL);
	for (int arg_idx = 0; arg_idx != MAX_ARG_NR; ++arg_idx) {
		argv[arg_idx] = NULL;
	}
	char *next = cmd_str;
	int argc = 0;
	while (*next) {
		while (*next == token) {
			next++;
		}
		if (*next == 0) {
			break;
		}
		argv[argc] = next;
		while (*next && *next != token) {
			++next;
		}
		if (*next) {
			*next++ = 0;
		}
		if (argc > MAX_ARG_NR) {
			return -1;
		}
		++argc;
	}
	return argc;
}

static void help(void) {
	printf("\nthe buildin commands:\n");
	printf("\thelp: print this help message and exit\n");
	printf("\tclear: clear the screen\n");
	printf("\tps: show process information\n");

	printf("\nthe extern commands:\n");
	printf("\techo: print the args\n");
	printf("\tcat: read or write file\n");
	printf("\tls: show the files in root directory\n");
	printf("\trm: remove a regular file\n");
	printf("shortcut key:\n");

	printf("\nctrl+l: clear screen\n");
	printf("\nctrl+u: clear input\n");
}

static void cmd_execute(uint32_t argc UNUSED, const char **argv) {
	if (!strcmp("help", argv[0])) {
		help();
		return;
	} else if (!strcmp("clear", argv[0])) {
		clear();
		return;
	} else if (!strcmp("ps", argv[0])) {
		ps();
		return;
	}
	pid_t pid = fork();
	if (pid) {
		int status;
		pid_t child_pid = wait(&status);
		if (child_pid == -1) {
			panic("cmd_execute: no child\n");
		}
		if (status) {
			printf("\x1b\x0c child_pid %d, it's status: "
			       "%d\n\x1b\x07",
			       child_pid, status);
		}
	} else {
		struct stat file_stat;
		if (stat(argv[0], &file_stat) == -1) {
			printf("cmd_execute: %s: "
			       "no such file or directory\n",
			       argv[0]);
			exit(-1);
		} else {
			execv(argv[0], argv);
		}
	}
}

char *argv[MAX_ARG_NR] = { NULL };

int main(void) {
	clear();
	printf("\n\x1b\x0eWelcome to use the RecoverOs's default shell\n");
	printf("Type 'help' to get help\n\n");
	cwd_cache[0] = '/';
	while (1) {
		print_prompt();
		memset(cmd_line, 0, MAX_PATH_LEN);
		readline(cmd_line, MAX_PATH_LEN);
		if (cmd_line[0] == 0) {
			continue;
		}

		char *pipe_symbol = strchr(cmd_line, '|');
		if (pipe_symbol) {
			int fd[2] = { -1, -1 };
			pipe(fd);
			fd_redirect(1, fd[1]);
			char *each_cmd = cmd_line;
			*pipe_symbol = 0;

			int argc = cmd_parse(each_cmd, argv, ' ');
			cmd_execute(argc, (const char **)argv);
			each_cmd = pipe_symbol + 1;
			fd_redirect(0, fd[0]);

			while ((pipe_symbol = strchr(cmd_line, '|'))) {
				*pipe_symbol = 0;
				argc = -1;
				argc = cmd_parse(each_cmd, argv, ' ');
				fd_redirect(1, fd[1]);
				cmd_execute(argc, (const char **)argv);
				each_cmd = pipe_symbol + 1;
			}
			fd_redirect(1, 1);
			argc = cmd_parse(each_cmd, (char **)argv, ' ');
			cmd_execute(argc, (const char **)argv);
			fd_redirect(0, 0);
			close(fd[0]);
			close(fd[1]);
		} else {
			int argc = cmd_parse(cmd_line, argv, ' ');
			if (argc == -1) {
				printf("shell: num of arguments exeed %d\n",
				       MAX_ARG_NR);
				continue;
			}
			cmd_execute(argc, (const char **)argv);
		}
	}
	panic("shell: should not be here\n");
}
