
#这个Makefile有很大改进空间

BUILD_DIR=./build
BOOT_BIN=$(BUILD_DIR)/boot.bin
DISK=boot.img

CC=gcc
LD=ld
SHELL_SIZE=11648
CFLAGS=-c -fno-stack-protector -fno-builtin \
       -W -Wall -Wmissing-prototypes -Wstrict-prototypes \

LDFLAGS=-e main -Ttext 0xffff800000000800 --no-relax

TARGET=$(BUILD_DIR)/kernel.bin

OBJS_NP=main.o print.o string.o init.o memory.o bitmap.o \
	intr.o debug.o list.o tss.o timer.o thread.o sync.o \
	console.o process.o syscall.o syscall-init.o stdio.o \
	stdio-kernel.o fork.o wait_exit.o ide.o fs.o inode.o \
	dir.o file.o exec.o keyboard.o ioqueue.o pipe.o

OBJS=$(foreach n,$(OBJS_NP),$(BUILD_DIR)/$(n))

ASOBJS=$(BUILD_DIR)/knl.o 

SRC_NP=$(OBJS_NP:.o=.c)
SRC=$(foreach n,$(SRC_NP),$(shell find . -name $(n)))

DEPC=$(SRC:.c=.d)


run:$(DISK) bochsrc
	bochs -q
	
%.d:%.c
	$(CC) -MM $< -MT $(subst .c,.o,$(BUILD_DIR)/$(notdir $<)) -o $@
	echo "	$(CC) $(CFLAGS) $< -o $(subst .c,.o,$(BUILD_DIR)/$(notdir $<))" >> $@

include $(DEPC)

$(DISK):$(BUILD_DIR) $(BOOT_BIN) $(TARGET)
	if [[ ! -f $@ ]];then \
	bximage -func=create -hd=10M -imgmode=flat -q boot.img; \
	dd if=$(BOOT_BIN) of=$@ conv=notrunc; \
	sh apart.sh; \
	fi
	dd if=$(TARGET) of=$@ bs=512 seek=3 conv=notrunc
	make -f shell/Makefile clean shell/shell
	sh command/compile.sh

$(BUILD_DIR):
	if [[ ! -d $(BUILD_DIR) ]];then mkdir $(BUILD_DIR);fi

$(BOOT_BIN):boot/boot.asm
	nasm $^ -o $@

$(TARGET):$(OBJS) $(ASOBJS)
	$(LD) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/knl.o:kernel/knl.S
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BOOT_BIN) $(OBJS) $(TARGET) $(ASOBJS) #$(DEPC)
	make -f shell/Makefile clean

.PHONY:run clean
