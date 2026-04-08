#include "task.h"
#include "scheduler.h"

extern void *pmm_alloc_page(void);

TCB os_tasks[MAX_TASKS] = {
    [0] = {0, 0, 0, 0, TASK_STATE_RUNNING, 0, SCHED_TIME_SLICE_TICKS, 0, 0, 1,
        WORKLOAD_CLASS_SIM, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0,
        {0},
        -1, 0, 0, 0, -1,
        {0}, TASK_WAIT_NONE, 0},
    [1] = {0, 0, 0, 0, TASK_STATE_TERMINATED, 0, SCHED_TIME_SLICE_TICKS, 0, 0, 1,
        WORKLOAD_CLASS_LOG, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0,
        {0},
        -1, 0, 0, 0, -1,
        {0}, TASK_WAIT_NONE, 1},
    [2] = {0, 0, 0, 0, TASK_STATE_TERMINATED, 0, SCHED_TIME_SLICE_TICKS, 0, 0, 1,
        WORKLOAD_CLASS_EXPORT, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0,
        {0},
        -1, 0, 0, 0, -1,
        {0}, TASK_WAIT_NONE, 2},
};
int os_current_task = 0;
int os_task_count = 1;

int task_is_valid(int task_id) {
    return task_id >= 0 && task_id < MAX_TASKS;
}

int task_alloc_slot(void) {
    for (int i = 1; i < MAX_TASKS; i++) {
        if (os_tasks[i].state == TASK_STATE_TERMINATED) {
            return i;
        }
    }
    return -1;
}

void task_trace_event(int task_id, uint32_t event, uint32_t arg) {
    if (!task_is_valid(task_id)) {
        return;
    }
    if (!os_tasks[task_id].trace_enabled) {
        return;
    }
    os_tasks[task_id].trace_last_event = event;
    os_tasks[task_id].trace_last_arg = arg;
    os_tasks[task_id].trace_event_count++;
}

void task_trace_syscall(int task_id, uint32_t syscall_num, uint32_t result) {
    uint32_t packed = ((syscall_num & 0xFFu) << 24) | (result & 0x00FFFFFFu);
    task_trace_event(task_id, TASK_TRACE_EVT_SYSCALL, packed);
}

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
    os_tasks[index].runtime_ticks = 0;
    os_tasks[index].context_switches = 0;
    os_tasks[index].priority = 1;
    if (index == 1) {
        os_tasks[index].workload_class = WORKLOAD_CLASS_LOG;
    } else if (index == 2) {
        os_tasks[index].workload_class = WORKLOAD_CLASS_EXPORT;
    } else {
        os_tasks[index].workload_class = WORKLOAD_CLASS_SIM;
    }
    os_tasks[index].trace_enabled = 0;
    os_tasks[index].trace_last_event = 0;
    os_tasks[index].trace_last_arg = 0;
    os_tasks[index].trace_event_count = 0;
    os_tasks[index].fault_code = 0;
    os_tasks[index].fault_addr = 0;
    os_tasks[index].fault_eip = 0;
    os_tasks[index].user_entry = 0;
    os_tasks[index].user_stack = 0;
    os_tasks[index].user_retval = 0;
    os_tasks[index].signal_pending = 0;
    os_tasks[index].signal_mask = 0;
    for (int sig = 0; sig < TASK_SIGNAL_MAX; sig++) {
        os_tasks[index].signal_handler[sig] = 0;
    }
    os_tasks[index].parent_pid = -1;
    os_tasks[index].exit_status = 0;
    os_tasks[index].exited = 0;
    os_tasks[index].waited = 0;
    os_tasks[index].wait_target = -1;
    for (int fd = 0; fd < TASK_MAX_FDS; fd++) {
        os_tasks[index].fd_table[fd] = -1;
    }
    os_tasks[index].wait_reason = 0;
    os_tasks[index].task_id = index;

    if (index >= os_task_count) {
        os_task_count = index + 1;
    }
}