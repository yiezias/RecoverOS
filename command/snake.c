#include "../lib/stdio.h"
#include "../lib/string.h"
#include "../lib/user/assert.h"
#include "../lib/user/syscall.h"

#define WID 20
#define HEI 20

enum direct {
	LEFT,
	DOWN,
	UP,
	RIGHT,
	QUIT,
};

enum status {
	SPACE,
	BLOCK,
	FOOD,
};

const uint8_t property[] = { 0b01110000, 0b01000100, 0b00100000 };

int score = 0;

static void output(enum status *map, char *buf) {
	clear();
	printf("\x1b\x0aWelcom to the game snake\n"
	       "type 'h','j','k','l' to move the snake "
	       "left,down,up,right. 'q' to quit\n\n");
	for (int i = 0; i != HEI; ++i) {
		for (int i2 = 0; i2 != 1; ++i2) {
			for (int j = 0; j != WID; ++j) {
				sprintf(buf, "\x1b%c",
					property[map[i * HEI + j]]);
				buf += 2;
				for (int j2 = 0; j2 != 2; ++j2) {
					sprintf(buf++, " ");
				}
			}
			sprintf(buf++, "\n");
		}
	}
	sprintf(buf++, "\n");
	sprintf(buf, "\x1b\x07score:\t%d", score);
}

static enum direct input(int fd) {
	static char byte;
	//加static防止输入阻塞，read失败byte值不变
	//在子进程没有其他函数调用时不加static也可以（为什么？）
	//猜：没有其他函数调用时byte所在栈空间不会被覆盖，相当于值不改变
	while (1) {
		read(fd, &byte, 1);
		switch (byte) {
		case 'h':
			return LEFT;
		case 'j':
			return DOWN;
		case 'k':
			return UP;
		case 'l':
			return RIGHT;
		case 'q':
			return QUIT;
		default:
			//用递归会出问题？
			break;
		}
	}
}


uint32_t seed = 1804289383;
static void srand(uint32_t nseed) {
	seed = nseed;
}

static uint32_t rand(void) {
	seed = (seed * 6807) % 2147483647;
	return seed;
}

static int getcnt(enum status *map, enum status val) {
	int valcnt = 0;
	for (int i = 0; i != HEI; ++i) {
		for (int j = 0; j != WID; ++j) {
			if (map[i * HEI + j] == val) {
				++valcnt;
			}
		}
	}
	return valcnt;
}

static int add_food(enum status *map) {
	int space_var = getcnt(map, SPACE);
	if (space_var == 0) {
		return -1;
	}
	srand(getticks());
	int random = rand() % space_var;
	int idx = 0, i = 0;
	for (; i != HEI * WID; ++i) {
		if (map[i] == SPACE) {
			if (idx == random) {
				map[i] = FOOD;
				break;
			}
			++idx;
		}
	}
	return i;
}

static void sleep(size_t ticks) {
	size_t start = getticks();
	while (getticks() - start < ticks) {
		sched_yield();
	}
}

static int turn(int idx, enum direct d) {
	int row = idx / HEI;
	int col = idx % HEI;
	switch (d) {
	case LEFT:
		col = (col + WID - 1) % WID;
		break;
	case DOWN:
		row = (row + 1) % WID;
		break;
	case UP:
		row = (row + HEI - 1) % HEI;
		break;
	case RIGHT:
		col = (col + 1) % WID;
		break;
	default:
		break;
	}
	return row * HEI + col;
}

struct snake_node {
	size_t site;
	struct list_elem elem;
};

void list_init(struct list *list) {
	list->head.prev = NULL;
	list->head.next = &list->tail;
	list->tail.prev = &list->head;
	list->tail.next = NULL;
}

void list_insert(struct list_elem *before, struct list_elem *elem) {
	elem->prev = before->prev;
	elem->next = before;
	elem->prev->next = elem;
	before->prev = elem;
}

void list_push(struct list *plist, struct list_elem *elem) {
	list_insert(plist->head.next, elem);
}

static void snake_init(struct list *snake, enum status *map) {
	struct snake_node *beg = malloc(sizeof(struct snake_node));
	struct snake_node *end = malloc(sizeof(struct snake_node));
	list_init(snake);
	assert(snake && beg && end);
	beg->site = add_food(map);
	end->site = turn(beg->site, RIGHT);
	map[beg->site] = BLOCK;
	map[end->site] = BLOCK;
	list_push(snake, &end->elem);
	list_push(snake, &beg->elem);
}

static int snake_move(struct list *snake, enum status *map, enum direct d) {
	static enum direct pred = RIGHT;
	if ((!(d % 3)) == (!(pred % 3)) && d != pred) {
		d = pred;
	}
	struct snake_node *beg =
		elem2entry(struct snake_node, elem, snake->head.next);
	struct snake_node *end =
		elem2entry(struct snake_node, elem, snake->tail.prev);
	map[end->site] = SPACE;
	for (struct list_elem *idx = snake->tail.prev; idx != snake->head.next;
	     idx = idx->prev) {
		struct snake_node *cur =
			elem2entry(struct snake_node, elem, idx);
		struct snake_node *pre =
			elem2entry(struct snake_node, elem, idx->prev);
		cur->site = pre->site;
	}
	beg->site = turn(beg->site, d);
	if (map[beg->site] == BLOCK) {
		return -1;
	}
	if (map[beg->site] == FOOD) {
		++score;
		map[beg->site] = BLOCK;
		struct snake_node *newbeg = malloc(sizeof(struct snake_node));
		assert(newbeg);
		newbeg->site = turn(beg->site, d);
		list_push(snake, &newbeg->elem);
		map[newbeg->site] = BLOCK;
		add_food(map);
	} else {
		map[beg->site] = BLOCK;
	}
	pred = d;
	return 0;
}

int main(void) {
	int fd[2] = { -1, -1 };
	pipe(fd);
	pid_t ret_pid = fork();
	if (!ret_pid) {
		char byte;
		do {
			read(0, &byte, 1);
			write(fd[1], &byte, 1);
			if (byte == 'q') {
				close(fd[0]);
				close(fd[1]);
				return 0;
			}
		} while (1);
	} else {
		enum status Map[WID * HEI] = { 0 };
		size_t buf_len = WID * HEI * 20;
		char *buf = malloc(buf_len);
		memset(buf, 0, buf_len);
		enum direct in = LEFT;

		struct list *snake = malloc(sizeof(struct list));
		snake_init(snake, Map);
		write(fd[1], "h", 1);
		add_food(Map);
		do {
			if (snake_move(snake, Map, in)) {
				printf("\x1b\x78\n\n\nGame over\n");
				close(fd[0]);
				close(fd[1]);
				return 0;
			}
			output(Map, buf);
			printf("%s\x1b\x07", buf);

			in = input(fd[0]);
			if (in == QUIT) {
				close(fd[0]);
				close(fd[1]);
				free(buf);
				return 0;
			}
			sleep(10000);
		} while (1);
	}
}
