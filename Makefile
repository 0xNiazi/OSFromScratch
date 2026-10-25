# Toolchain
ASM      := nasm
CC       := gcc
LD       := ld
QEMU     := qemu-system-i386

# Compilation Flags
CFLAGS   := -m32 -masm=intel -ffreestanding -fno-pie -fno-asynchronous-unwind-tables -Wall -Wextra -Iinclude
LDFLAGS  := -m elf_i386 -T linker.ld --oformat binary

# Source & Target Definitions
BOOT_SRC         := boot/boot.asm
BOOT_BIN         := boot.bin

KERNEL_ENTRY_SRC := kernel/kernel_entry.asm
KERNEL_ENTRY_OBJ := kernel/kernel_entry.o

INTERRUPT_SRC    := kernel/interrupt.asm
INTERRUPT_OBJ    := kernel/interrupt.o

# Find all C source files in kernel/ and drivers/
C_SRCS           := $(wildcard kernel/*.c drivers/*.c)
C_OBJS           := $(C_SRCS:.c=.o)

KERNEL_BIN       := kernel.bin
OS_IMAGE         := os-image.bin

.PHONY: all run clean

all: $(OS_IMAGE)

# Assemble bootloader
$(BOOT_BIN): $(BOOT_SRC) boot/gdt.asm
	$(ASM) -f bin $< -o $@

# Assemble kernel entry point and interrupt stubs
$(KERNEL_ENTRY_OBJ): $(KERNEL_ENTRY_SRC)
	$(ASM) -f elf32 $< -o $@

$(INTERRUPT_OBJ): $(INTERRUPT_SRC)
	$(ASM) -f elf32 $< -o $@

# Pattern rule for compiling all C files in kernel/ and drivers/
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link all object files into flat binary
$(KERNEL_BIN): $(KERNEL_ENTRY_OBJ) $(INTERRUPT_OBJ) $(C_OBJS) linker.ld
	$(LD) $(LDFLAGS) $(KERNEL_ENTRY_OBJ) $(INTERRUPT_OBJ) $(C_OBJS) -o $@

# Combine bootloader and kernel into a bootable disk image (padded to 64K)
$(OS_IMAGE): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $@
	truncate -s 64K $@

run: $(OS_IMAGE)
	$(QEMU) -drive format=raw,file=$(OS_IMAGE)

clean:
	rm -f *.bin kernel/*.o drivers/*.o *.elf *.ppm
