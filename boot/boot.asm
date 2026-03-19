[org 0x7c00]
[bits 16]

KERNEL_OFFSET equ 0x1000

; Entry point - BIOS loads us at 0x7c00
start:
    ; Clear interrupts and setup segments
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti
    
    ; Save boot drive number
    mov [BOOT_DRIVE], dl
    
    ; Print 'A' - we're alive!
    mov ah, 0x0e
    mov al, 'A'
    int 0x10

    ; --- VESA GATEWAY TO GRAPHICS START ---
    ; 1. Get VBE Mode Information
    mov ax, 0x4F01          ; VBE Get Mode Info function
    mov cx, 0x115           ; Mode 0x115: 800x600, 32-bit color
    mov di, 0x8000          ; Use 0x8000 as a temporary buffer for info
    int 0x10                ; BIOS Video Interrupt

    cmp ax, 0x004F          ; Check for VBE success
    jne vesa_error          ; Handle error if VESA isn't supported

    ; 2. Save Physical Framebuffer Address (Offset 40 in ModeInfoBlock)
    mov eax, [0x8000 + 40]
    mov [vbe_framebuffer], eax
    ; Store at 0x500 (BDA free area, safe from kernel load at 0x1000+)
    mov [0x500], eax

    ; 3. Set the VBE Mode
    mov ax, 0x4F02          ; VBE Set Mode function
    mov bx, 0x4115          ; 0x115 | 0x4000 (Set LFB bit for Linear Framebuffer)
    int 0x10
    ; --- VESA GATEWAY TO GRAPHICS END ---

    ; Load kernel from disk using LBA
    call load_kernel
    
    ; Enter protected mode
    cli
    lgdt [gdt_descriptor]
    
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    
    ; Far jump to flush pipeline and enter protected mode
    jmp 0x08:protected_mode

[bits 32]
protected_mode:
    ; Setup segment registers for protected mode
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Setup stack
    ; Setup stack well above BSS (backbuffer ends at ~0x200000)
    mov ebp, 0x400000
    mov esp, ebp
    
    ; Jump to kernel
    jmp 0x08:KERNEL_OFFSET

[bits 16]
load_kernel:
    ;-------------------------------------------------------------------
    ; Use INT 13h AH=42h (Extended Read / LBA) to load the kernel.
    ; This avoids CHS geometry issues with hard-disk images in QEMU.
    ; We load 48 sectors (24 KB) starting at LBA 1 into 0x0000:0x1000.
    ; MAX is 54 sectors (ends at 0x7C00) — beyond that it overwrites
    ; the bootloader code itself! 48 leaves safe margin.
    ;-------------------------------------------------------------------

    ; First, test if LBA extensions are available
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc .try_chs              ; No LBA support, fallback to CHS

    ; --- LBA path (preferred) ---
    mov si, dap              ; DS:SI -> Disk Address Packet
    mov ah, 0x42             ; Extended Read
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error
    ret

.try_chs:
    ;-------------------------------------------------------------------
    ; CHS fallback for older BIOS / floppy
    ;-------------------------------------------------------------------
    mov ah, 0x00
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error

    xor ax, ax
    mov es, ax
    mov bx, KERNEL_OFFSET
    mov ch, 0               ; Cylinder 0
    mov dh, 0               ; Head 0
    mov cl, 2               ; Start from sector 2
    mov di, 48              ; 48 sectors to read (must not overwrite bootloader at 0x7C00)

.read_loop:
    push cx
    push dx
    push di
    push bx
    push es

    mov ah, 0x02
    mov al, 1
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error

    pop es
    pop bx
    pop di
    pop dx
    pop cx

    add bx, 512

    inc cl
    cmp cl, 19
    jl .no_geo_advance
    
    mov cl, 1
    inc dh
    cmp dh, 2
    jl .no_geo_advance
    
    mov dh, 0
    inc ch

.no_geo_advance:
    dec di
    jnz .read_loop
    ret

disk_error:
    mov ah, 0x0e
    mov al, 'E'
    int 0x10
    
    ; Print error code
    mov al, ah
    call print_hex
    
    ; Hang
    cli
    hlt
    jmp $

vesa_error:
    mov ah, 0x0e
    mov al, 'V'             ; Print 'V' for VESA Error
    int 0x10
    cli
    hlt
    jmp $

; Print AL as hex
print_hex:
    push ax
    shr al, 4
    call print_hex_digit
    pop ax
    and al, 0x0F
    call print_hex_digit
    ret

print_hex_digit:
    cmp al, 9
    jle .digit
    add al, 7
.digit:
    add al, '0'
    mov ah, 0x0e
    int 0x10
    ret

; ----- DATA -----

; Disk Address Packet for INT 13h AH=42h
ALIGN 4
dap:
    db 0x10                  ; Size of DAP (16 bytes)
    db 0                     ; Reserved
    dw 48                    ; Number of sectors to read (max 54, must not reach 0x7C00)
    dw KERNEL_OFFSET         ; Offset of destination buffer
    dw 0x0000                ; Segment of destination buffer
    dq 1                     ; Start LBA (sector 1 = first sector after bootsector)

; GDT Definition
gdt_start:
    ; Null descriptor
    dq 0x0

gdt_code:
    ; Code segment: base=0, limit=0xFFFFF, present, ring 0, executable, readable
    dw 0xFFFF       ; Limit 0:15
    dw 0x0000       ; Base 0:15
    db 0x00         ; Base 16:23
    db 10011010b    ; Access: present, ring 0, code, executable, readable
    db 11001111b    ; Flags: granularity, 32-bit
    db 0x00         ; Base 24:31

gdt_data:
    ; Data segment: base=0, limit=0xFFFFF, present, ring 0, writable
    dw 0xFFFF       ; Limit 0:15
    dw 0x0000       ; Base 0:15
    db 0x00         ; Base 16:23
    db 10010010b    ; Access: present, ring 0, data, writable
    db 11001111b    ; Flags: granularity, 32-bit
    db 0x00         ; Base 24:31

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; Size
    dd gdt_start                 ; Offset

BOOT_DRIVE: db 0
vbe_framebuffer: dd 0       ; Variable to store LFB address for Kernel access

; Pad to 510 bytes and add boot signature
times 510-($-$$) db 0
dw 0xAA55
