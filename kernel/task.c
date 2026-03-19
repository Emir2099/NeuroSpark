#include "task.h"
#include "scheduler.h"

extern void *pmm_alloc_page(void);

TCB os_tasks[MAX_TASKS] = {
        {0, 0, 0, 0, TASK_STATE_RUNNING, 0, SCHED_TIME_SLICE_TICKS, 0, 0},
};
int os_current_task = 0;
int os_task_count = 1;

void create_task(int index, void (*func_ptr)(), uint32_t page_dir) {
    if (index <= 0 || index >= MAX_TASKS || func_ptr == 0) {
        return;
    }

    // Allocate 16KB stack (4 physical pages) for deep dashboard call paths.
    uint32_t stack_phys = (uint32_t)pmm_alloc_page();
    if (stack_phys == 0) {
        return;
    }
    pmm_alloc_page();
    pmm_alloc_page();
    pmm_alloc_page();
    uint32_t *stack = (uint32_t *)(stack_phys + 16384);

    // switch_task pops: ebx, esi, edi, ebp, ret-eip
    *--stack = (uint32_t)func_ptr;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;

    os_tasks[index].esp = (uint32_t)stack;
    os_tasks[index].ebp = 0;
    os_tasks[index].eip = (uint32_t)func_ptr;
    os_tasks[index].page_dir = page_dir;
    os_tasks[index].state = TASK_STATE_READY;
    os_tasks[index].wake_tick = 0;
    os_tasks[index].time_slice = SCHED_TIME_SLICE_TICKS;
    os_tasks[index].wait_reason = 0;
    os_tasks[index].task_id = index;

    if (index >= os_task_count) {
        os_task_count = index + 1;
    }
}