# Cross-compiler toolchain
CC = gcc -m32
LD = ld -m elf_i386

# Compiler flags
CFLAGS = -ffreestanding -m32 -O2 -Wall -Wextra \
         -fno-stack-protector -fno-pic -fno-pie \
         -nostdlib -nostdinc

all: NeuroSpark.bin

# Build the final OS image
# Boot (1 sector) + Kernel (up to ~500 sectors) + Data area
# Total: 512KB = 1024 sectors — gives room for kernel growth and 4 save slots
NeuroSpark.bin: boot/boot.bin kernel.bin
	cat boot/boot.bin kernel.bin > $@
	truncate -s 524288 $@

# Assemble bootloader
boot/boot.bin: boot/boot.asm
	nasm -f bin $< -o $@

# Compile and link kernel
# kernel.bin: kernel/kernel.c linker.ld
# 	$(CC) $(CFLAGS) -c kernel/kernel.c -o kernel.o
# 	$(LD) -o $@ -T linker.ld kernel.o --oformat binary

kernel.bin: kernel/kernel.c kernel/disk.c kernel/disk.h kernel/page_cache.c kernel/page_cache.h kernel/partition.c kernel/partition.h kernel/filesystem.c kernel/filesystem.h kernel/ext2fs.c kernel/ext2fs.h kernel/pmm.c kernel/paging.c kernel/paging.asm kernel/interrupt.asm kernel/idt.c kernel/syscall.c kernel/graphics.c kernel/pci.c kernel/ahci.c kernel/ahci.h kernel/scheduler.c kernel/shell.c kernel/input.c kernel/dashboard.c kernel/storage_manager.c kernel/klog.c kernel/vfs.c kernel/net.c kernel/net.h kernel/profiling.c kernel/profiling.h kernel/model_manager.c kernel/model_manager.h kernel/module_loader.c kernel/module_loader.h kernel/posix.c kernel/posix.h kernel/audio.c kernel/audio.h kernel/multiboot.asm kernel/usermode.c kernel/usermode.asm kernel/wm.c kernel/wm.h kernel/font.h linker.ld kernel/launcher.c kernel/launcher.h
	nasm -f elf32 kernel/multiboot.asm -o multiboot.o
	nasm -f elf32 kernel/interrupt.asm -o interrupt.o
	nasm -f elf32 kernel/paging.asm -o paging_asm.o
	nasm -f elf32 kernel/switch.asm -o switch.o
	nasm -f elf32 kernel/usermode.asm -o usermode_asm.o
	$(CC) $(CFLAGS) -c kernel/disk.c -o disk.o
	$(CC) $(CFLAGS) -c kernel/page_cache.c -o page_cache.o
	$(CC) $(CFLAGS) -c kernel/partition.c -o partition.o
	$(CC) $(CFLAGS) -c kernel/filesystem.c -o filesystem.o
	$(CC) $(CFLAGS) -c kernel/ext2fs.c -o ext2fs.o
	$(CC) $(CFLAGS) -c kernel/pmm.c -o pmm.o
	$(CC) $(CFLAGS) -c kernel/paging.c -o paging.o
	$(CC) $(CFLAGS) -c kernel/task.c -o task.o
	$(CC) $(CFLAGS) -c kernel/idt.c -o idt.o
	$(CC) $(CFLAGS) -c kernel/syscall.c -o syscall.o
	$(CC) $(CFLAGS) -c kernel/graphics.c -o graphics.o
	$(CC) $(CFLAGS) -c kernel/pci.c -o pci.o
	$(CC) $(CFLAGS) -c kernel/ahci.c -o ahci.o
	$(CC) $(CFLAGS) -c kernel/scheduler.c -o scheduler.o
	$(CC) $(CFLAGS) -c kernel/shell.c -o shell.o
	$(CC) $(CFLAGS) -c kernel/input.c -o input.o
	$(CC) $(CFLAGS) -c kernel/dashboard.c -o dashboard.o
	$(CC) $(CFLAGS) -c kernel/storage_manager.c -o storage_manager.o
	$(CC) $(CFLAGS) -c kernel/klog.c -o klog.o
	$(CC) $(CFLAGS) -c kernel/vfs.c -o vfs.o
	$(CC) $(CFLAGS) -c kernel/net.c -o net.o
	$(CC) $(CFLAGS) -c kernel/profiling.c -o profiling.o
	$(CC) $(CFLAGS) -c kernel/model_manager.c -o model_manager.o
	$(CC) $(CFLAGS) -c kernel/module_loader.c -o module_loader.o
	$(CC) $(CFLAGS) -c kernel/posix.c -o posix.o
	$(CC) $(CFLAGS) -c kernel/audio.c -o audio.o
	$(CC) $(CFLAGS) -c kernel/usermode.c -o usermode.o
	$(CC) $(CFLAGS) -c kernel/launcher.c -o launcher.o
	$(CC) $(CFLAGS) -c kernel/wm.c -o wm.o
	$(CC) $(CFLAGS) -c kernel/kernel.c -o kernel.o
	$(LD) -o $@ -T linker.ld multiboot.o kernel.o disk.o page_cache.o partition.o filesystem.o ext2fs.o pmm.o paging.o paging_asm.o interrupt.o switch.o usermode_asm.o task.o scheduler.o idt.o syscall.o graphics.o pci.o ahci.o shell.o input.o dashboard.o storage_manager.o klog.o vfs.o net.o profiling.o model_manager.o module_loader.o posix.o audio.o usermode.o launcher.o wm.o --oformat binary

# Clean build artifacts
clean:
	rm -f NeuroSpark.bin kernel.bin kernel.o disk.o page_cache.o partition.o filesystem.o ext2fs.o pmm.o paging.o paging_asm.o interrupt.o switch.o usermode_asm.o task.o scheduler.o idt.o syscall.o graphics.o pci.o ahci.o shell.o input.o dashboard.o storage_manager.o klog.o vfs.o net.o profiling.o model_manager.o module_loader.o posix.o audio.o usermode.o wm.o launcher.o boot/boot.bin

# Run in QEMU
run: NeuroSpark.bin
	qemu-system-i386 -vga std -rtc base=localtime,clock=host -net nic,model=rtl8139 -net user -drive file=$<,format=raw,index=0,media=disk

validate-phase15-17:
	bash tools/validate_phase15_17.sh

validate-phase20:
	bash tools/validate_phase20.sh

.PHONY: all clean run validate-phase15-17 validate-phase20
