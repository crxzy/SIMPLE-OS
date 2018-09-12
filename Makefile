BUILD_DIR = ./build
ENTRY_POINT = 0xc0001500
AS = nasm
ASFLAGS = -f elf
CC = gcc
CFLAGS = -m32 -c  -fno-builtin -fno-stack-protector -DDEBUG
LD = ld
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main
LIB = -I kernel/ -I . -I lib/
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o  $(BUILD_DIR)/print.o

default: dd


# boot
$(BUILD_DIR)/mbr: boot/mbr.asm
	$(AS) -f bin $^ -o $@ -I boot/
$(BUILD_DIR)/loader: boot/loader.asm
	$(AS) -f bin $^ -o $@ -I boot/

# kernel
$(BUILD_DIR)/main.o: kernel/main.c
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/init.o: kernel/init.c kernel/init.h
	$(CC) $(CFLAGS) $< -o $@ $(LIB)
$(BUILD_DIR)/print.o: lib/kernel/print.asm
	$(AS) -f elf $^ -o $@ -I boot/

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