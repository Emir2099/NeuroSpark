global switch_task

; switch_task(TCB *old, TCB *new)
switch_task:
    ; 1. Save old task's context
    push ebp
    push edi
    push esi
    push ebx

    ; Get the 'old' TCB pointer (first argument)
    mov eax, [esp + 20]     
    mov [eax], esp          ; Save current ESP (stack pointer) into TCB

    ; 2. Load new task's context
    mov eax, [esp + 24]     ; Get the 'new' TCB pointer (second argument)
    mov ecx, [eax + 12]     ; Load new task CR3 (TCB.page_dir)
    mov cr3, ecx
    mov esp, [eax]          ; Load the new task's ESP

    ; 3. Restore new task's registers
    pop ebx
    pop esi
    pop edi
    pop ebp
    ret                     ; Jump to the new task's EIP