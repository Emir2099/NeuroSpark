[bits 32]

global enter_user_mode
global user_program_start
global user_program_end

section .text

enter_user_mode:
    ; enter_user_mode(entry_eip, user_stack, page_dir)
    mov eax, [esp + 12]
    mov cr3, eax

    mov edx, [esp + 4]
    mov ebx, [esp + 8]

    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push dword 0x23
    push ebx
    pushfd
    or dword [esp], 0x200
    push dword 0x1B
    push edx
    iretd

user_program_start:
    call .get_pc
.get_pc:
    pop esi
    add esi, user_msg - .get_pc

.loop:
    mov eax, 4
    mov ebx, esi
    int 0x80

    mov ecx, 0x180000
.delay:
    dec ecx
    jnz .delay

    jmp .loop

user_msg db "user-mode process alive", 10, 0
user_program_end:
