ENTRY(kernel_main)
OUTPUT_FORMAT(binary)

SECTIONS {
    /* Kernel loads at 0x1000 */
    . = 0x1000;

    .text : ALIGN(16) {
        *(.text.entry)    /* Entry point first */
        *(.text)
        *(.text.*)
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
