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

extern schedule ; Defined in task.c

irq0_handler:
    pushad          ; Save all registers
    
    call schedule   ; The OS decides who runs next
    
    mov al, 0x20    ; Send EOI (End of Interrupt) to PIC
    out 0x20, al
    
    popad           ; Restore registers for the NEW task
    iretd           ; Jump back into the task's code