; Multiboot Header for GRUB/QEMU -kernel support
section .multiboot
align 4
    dd 0x1BADB002              ; Magic number
    dd 0x00000003              ; Flags (page align + memory info)
    dd -(0x1BADB002 + 0x00000003) ; Checksum

    ; Optional fields (if flags set bit 2, which we didn't, but let's keep it simple)
    ; For video mode request (bit 2), we would need more fields here.
    ; Since we handle video in bootloader (if using custom bootloader), we don't strictly need it here.
    ; But if booting via -kernel, the bootloader sets video mode if requested via VBE fields here.
    ; For now, minimal header is enough.
