
CFLAGS="-c -fno-stack-protector -fno-builtin \
	-W -Wall -Wmissing-prototypes -Wstrict-prototypes"

compile() {
	gcc $CFLAGS $1 -o command.o
	gcc $CFLAGS -DUSER_DEBUG lib/string.c -o string.o
	gcc $CFLAGS lib/user/assert.c -o assert.o
	as command/start.S -o start.o
	ld command.o start.o build/syscall.o build/stdio.o string.o assert.o -o $2
	strip -s $2
	rm command.o string.o assert.o start.o
	dd if=$2 of=boot.img bs=512 seek=$3 conv=notrunc
}

compile command/ls.c ls 360
compile command/echo.c echo 390
compile command/cat.c cat 420
compile command/rm.c rm 450
compile command/2048.c 2048 480
compile command/snake.c snake 520
