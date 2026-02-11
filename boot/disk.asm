load_kernel:
    pusha
    push dx

    ; Step 1: Reset the disk system (Important for some BIOS)
    mov ah, 0x00
    int 0x13
    jc disk_error

    ; Step 2: Read from disk
    mov ah, 0x02    ; Read sectors
    mov al, dh      ; Read dh sectors
    mov ch, 0x00    ; Cylinder 0
    mov dh, 0x00    ; Head 0
    mov cl, 0x02    ; Start from sector 2
    int 0x13

    jc disk_error
    pop dx
    cmp al, dh      ; Check if we read enough sectors
    jne disk_error
    popa
    ret

disk_error:
    mov ah, 0x0e
    mov al, 'E'
    int 0x10
    jmp $
