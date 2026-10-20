# Toolchain
ASM      := nasm
CC       := gcc
LD       := ld
QEMU     := qemu-system-i386

# Source and Target definitions
BOOT_SRC := boot/boot.asm
BOOT_BIN := boot.bin

.PHONY: all run clean

all: $(BOOT_BIN)

$(BOOT_BIN): $(BOOT_SRC)
	$(ASM) -f bin $< -o $@

run: $(BOOT_BIN)
	$(QEMU) -drive format=raw,file=$(BOOT_BIN)

clean:
	rm -f *.bin *.o *.elf *.ppm
