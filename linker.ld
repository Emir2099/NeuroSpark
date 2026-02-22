ENTRY(kernel_main)
OUTPUT_FORMAT(binary)

SECTIONS {
    /* Kernel loads at 0x1000 */
    . = 0x1000;

    .text : ALIGN(16) {
        *(.text.entry)    /* Entry point first — bootloader jumps to 0x1000 */
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

    .bss : ALIGN(16) {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }

    /* Discard unnecessary sections */
    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note*)
    }
}
