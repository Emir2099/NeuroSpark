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

exception_wrapper:
    cli
    push esp
    call exception_handler
    add esp, 4
.halt:
    hlt
    jmp .halt