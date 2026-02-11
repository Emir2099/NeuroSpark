[bits 16]
switch_to_pm:
    cli                     ; Disable interrupts
    lgdt [gdt_descriptor]   ; Load our GDT map
    mov eax, cr0
    or eax, 0x1             ; Flip the Protected Mode bit
    mov cr0, eax
    jmp CODE_SEG:init_pm    ; Far jump to flush the pipeline

[bits 32]
init_pm:
    mov ax, DATA_SEG        ; Update all segment registers
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000        ; Update stack for 32-bit
    mov esp, ebp

    call BEGIN_PM           ; Return to boot.asm
