[bits 32]
extern timer_handler
global timer_wrapper

timer_wrapper:
    pusha               ; Save all registers for the C kernel
    call timer_handler  ; Jump to the C timer function
    
    ; Send End-of-Interrupt (EOI) to the PIC (Port 0x20)
    mov al, 0x20
    out 0x20, al
    
    popa                ; Restore registers
    iretd               ; Return to whatever the CPU was doing


; New keyboard wrapper
global keyboard_wrapper
extern keyboard_handler

keyboard_wrapper:
    pusha
    call keyboard_handler
    
    ; Send End-of-Interrupt to PIC
    mov al, 0x20
    out 0x20, al
    
    popa
    iretd

global mouse_wrapper
extern mouse_handler

mouse_wrapper:
    pusha
    call mouse_handler

    ; IRQ12 comes from slave PIC: EOI slave first, then master
    mov al, 0x20
    out 0xA0, al
    out 0x20, al

    popa
    iretd

global rtl8139_wrapper
extern net_irq_handler

rtl8139_wrapper:
    pusha
    call net_irq_handler

    ; IRQ11 comes from slave PIC: EOI slave first, then master
    mov al, 0x20
    out 0xA0, al
    out 0x20, al

    popa
    iretd

extern schedule ; Defined in task.c

irq0_handler:
    pushad          ; Save all registers
    
    call schedule   ; The OS decides who runs next
    
    mov al, 0x20    ; Send EOI (End of Interrupt) to PIC
    out 0x20, al
    
    popad           ; Restore registers for the NEW task
    iretd           ; Jump back into the task's code

    extern syscall_handler

global syscall_wrapper
syscall_wrapper:
    pushad          ; Save all registers
    
    push esp        ; Pass the stack pointer (containing the registers) to C
    call syscall_handler
    add esp, 4      ; Clean up stack
    
    popad           ; Restore registers (potentially with a return value in EAX)
    iretd

global exception_wrapper
extern exception_handler

%macro EXC_NOERR 1
global exception%1_wrapper
exception%1_wrapper:
    mov edx, esp
    push edx
    push dword 0
    push dword %1
    call exception_handler
    add esp, 12
    iretd
%endmacro

%macro EXC_ERR 1
global exception%1_wrapper
exception%1_wrapper:
    mov eax, [esp]
    mov edx, esp
    push edx
    push eax
    push dword %1
    call exception_handler
    add esp, 12
    add esp, 4
    iretd
%endmacro

EXC_NOERR 0
EXC_NOERR 1
EXC_NOERR 2
EXC_NOERR 3
EXC_NOERR 4
EXC_NOERR 5
EXC_NOERR 6
EXC_NOERR 7
EXC_ERR 8
EXC_NOERR 9
EXC_ERR 10
EXC_ERR 11
EXC_ERR 12
EXC_ERR 13
EXC_ERR 14
EXC_NOERR 15
EXC_NOERR 16
EXC_ERR 17
EXC_NOERR 18
EXC_NOERR 19
EXC_NOERR 20
EXC_NOERR 21
EXC_NOERR 22
EXC_NOERR 23
EXC_NOERR 24
EXC_NOERR 25
EXC_NOERR 26
EXC_NOERR 27
EXC_NOERR 28
EXC_NOERR 29
EXC_NOERR 30
EXC_NOERR 31