global loadPageDirectory
global enablePaging

loadPageDirectory:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8]
    mov cr3, eax      ; Load Page Directory physical address into CR3
    mov esp, ebp
    pop ebp
    ret

enablePaging:
    push ebp
    mov ebp, esp
    mov eax, cr0
    or eax, 0x80000001 ; Set the PG (Paging) and PE (Protection) bits
    mov cr0, eax
    mov esp, ebp
    pop ebp
    ret