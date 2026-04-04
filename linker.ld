ENTRY(kernel_main)
OUTPUT_FORMAT(binary)

SECTIONS {
    /* Kernel loads at 0x10000 */
    . = 0x10000;

    .text : ALIGN(16) {
        *(.text.entry)    /* Entry point first — bootloader jumps to 0x10000 */
        *(.text)
        *(.text.*)
    }

    /* Multiboot header: must be within the first 8KB of the binary for GRUB,
       but AFTER .text.entry so the bootloader's jmp 0x1000 hits kernel_main. */
    .multiboot : ALIGN(4) {
        *(.multiboot)
    }

    .rodata : ALIGN(16) {
        *(.rodata)
        *(.rodata.*)
    }

    .data : ALIGN(16) {
        *(.data)
        *(.data.*)
    }

    /* Jump BSS past the 1MB mark to avoid the BIOS ROM region (0xC0000-0xFFFFF).
       The backbuffer (~1.9MB in BSS) would otherwise land in ROM addresses where
       writes are silently ignored, causing a garbled noise band on screen. */
    . = 0x100000;

    .bss : ALIGN(4096) {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    }

    . = ALIGN(4096);
    _kernel_end = .;

    /* Discard unnecessary sections */
    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note*)
    }
}
