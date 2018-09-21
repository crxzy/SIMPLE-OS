BUILD_DIR = ./build
ENTRY_POINT = 0xc0001500
AS = nasm
ASFLAGS = -f elf
CC = gcc
CFLAGS = -m32 -c  -fno-builtin -fno-stack-protector -DDEBUG
LD = ld
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main
LIB = -I lib/ -I lib/kernel/ -I lib/user/ -I kernel/ -I device/ -I thread/ -I userprog/ -I fs/ -I shell/ -I .
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o  $(BUILD_DIR)/print.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/interrupt.o\
	$(BUILD_DIR)/timer.o  $(BUILD_DIR)/list.o  $(BUILD_DIR)/bitmap.o $(BUILD_DIR)/string.o $(BUILD_DIR)/debug.o\
	$(BUILD_DIR)/memory.o $(BUILD_DIR)/thread.o $(BUILD_DIR)/sync.o $(BUILD_DIR)/switch.o $(BUILD_DIR)/tss.o\
	$(BUILD_DIR)/process.o $(BUILD_DIR)/syscall.o $(BUILD_DIR)/syscall-init.o $(BUILD_DIR)/stdio.o $(BUILD_DIR)/stdio-kernel.o\
	$(BUILD_DIR)/console.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/ioqueue.o $(BUILD_DIR)/ide.o $(BUILD_DIR)/fs.o \
    $(BUILD_DIR)/inode.o $(BUILD_DIR)/file.o $(BUILD_DIR)/dir.o\
	$(BUILD_DIR)/fork.o $(BUILD_DIR)/shell.o $(BUILD_DIR)/buildin_cmd.o $(BUILD_DIR)/assert.o $(BUILD_DIR)/exec.o

default: dd


# boot
$(BUILD_DIR)/mbr: boot/mbr.asm
	$(AS) -f bin $^ -o $@ -I boot/
$(BUILD_DIR)/loader: boot/loader.asm
	$(AS) -f bin $^ -o $@ -I boot/

# asm 
$(BUILD_DIR)/print.o: lib/kernel/print.asm
	$(AS) -f elf $^ -o $@ 
$(BUILD_DIR)/kernel.o: kernel/kernel.asm
	$(AS) -f elf $^ -o $@
$(BUILD_DIR)/switch.o: thread/switch.asm
	$(AS) -f elf $^ -o $@
	
# kernel
$(BUILD_DIR)/main.o: kernel/main.c
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/init.o: kernel/init.c kernel/init.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/interrupt.o: kernel/interrupt.c kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/timer.o: device/timer.c device/timer.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/list.o: lib/kernel/list.c lib/kernel/list.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/bitmap.o: lib/kernel/bitmap.c lib/kernel/bitmap.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/string.o: lib/string.c lib/string.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/debug.o: kernel/debug.c kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/memory.o: kernel/memory.c kernel/memory.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/thread.o: thread/thread.c thread/thread.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/sync.o: thread/sync.c thread/sync.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/tss.o: userprog/tss.c userprog/tss.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/process.o: userprog/process.c userprog/process.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/syscall.o: lib/user/syscall.c lib/user/syscall.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/syscall-init.o: userprog/syscall-init.c userprog/syscall-init.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/stdio.o: lib/stdio.c lib/stdio.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/stdio-kernel.o: lib/kernel/stdio-kernel.c lib/kernel/stdio-kernel.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/console.o: device/console.c device/console.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/keyboard.o: device/keyboard.c device/keyboard.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/ioqueue.o: device/ioqueue.c device/ioqueue.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/ide.o: device/ide.c device/ide.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/fs.o: fs/fs.c fs/fs.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/dir.o: fs/dir.c fs/dir.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/inode.o: fs/inode.c fs/inode.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/file.o: fs/file.c fs/file.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/assert.o: lib/user/assert.c lib/user/assert.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/fork.o: userprog/fork.c userprog/fork.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/shell.o: shell/shell.c shell/shell.h 
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/buildin_cmd.o: shell/buildin_cmd.c shell/buildin_cmd.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/exec.o: userprog/exec.c userprog/exec.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)

# img
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

kernel: $(BUILD_DIR)/kernel.bin

boot: $(BUILD_DIR)/mbr $(BUILD_DIR)/loader

dd: boot kernel
	dd if=$(BUILD_DIR)/mbr of=hd80m.img conv=notrunc bs=446 count=1
	dd if=$(BUILD_DIR)/loader of=hd80m.img bs=512 conv=notrunc seek=1
	dd if=$(BUILD_DIR)/kernel.bin of=hd80m.img conv=notrunc seek=9

clean:
	rm -rf $(BUILD_DIR)/*