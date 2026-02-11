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
    
    ; Load kernel from disk
    call load_kernel
    
    ; Print 'L' - kernel loaded!
    mov ah, 0x0e
    mov al, 'L'
    int 0x10
    
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
    mov ebp, 0x90000
    mov esp, ebp
    
    ; Jump to kernel
    jmp 0x08:KERNEL_OFFSET

[bits 16]
load_kernel:
    ; Reset disk system
    mov ah, 0x00
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error
    
    ; Read kernel sectors using CHS
    ; We'll read 16 sectors to be safe (8KB)
    mov ah, 0x02          ; Read function
    mov al, 16            ; Number of sectors to read
    mov ch, 0             ; Cylinder 0
    mov cl, 2             ; Start from sector 2 (sector 1 is bootloader)
    mov dh, 0             ; Head 0
    mov dl, [BOOT_DRIVE]  ; Drive number
    
    ; Destination: 0x0000:0x1000
    xor bx, bx
    mov es, bx
    mov bx, KERNEL_OFFSET
    
    int 0x13
    jc disk_error
    
    ; Check if we read the right number of sectors
    cmp al, 16
    jne disk_error
    
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

; Pad to 510 bytes and add boot signature
times 510-($-$$) db 0
dw 0xAA55
