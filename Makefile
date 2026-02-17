# Cross-compiler toolchain
CC = i686-elf-gcc
LD = i686-elf-ld

# Compiler flags
CFLAGS = -ffreestanding -m32 -O2 -Wall -Wextra \
         -fno-stack-protector -fno-pic -fno-pie \
         -nostdlib -nostdinc

all: NeuroSpark.bin

# Build the final OS image
# Boot (1 sector) + Kernel (up to 32 sectors) + Data area (sectors 66+)
# Total: 128KB = 256 sectors — gives room for kernel growth and 4 save slots
NeuroSpark.bin: boot/boot.bin kernel.bin
	cat boot/boot.bin kernel.bin > $@
	truncate -s 131072 $@

# Assemble bootloader
boot/boot.bin: boot/boot.asm
	nasm -f bin $< -o $@

# Compile and link kernel
# kernel.bin: kernel/kernel.c linker.ld
# 	$(CC) $(CFLAGS) -c kernel/kernel.c -o kernel.o
# 	$(LD) -o $@ -T linker.ld kernel.o --oformat binary

kernel.bin: kernel/kernel.c kernel/disk.c kernel/disk.h kernel/pmm.c kernel/interrupt.asm linker.ld
	nasm -f elf32 kernel/interrupt.asm -o interrupt.o
	$(CC) $(CFLAGS) -c kernel/disk.c -o disk.o
	$(CC) $(CFLAGS) -c kernel/pmm.c -o pmm.o
	$(CC) $(CFLAGS) -c kernel/kernel.c -o kernel.o
	$(LD) -o $@ -T linker.ld kernel.o disk.o pmm.o interrupt.o --oformat binary

# Clean build artifacts
clean:
	rm -f NeuroSpark.bin kernel.bin kernel.o disk.o pmm.o interrupt.o boot/boot.bin

# Run in QEMU
run: NeuroSpark.bin
	qemu-system-i386 -drive file=$<,format=raw,index=0,media=disk

.PHONY: all clean run
