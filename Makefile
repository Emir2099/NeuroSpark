# Cross-compiler toolchain
CC = gcc -m32
LD = ld -m elf_i386
PYTHON3 ?= python3

# Raw disk image size (must be large enough for guest TFS payloads)
DISK_IMAGE_SIZE ?= 67108864

# Optional host file to pre-seed into guest TFS at build time
MODEL_FILE ?= tinyllm.pkg
MODEL_GUEST_NAME ?= tinyllm.pkg

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
	truncate -s $(DISK_IMAGE_SIZE) $@
	@if [ -f "$(MODEL_FILE)" ]; then \
		echo "[pack] injecting $(MODEL_FILE) -> /$(MODEL_GUEST_NAME)"; \
		$(PYTHON3) tools/tfs_inject.py "$@" "$(MODEL_FILE)" "$(MODEL_GUEST_NAME)"; \
	else \
		echo "[pack] skip: $(MODEL_FILE) not found"; \
	fi

# Assemble bootloader
boot/boot.bin: boot/boot.asm
	nasm -f bin $< -o $@

# Compile and link kernel
# kernel.bin: kernel/kernel.c linker.ld
# 	$(CC) $(CFLAGS) -c kernel/kernel.c -o kernel.o
# 	$(LD) -o $@ -T linker.ld kernel.o --oformat binary

kernel.bin: kernel/kernel.c kernel/disk.c kernel/disk.h kernel/page_cache.c kernel/page_cache.h kernel/partition.c kernel/partition.h kernel/filesystem.c kernel/filesystem.h kernel/ext2fs.c kernel/ext2fs.h kernel/pmm.c kernel/paging.c kernel/paging.asm kernel/interrupt.asm kernel/idt.c kernel/syscall.c kernel/graphics.c kernel/pci.c kernel/ahci.c kernel/ahci.h kernel/scheduler.c kernel/shell.c kernel/input.c kernel/dashboard.c kernel/storage_manager.c kernel/klog.c kernel/vfs.c kernel/net.c kernel/net.h kernel/net_socket.c kernel/net_socket.h kernel/remote_auth.c kernel/remote_auth.h kernel/profiling.c kernel/profiling.h kernel/model_manager.c kernel/model_manager.h kernel/ai_runtime.c kernel/ai_runtime.h kernel/ai_scheduler.c kernel/ai_scheduler.h kernel/ai_train.c kernel/ai_train.h kernel/module_loader.c kernel/module_loader.h kernel/posix.c kernel/posix.h kernel/usb_core.c kernel/usb_core.h kernel/usb_xhci.c kernel/usb_xhci.h kernel/usb_hid.c kernel/usb_hid.h kernel/usb_msc.c kernel/usb_msc.h kernel/usb_cdc.c kernel/usb_cdc.h kernel/audio.c kernel/audio.h kernel/audio_ac97.c kernel/audio_ac97.h kernel/multiboot.asm kernel/usermode.c kernel/usermode.asm kernel/wm.c kernel/wm.h kernel/font.h linker.ld kernel/launcher.c kernel/launcher.h kernel/neuromorphic/driver_core.c kernel/pipeline_graph.c
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
	$(CC) $(CFLAGS) -c kernel/net_socket.c -o net_socket.o
	$(CC) $(CFLAGS) -c kernel/remote_auth.c -o remote_auth.o
	$(CC) $(CFLAGS) -c kernel/profiling.c -o profiling.o
	$(CC) $(CFLAGS) -c kernel/model_manager.c -o model_manager.o
	$(CC) $(CFLAGS) -c kernel/ai_runtime.c -o ai_runtime.o
	$(CC) $(CFLAGS) -c kernel/ai_scheduler.c -o ai_scheduler.o
	$(CC) $(CFLAGS) -c kernel/ai_train.c -o ai_train.o
	$(CC) $(CFLAGS) -c kernel/module_loader.c -o module_loader.o
	$(CC) $(CFLAGS) -c kernel/posix.c -o posix.o
	$(CC) $(CFLAGS) -c kernel/usb_core.c -o usb_core.o
	$(CC) $(CFLAGS) -c kernel/usb_xhci.c -o usb_xhci.o
	$(CC) $(CFLAGS) -c kernel/usb_hid.c -o usb_hid.o
	$(CC) $(CFLAGS) -c kernel/usb_msc.c -o usb_msc.o
	$(CC) $(CFLAGS) -c kernel/usb_cdc.c -o usb_cdc.o
	$(CC) $(CFLAGS) -c kernel/audio.c -o audio.o
	$(CC) $(CFLAGS) -c kernel/audio_ac97.c -o audio_ac97.o
	$(CC) $(CFLAGS) -c kernel/usermode.c -o usermode.o
	$(CC) $(CFLAGS) -c kernel/launcher.c -o launcher.o
	$(CC) $(CFLAGS) -c kernel/wm.c -o wm.o
	$(CC) $(CFLAGS) -c kernel/kernel.c -o kernel.o
	$(CC) $(CFLAGS) -c kernel/neuromorphic/driver_core.c -o driver_core.o
	$(CC) $(CFLAGS) -c kernel/pipeline_graph.c -o pipeline_graph.o
	$(LD) -o $@ -T linker.ld multiboot.o kernel.o disk.o page_cache.o partition.o filesystem.o ext2fs.o pmm.o paging.o paging_asm.o interrupt.o switch.o usermode_asm.o task.o scheduler.o idt.o syscall.o graphics.o pci.o ahci.o shell.o input.o dashboard.o storage_manager.o klog.o vfs.o net.o net_socket.o remote_auth.o profiling.o model_manager.o ai_runtime.o ai_scheduler.o ai_train.o module_loader.o posix.o usb_core.o usb_xhci.o usb_hid.o usb_msc.o usb_cdc.o audio.o audio_ac97.o usermode.o launcher.o wm.o driver_core.o pipeline_graph.o --oformat binary

# Clean build artifacts
clean:
	rm -f NeuroSpark.bin kernel.bin kernel.o disk.o page_cache.o partition.o filesystem.o ext2fs.o pmm.o paging.o paging_asm.o interrupt.o switch.o usermode_asm.o task.o scheduler.o idt.o syscall.o graphics.o pci.o ahci.o shell.o input.o dashboard.o storage_manager.o klog.o vfs.o net.o net_socket.o remote_auth.o profiling.o model_manager.o ai_runtime.o ai_scheduler.o ai_train.o module_loader.o posix.o usb_core.o usb_xhci.o usb_hid.o usb_msc.o usb_cdc.o audio.o audio_ac97.o usermode.o wm.o launcher.o driver_core.o pipeline_graph.o boot/boot.bin

# Run in QEMU
run: NeuroSpark.bin
	qemu-system-i386 -vga std -rtc base=localtime,clock=host -net nic,model=rtl8139 -net user -device qemu-xhci,id=xhci -drive file=$<,format=raw,index=0,media=disk

validate-phase15-17:
	bash tools/validate_phase15_17.sh

validate-phase20:
	bash tools/validate_phase20.sh

tfs-inject: NeuroSpark.bin
	$(PYTHON3) tools/tfs_inject.py NeuroSpark.bin $(MODEL_FILE) $(MODEL_GUEST_NAME)

.PHONY: all clean run validate-phase15-17 validate-phase20 tfs-inject
