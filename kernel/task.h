#ifndef TASK_H
#define TASK_H

typedef unsigned int uint32_t;

enum {
  TASK_STATE_READY = 0,
  TASK_STATE_RUNNING = 1,
  TASK_STATE_BLOCKED = 2,
  TASK_STATE_SLEEPING = 3,
  TASK_STATE_TERMINATED = 4,
};

#define MAX_TASKS 3
#define TASK_MAX_FDS 256

enum {
  WORKLOAD_CLASS_SIM = 0,
  WORKLOAD_CLASS_LOG = 1,
  WORKLOAD_CLASS_EXPORT = 2,
};

enum {
  TASK_TRACE_EVT_SWITCH_IN = 1,
  TASK_TRACE_EVT_SWITCH_OUT = 2,
  TASK_TRACE_EVT_SYSCALL = 3,
  TASK_TRACE_EVT_SLEEP = 4,
  TASK_TRACE_EVT_WAKE = 5,
  TASK_TRACE_EVT_TERMINATE = 6,
  TASK_TRACE_EVT_PRIORITY = 7,
  TASK_TRACE_EVT_KILL = 8,
  TASK_TRACE_EVT_FAULT = 9,
};

typedef struct {
  uint32_t esp;
  uint32_t ebp;
  uint32_t eip;
  uint32_t page_dir;
  uint32_t state;
  uint32_t wake_tick;
  uint32_t time_slice;
  uint32_t runtime_ticks;
  uint32_t context_switches;
  uint32_t priority;
  uint32_t workload_class;
  uint32_t trace_enabled;
  uint32_t trace_last_event;
  uint32_t trace_last_arg;
  uint32_t trace_event_count;
  uint32_t fault_code;
  uint32_t fault_addr;
  uint32_t fault_eip;
  int fd_table[TASK_MAX_FDS];
  int wait_reason;
  int task_id;
} TCB;

extern TCB os_tasks[MAX_TASKS];
extern int os_current_task;
extern int os_task_count;

void create_task(int index, void (*func_ptr)(), uint32_t page_dir);
int task_is_valid(int task_id);
void task_trace_event(int task_id, uint32_t event, uint32_t arg);
void task_trace_syscall(int task_id, uint32_t syscall_num, uint32_t result);

#endif