typedef unsigned int uint32_t;

/* The TCB you just defined */
typedef struct {
    uint32_t esp; 
    uint32_t ebp;
    uint32_t eip;
    uint32_t page_dir;
    uint32_t state;
    int task_id;
} TCB;

extern void switch_task(TCB *old, TCB *new);
extern void* pmm_alloc_page();

TCB os_tasks[2]; 
int os_current_task = 0;

void schedule() {
    int old_task = os_current_task;
    os_current_task = (os_current_task + 1) % 2; // Simple Round-Robin
    
    // Perform the actual CPU context switch
    switch_task(&os_tasks[old_task], &os_tasks[os_current_task]);
}

/* Function to initialize a task's stack and state */
void create_task(int index, void (*func_ptr)(), uint32_t page_dir) {
    // 1. Allocate a 4KB physical page for the task's stack
    // Note: pmm_alloc_page is extern from pmm.c
    uint32_t stack_phys = (uint32_t)pmm_alloc_page(); 
    uint32_t *stack = (uint32_t *)(stack_phys + 4096); // Stack grows downwards

    // 2. Mock a CPU Stack Frame
    // We push values in reverse order of how 'switch_task' pops them
    *--stack = (uint32_t)func_ptr; // The EIP (Instruction Pointer)
    *--stack = 0;                  // EBP
    *--stack = 0;                  // EDI
    *--stack = 0;                  // ESI
    *--stack = 0;                  // EBX

    // 3. Save the stack top into the TCB
    os_tasks[index].esp = (uint32_t)stack;
    os_tasks[index].page_dir = page_dir;
    os_tasks[index].task_id = index;
    os_tasks[index].state = 1;    // State: Ready
}