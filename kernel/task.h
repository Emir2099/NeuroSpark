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

typedef struct {
  uint32_t esp;
  uint32_t ebp;
  uint32_t eip;
  uint32_t page_dir;
  uint32_t state;
  uint32_t wake_tick;
  uint32_t time_slice;
  int wait_reason;
  int task_id;
} TCB;

extern TCB os_tasks[MAX_TASKS];
extern int os_current_task;
extern int os_task_count;

void create_task(int index, void (*func_ptr)(), uint32_t page_dir);

#endif