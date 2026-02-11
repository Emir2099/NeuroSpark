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