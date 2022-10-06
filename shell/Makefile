
CC=gcc
CFLAGS=-c -fno-stack-protector -fno-builtin \
       -W -Wall -Wmissing-prototypes -Wstrict-prototypes

shell/shell:shell/shell.o shell/start.o ./build/syscall.o ./build/stdio.o \
	shell/string.o shell/assert.o
	ld $^ -o shell/shell
	strip -s shell/shell
	dd if=shell/shell of=./boot.img bs=512 seek=300 conv=notrunc

shell/start.o:shell/start.S
	$(CC) $(CFLAGS) $< -o $@

shell/shell.o:shell/shell.c
	$(CC) $(CFLAGS) $< -o $@

shell/string.o:./lib/string.c
	$(CC) $(CFLAGS) -DUSER_DEBUG $< -o $@

shell/assert.o:./lib/user/assert.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm shell/*.o -rf
