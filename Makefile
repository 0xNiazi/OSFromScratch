# Toolchain
ASM      := nasm
CC       := gcc
LD       := ld
QEMU     := qemu-system-i386

# Compilation Flags
CFLAGS   := -m32 -ffreestanding -fno-pie -fno-asynchronous-unwind-tables -Wall -Wextra -Iinclude
LDFLAGS  := -m elf_i386 -T linker.ld --oformat binary

# Source & Target Definitions
BOOT_SRC         := boot/boot.asm
BOOT_BIN         := boot.bin

KERNEL_ENTRY_SRC := kernel/kernel_entry.asm
KERNEL_ENTRY_OBJ := kernel/kernel_entry.o

KERNEL_C_SRC     := kernel/kernel.c
KERNEL_C_OBJ     := kernel/kernel.o

KERNEL_BIN       := kernel.bin
OS_IMAGE         := os-image.bin

.PHONY: all run clean

all: $(OS_IMAGE)

# Assemble bootloader
$(BOOT_BIN): $(BOOT_SRC) boot/gdt.asm
	$(ASM) -f bin $< -o $@

# Assemble kernel entry point
$(KERNEL_ENTRY_OBJ): $(KERNEL_ENTRY_SRC)
	$(ASM) -f elf32 $< -o $@

# Compile C kernel
$(KERNEL_C_OBJ): $(KERNEL_C_SRC) include/types.h
	$(CC) $(CFLAGS) -c $< -o $@

# Link kernel into raw binary
$(KERNEL_BIN): $(KERNEL_ENTRY_OBJ) $(KERNEL_C_OBJ) linker.ld
	$(LD) $(LDFLAGS) $(KERNEL_ENTRY_OBJ) $(KERNEL_C_OBJ) -o $@

# Combine bootloader and kernel into a bootable disk image (padded to 64K)
$(OS_IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $@
	truncate -s 64K $@

run: $(OS_IMAGE)
	$(QEMU) -drive format=raw,file=$(OS_IMAGE)

clean:
	rm -f *.bin kernel/*.o *.elf *.ppm
