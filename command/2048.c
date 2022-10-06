#include "../lib/stdio.h"
#include "../lib/string.h"
#include "../lib/user/assert.h"
#include "../lib/user/syscall.h"

#define ROW 4
#define COL 4

typedef int Grid[ROW * COL];

enum dire {
	LEFT,
	DOWN,
	UP,
	RIGHT,
	QUIT,
};

static int getcnt(Grid grid, int val) {
	int valcnt = 0;
	for (int i = 0; i != ROW; ++i) {
		for (int j = 0; j != COL; ++j) {
			if (grid[i * ROW + j] == val) {
				++valcnt;
			}
		}
	}
	return valcnt;
}

static enum dire input(void) {
	char byte;
	read(0, &byte, 1);

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
		return input();
	}
}

static int num2idx(int num) {
	int idx = 0, n = 1;
	while (n != num) {
		++idx;
		n *= 2;
	}
	return idx;
}

uint8_t property[] = { 0b01111000, 0b01001101, 0b00101101, 0b00011110,
		       0b01101100, 0b00111100, 0b01011010, 0b01101000,
		       0b00100100, 0b00011100, 0b01100100, 0b00111010 };

static void output(Grid grid, char *buf) {
	clear();
	printf("\x1b\x0aWelcome to the game 2048\ntype 'h', 'j', 'k' 'l'"
	       " to turn left, down, up, right, 'q' to quit.\n"
	       "Until a block's num reaches 2048\n\n");
	for (int i = 0; i != ROW; ++i) {
		for (int i2 = 0; i2 != 3; ++i2) {
			for (int j = 0; j != COL; ++j) {
				sprintf(buf++, "\x1b");
				sprintf(buf++, "%c",
					property[num2idx(grid[i * ROW + j])]);
				sprintf(buf, "      ");
				if (i2 == 1 && grid[i * ROW + j] != 1) {
					sprintf(buf + 1, "%d",
						grid[i * ROW + j]);
				}
				for (int idx = 0; idx != 6; ++idx) {
					if (buf[idx] == 0) {
						buf[idx] = ' ';
					}
				}
				buf += 6;
			}
			sprintf(buf++, "\n");
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

static int badd(Grid grid) {
	int space_var = getcnt(grid, 1);
	if (space_var == 0) {
		return -1;
	}
	srand(getticks());
	int random = rand() % space_var;
	int idx = 0;
	for (int i = 0; i != ROW * COL; ++i) {
		if (grid[i] == 1) {
			if (idx == random) {
				grid[i] = 2 * (rand() % 2 + 1);
				break;
			}
			++idx;
		}
	}
	return 0;
}

static void rmv(int *arr, int idx, int len) {
	do {
		arr[idx] = arr[idx + 1];
	} while (++idx != len);
	arr[len - 1] = 1;
}

static void mvleft(int *arr, int col, int row) {
	int *aend = arr + col * row;
	while (arr != aend) {
		for (int i = col - 1; i >= 0; --i) {
			if (arr[i] == 1) {
				rmv(arr, i, col);
			}
		}
		for (int i = 0; i != col - 1; ++i) {
			if (arr[i] == arr[i + 1] && arr[i] != 1) {
				arr[i] *= 2;
				rmv(arr, i + 1, col);
			}
		}
		arr += col;
	}
}


static void trans(int *dst, int *src, int row, int col) {
	for (int i = 0; i != col; ++i) {
		for (int j = 0; j != row; ++j) {
			dst[i * col + j] = src[j * row + i];
		}
	}
}

static void tover(int *dst, int *src, int row, int col) {
	for (int i = 0; i != row; ++i) {
		for (int j = 0; j != col; ++j) {
			dst[i * row + j] = src[i * row + col - 1 - j];
		}
	}
}

static void pleft(Grid grid) {
	mvleft((int *)grid, COL, ROW);
}


static void pup(Grid grid) {
	int gtmp[COL][ROW];
	trans((int *)gtmp, (int *)grid, ROW, COL);
	mvleft((int *)gtmp, ROW, COL);

	trans((int *)grid, (int *)gtmp, COL, ROW);
}

static void pright(Grid grid) {
	int gtmp[ROW][COL];
	tover((int *)gtmp, (int *)grid, ROW, COL);
	mvleft((int *)gtmp, COL, ROW);

	tover((int *)grid, (int *)gtmp, ROW, COL);
}

static void pdown(Grid grid) {
	int gtmp1[COL][ROW];
	int gtmp2[COL][ROW];
	trans((int *)gtmp1, (int *)grid, ROW, COL);
	tover((int *)gtmp2, (int *)gtmp1, COL, ROW);
	mvleft((int *)gtmp2, ROW, COL);

	tover((int *)gtmp1, (int *)gtmp2, COL, ROW);
	trans((int *)grid, (int *)gtmp1, COL, ROW);
}

void (*func[4])(Grid grid) = { pleft, pdown, pup, pright };


int main(void) {
	Grid grid = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
	char buf[COL * ROW * 7 * 6] = { 0 };
	badd(grid);
	while (1) {
		if (badd(grid)) {
			printf("\x1b\x78Game over\n");
			return 0;
		}
		output(grid, buf);
		printf("%s\x1b\x07", buf);
		if (getcnt(grid, 2048)) {
			printf("\x1b\x4eYou win!!!\n");
			return 0;
		}

		enum dire dire = input();
		if (dire == QUIT) {
			return 0;
		}
		func[dire](grid);
	}
}
