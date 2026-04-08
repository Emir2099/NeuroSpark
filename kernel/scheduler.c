#include "scheduler.h"
#include "profiling.h"
#include "paging.h"
#include "vfs.h"

typedef unsigned int uint32_t;

typedef struct {
  int voltage;
  int spike_count;
  int id;
  int synaptic_weight;
  int refractory_timer;
  int dynamic_threshold;
} Neuron;

typedef struct {
  Neuron neurons[5];
  unsigned char current_phase;
  int pixel_recent_spikes;
} NeuralPixel;

typedef struct {
  int task_id;
  int priority;
  int target_pixel;
  int base_integration;
  int fire_threshold;
  const char *task_name;
  int saved_voltages[5];
  int saved_weights[5];
  int saved_thresholds[5];
  int last_spike_count;
  int spikes_per_second;
  int state;
} TaskControlBlock;

extern void switch_task(TCB *old, TCB *new);
extern TaskControlBlock task_list[2];
extern NeuralPixel os_memory_map[2];

static irq_lock_t scheduler_lock = {0};
static volatile uint32_t scheduler_ticks = 0;

typedef struct {
  int queue[IPC_QUEUE_CAPACITY];
  int head;
  int tail;
  int count;
} ipc_channel_t;

static ipc_channel_t ipc_channels[IPC_CHANNELS];
static wait_queue_t ipc_send_waiters[IPC_CHANNELS];
static wait_queue_t ipc_recv_waiters[IPC_CHANNELS];
static int ipc_initialized = 0;

static void ipc_init_once(void) {
  if (ipc_initialized) {
    return;
  }
  for (int c = 0; c < IPC_CHANNELS; c++) {
    ipc_channels[c].head = 0;
    ipc_channels[c].tail = 0;
    ipc_channels[c].count = 0;
    for (int i = 0; i < IPC_QUEUE_CAPACITY; i++) {
      ipc_channels[c].queue[i] = 0;
    }
    wait_queue_init(&ipc_send_waiters[c]);
    wait_queue_init(&ipc_recv_waiters[c]);
  }
  ipc_initialized = 1;
}

/* ============================================================
 * Shared Structure Lock Audit
 *
 * SCHEDULER_LOCK protects:
 * - os_tasks[] array: All state, wake_tick, time_slice fields
 * - scheduler_ticks: Global time counter
 * - ipc_channels[]: All 4 channel queues (head, tail, count)
 * - Event signaled flags and wait queue task lists
 *
 * KB_LOCK (in syscall.c) protects:
 * - kb_buf[], kb_head, kb_tail: Keyboard input buffer
 *
 * Other structures:
 * - page_directory: User page dir is per-task (TCB.page_dir), not shared
 * - Disk operations in vfs.c and disk.c: Protected by their own I/O ops
 * - Network queues (net.c): Protected if accessed from tasks; serial in ISR
 *
 * DISCIPLINE:
 * 1. All task state changes require scheduler_lock
 * 2. IPC operations require scheduler_lock
 * 3. Never call task_yield/schedule while holding a lock (would deadlock)
 * 4. Keyboard buffer read in ISR uses kb_lock
 * ============================================================ */

static uint32_t time_slice_for_priority(uint32_t prio) {
  if (prio == 0) {
    return 3;
  }
  if (prio >= 3) {
    return 8;
  }
  return 5 + prio;
}

unsigned int irq_save(void) {
  unsigned int flags;
  __asm__ volatile("pushf; pop %0; cli" : "=r"(flags) : : "memory");
  return flags;
}

void irq_restore(unsigned int flags) {
  __asm__ volatile("push %0; popf" : : "r"(flags) : "memory", "cc");
}

void irq_lock_acquire(irq_lock_t *lock, unsigned int *flags_out) {
  unsigned int flags = irq_save();
  while (lock->locked) {
  }
  lock->locked = 1;
  __asm__ volatile("" ::: "memory");
  if (flags_out != 0) {
    *flags_out = flags;
  }
}

void irq_lock_release(irq_lock_t *lock, unsigned int flags) {
  __asm__ volatile("" ::: "memory");
  lock->locked = 0;
  irq_restore(flags);
}

static void scheduler_wake_sleepers_locked(void) {
  for (int i = 0; i < os_task_count; i++) {
    if ((os_tasks[i].state == TASK_STATE_SLEEPING ||
         os_tasks[i].state == TASK_STATE_BLOCKED) &&
        os_tasks[i].wake_tick != 0 &&
        os_tasks[i].wake_tick <= scheduler_ticks) {
      os_tasks[i].state = TASK_STATE_READY;
      os_tasks[i].wake_tick = 0;
      task_trace_event(i, TASK_TRACE_EVT_WAKE, scheduler_ticks);
    }
  }
}

static int scheduler_pick_next_locked(int old_task) {
  int best_idx = -1;
  uint32_t best_prio = 0;

  for (int step = 1; step <= os_task_count; step++) {
    int idx = (old_task + step) % os_task_count;
    if (os_tasks[idx].state == TASK_STATE_READY) {
      uint32_t prio = os_tasks[idx].priority;
      if (best_idx < 0 || prio > best_prio) {
        best_idx = idx;
        best_prio = prio;
      }
    }
  }

  if (best_idx >= 0) {
    return best_idx;
  }

  if (os_tasks[old_task].state == TASK_STATE_RUNNING ||
      os_tasks[old_task].state == TASK_STATE_READY) {
    return old_task;
  }

  for (int i = 0; i < os_task_count; i++) {
    if (os_tasks[i].state != TASK_STATE_BLOCKED &&
        os_tasks[i].state != TASK_STATE_SLEEPING &&
        os_tasks[i].state != TASK_STATE_TERMINATED) {
      return i;
    }
  }

  return old_task;
}

static void scheduler_select_and_switch(int force_current_ready) {
  int old_task;
  int new_task;
  unsigned int flags;

  irq_lock_acquire(&scheduler_lock, &flags);

  old_task = os_current_task;
  if (force_current_ready && os_tasks[old_task].state == TASK_STATE_RUNNING) {
    os_tasks[old_task].state = TASK_STATE_READY;
  }

  scheduler_wake_sleepers_locked();
  new_task = scheduler_pick_next_locked(old_task);

  os_current_task = new_task;
  os_tasks[new_task].state = TASK_STATE_RUNNING;
  os_tasks[new_task].time_slice = time_slice_for_priority(os_tasks[new_task].priority);
  if (new_task != old_task) {
    task_trace_event(old_task, TASK_TRACE_EVT_SWITCH_OUT, new_task);
    task_trace_event(new_task, TASK_TRACE_EVT_SWITCH_IN, old_task);
    os_tasks[new_task].context_switches++;
  }

  irq_lock_release(&scheduler_lock, flags);

  if (new_task != old_task) {
    switch_task(&os_tasks[old_task], &os_tasks[new_task]);
  }
}

void schedule(void) { scheduler_select_and_switch(1); }

void task_yield(void) { schedule(); }

void task_sleep(unsigned int ticks) {
  unsigned int flags;
  int current;

  if (ticks == 0) {
    task_yield();
    return;
  }

  irq_lock_acquire(&scheduler_lock, &flags);
  current = os_current_task;
  os_tasks[current].state = TASK_STATE_SLEEPING;
  os_tasks[current].wake_tick = scheduler_ticks + ticks;
  task_trace_event(current, TASK_TRACE_EVT_SLEEP, ticks);
  irq_lock_release(&scheduler_lock, flags);

  scheduler_select_and_switch(0);
}

void task_exit_with_code(int code) {
  unsigned int flags;
  uint32_t task_pd = 0;
  int current = 0;
  int has_fault = 0;
  int parent = -1;

  irq_lock_acquire(&scheduler_lock, &flags);
  current = os_current_task;
  parent = os_tasks[current].parent_pid;
  task_pd = os_tasks[current].page_dir;
  has_fault = (os_tasks[current].fault_code != 0 ||
               os_tasks[current].fault_addr != 0 ||
               os_tasks[current].fault_eip != 0);
  if (!has_fault) {
    task_trace_event(os_current_task, TASK_TRACE_EVT_TERMINATE, 0);
  }
  os_tasks[os_current_task].state = TASK_STATE_TERMINATED;
  os_tasks[os_current_task].wake_tick = 0;
  os_tasks[os_current_task].exit_status = code;
  os_tasks[os_current_task].exited = 1;

  if (parent >= 0 && parent < os_task_count &&
      os_tasks[parent].state == TASK_STATE_BLOCKED &&
      os_tasks[parent].wait_reason == TASK_WAIT_CHILD) {
    int target = os_tasks[parent].wait_target;
    if (target < 0 || target == current) {
      os_tasks[parent].state = TASK_STATE_READY;
      os_tasks[parent].wake_tick = 0;
      os_tasks[parent].wait_reason = TASK_WAIT_NONE;
      os_tasks[parent].wait_target = -1;
    }
  }
  irq_lock_release(&scheduler_lock, flags);

  vfs_close_all_for_task(current);

  if (task_pd != 0 && task_pd != (uint32_t)page_directory) {
    destroy_user_address_space(task_pd);
    os_tasks[current].page_dir = 0;
  }

  scheduler_select_and_switch(0);
  while (1) {
    __asm__ volatile("hlt");
  }
}

void task_terminate_current(void) { task_exit_with_code(0); }

int task_kill(int task_id, int code) {
  unsigned int flags;
  uint32_t task_pd;
  int parent;

  if (task_id < 0 || task_id >= os_task_count) {
    return 0;
  }

  if (task_id == os_current_task) {
    task_exit_with_code(code);
    return 1;
  }

  irq_lock_acquire(&scheduler_lock, &flags);
  if (os_tasks[task_id].state == TASK_STATE_TERMINATED) {
    irq_lock_release(&scheduler_lock, flags);
    return 0;
  }

  parent = os_tasks[task_id].parent_pid;
  task_pd = os_tasks[task_id].page_dir;
  os_tasks[task_id].state = TASK_STATE_TERMINATED;
  os_tasks[task_id].wake_tick = 0;
  os_tasks[task_id].exit_status = code;
  os_tasks[task_id].exited = 1;

  if (parent >= 0 && parent < os_task_count &&
      os_tasks[parent].state == TASK_STATE_BLOCKED &&
      os_tasks[parent].wait_reason == TASK_WAIT_CHILD) {
    int target = os_tasks[parent].wait_target;
    if (target < 0 || target == task_id) {
      os_tasks[parent].state = TASK_STATE_READY;
      os_tasks[parent].wake_tick = 0;
      os_tasks[parent].wait_reason = TASK_WAIT_NONE;
      os_tasks[parent].wait_target = -1;
    }
  }
  irq_lock_release(&scheduler_lock, flags);

  vfs_close_all_for_task(task_id);
  if (task_pd != 0 && task_pd != (uint32_t)page_directory) {
    destroy_user_address_space(task_pd);
    os_tasks[task_id].page_dir = 0;
  }
  return 1;
}

void scheduler_timer_tick(void) {
  int should_preempt = 0;
  unsigned int flags;
  int current;
  uint32_t profile_stamp = profile_begin();

  irq_lock_acquire(&scheduler_lock, &flags);
  scheduler_ticks++;
  scheduler_wake_sleepers_locked();

  current = os_current_task;
  if (os_tasks[current].state == TASK_STATE_RUNNING) {
    os_tasks[current].runtime_ticks++;
    if (os_tasks[current].time_slice > 0) {
      os_tasks[current].time_slice--;
    }
    if (os_tasks[current].time_slice == 0) {
      should_preempt = 1;
    }
  }

  irq_lock_release(&scheduler_lock, flags);

  if (should_preempt) {
    /* Do not context-switch from IRQ context here.
     * Switching from timer interrupt leaves IF cleared in the resumed task,
     * which stalls subsequent timer ticks and makes UI appear frozen.
     * Keep round-robin accounting and let normal task yield points switch. */
    irq_lock_acquire(&scheduler_lock, &flags);
    if (os_tasks[current].state == TASK_STATE_RUNNING) {
      os_tasks[current].time_slice = time_slice_for_priority(os_tasks[current].priority);
    }
    irq_lock_release(&scheduler_lock, flags);
  }

  profile_end(PROFILE_SLOT_SCHED_TICK, profile_stamp);
}

uint32_t scheduler_now_ticks(void) { return scheduler_ticks; }

int ipc_send(int channel, int value) {
  unsigned int flags;
  ipc_channel_t *ch;
  int wake_recv = 0;

  if (channel < 0 || channel >= IPC_CHANNELS) {
    return 0;
  }

  ipc_init_once();

  irq_lock_acquire(&scheduler_lock, &flags);
  ch = &ipc_channels[channel];
  if (ch->count >= IPC_QUEUE_CAPACITY) {
    irq_lock_release(&scheduler_lock, flags);
    return 0;
  }

  ch->queue[ch->tail] = value;
  ch->tail = (ch->tail + 1) % IPC_QUEUE_CAPACITY;
  ch->count++;
  wake_recv = (ch->count > 0);

  irq_lock_release(&scheduler_lock, flags);
  if (wake_recv) {
    wait_queue_wake_one(&ipc_recv_waiters[channel]);
  }
  return 1;
}

int ipc_recv(int channel, int *out_value) {
  unsigned int flags;
  ipc_channel_t *ch;
  int wake_send = 0;

  if (channel < 0 || channel >= IPC_CHANNELS || out_value == 0) {
    return 0;
  }

  ipc_init_once();

  irq_lock_acquire(&scheduler_lock, &flags);
  ch = &ipc_channels[channel];
  if (ch->count <= 0) {
    irq_lock_release(&scheduler_lock, flags);
    return 0;
  }

  *out_value = ch->queue[ch->head];
  ch->head = (ch->head + 1) % IPC_QUEUE_CAPACITY;
  ch->count--;
  wake_send = (ch->count < IPC_QUEUE_CAPACITY);

  irq_lock_release(&scheduler_lock, flags);
  if (wake_send) {
    wait_queue_wake_one(&ipc_send_waiters[channel]);
  }
  return 1;
}

int ipc_queue_depth(int channel) {
  unsigned int flags;
  int depth;

  if (channel < 0 || channel >= IPC_CHANNELS) {
    return -1;
  }

  ipc_init_once();

  irq_lock_acquire(&scheduler_lock, &flags);
  depth = ipc_channels[channel].count;
  irq_lock_release(&scheduler_lock, flags);
  return depth;
}

void wait_queue_init(wait_queue_t *queue) {
  if (queue == 0) {
    return;
  }
  queue->head = 0;
  queue->tail = 0;
  queue->count = 0;
  for (int i = 0; i < MAX_TASKS; i++) {
    queue->task_ids[i] = -1;
  }
}

void wait_queue_wait(wait_queue_t *queue) {
  unsigned int flags;
  int current;

  if (queue == 0) {
    return;
  }

  irq_lock_acquire(&scheduler_lock, &flags);
  current = os_current_task;
  if (queue->count < MAX_TASKS) {
    queue->task_ids[queue->tail] = current;
    queue->tail = (queue->tail + 1) % MAX_TASKS;
    queue->count++;
    os_tasks[current].state = TASK_STATE_BLOCKED;
  }
  irq_lock_release(&scheduler_lock, flags);

  scheduler_select_and_switch(0);
}

int wait_queue_wake_one(wait_queue_t *queue) {
  unsigned int flags;
  int woken = 0;

  if (queue == 0) {
    return 0;
  }

  irq_lock_acquire(&scheduler_lock, &flags);
  while (queue->count > 0) {
    int task_id = queue->task_ids[queue->head];
    queue->task_ids[queue->head] = -1;
    queue->head = (queue->head + 1) % MAX_TASKS;
    queue->count--;

    if (task_id >= 0 && task_id < os_task_count &&
        os_tasks[task_id].state == TASK_STATE_BLOCKED) {
      os_tasks[task_id].state = TASK_STATE_READY;
      woken = 1;
      break;
    }
  }
  irq_lock_release(&scheduler_lock, flags);

  return woken;
}

int wait_queue_wake_all(wait_queue_t *queue) {
  int total = 0;
  while (wait_queue_wake_one(queue)) {
    total++;
  }
  return total;
}

void sched_event_init(sched_event_t *event) {
  if (event == 0) {
    return;
  }
  event->signaled = 0;
  wait_queue_init(&event->waiters);
}

void sched_event_wait(sched_event_t *event) {
  unsigned int flags;

  if (event == 0) {
    return;
  }

  irq_lock_acquire(&scheduler_lock, &flags);
  if (event->signaled) {
    event->signaled = 0;
    irq_lock_release(&scheduler_lock, flags);
    return;
  }
  irq_lock_release(&scheduler_lock, flags);

  wait_queue_wait(&event->waiters);
}

void sched_event_signal(sched_event_t *event) {
  if (event == 0) {
    return;
  }

  if (!wait_queue_wake_one(&event->waiters)) {
    unsigned int flags;
    irq_lock_acquire(&scheduler_lock, &flags);
    event->signaled = 1;
    irq_lock_release(&scheduler_lock, flags);
  }
}

void sched_event_broadcast(sched_event_t *event) {
  unsigned int flags;

  if (event == 0) {
    return;
  }

  wait_queue_wake_all(&event->waiters);

  irq_lock_acquire(&scheduler_lock, &flags);
  event->signaled = 1;
  irq_lock_release(&scheduler_lock, flags);
}

/* ============================================================
 * Timeout-capable sleep/wait primitives
 * ============================================================ */

int task_sleep_timeout(unsigned int ticks, unsigned int max_ticks) {
  unsigned int flags;
  int current;
  uint32_t deadline;

  if (ticks == 0) {
    task_yield();
    return 1;
  }

  if (max_ticks == 0) {
    return 0;
  }

  irq_lock_acquire(&scheduler_lock, &flags);
  current = os_current_task;
  deadline = scheduler_ticks + max_ticks;

  os_tasks[current].state = TASK_STATE_SLEEPING;
  os_tasks[current].wake_tick = scheduler_ticks + ticks;

  /* If sleep duration exceeds timeout, cap at deadline */
  if (os_tasks[current].wake_tick > deadline) {
    os_tasks[current].wake_tick = deadline;
  }

  task_trace_event(current, TASK_TRACE_EVT_SLEEP, ticks);
  irq_lock_release(&scheduler_lock, flags);

  scheduler_select_and_switch(0);

  /* Return 1 if we woke naturally, 0 if timeout */
  irq_lock_acquire(&scheduler_lock, &flags);
  int timed_out = (scheduler_ticks >= deadline);
  irq_lock_release(&scheduler_lock, flags);

  return !timed_out;
}

int wait_queue_wait_timeout(wait_queue_t *queue, unsigned int max_ticks) {
  unsigned int flags;
  int current;
  uint32_t deadline;

  if (queue == 0 || max_ticks == 0) {
    return 0;
  }

  irq_lock_acquire(&scheduler_lock, &flags);
  current = os_current_task;
  deadline = scheduler_ticks + max_ticks;

  if (queue->count < MAX_TASKS) {
    queue->task_ids[queue->tail] = current;
    queue->tail = (queue->tail + 1) % MAX_TASKS;
    queue->count++;
    os_tasks[current].state = TASK_STATE_BLOCKED;
    os_tasks[current].wake_tick = deadline;
  }
  irq_lock_release(&scheduler_lock, flags);

  scheduler_select_and_switch(0);

  /* Check if we timed out */
  irq_lock_acquire(&scheduler_lock, &flags);
  int timed_out = (scheduler_ticks >= deadline);
  irq_lock_release(&scheduler_lock, flags);

  return !timed_out;
}

int sched_event_wait_timeout(sched_event_t *event, unsigned int max_ticks) {
  unsigned int flags;

  if (event == 0 || max_ticks == 0) {
    return 0;
  }

  irq_lock_acquire(&scheduler_lock, &flags);
  if (event->signaled) {
    event->signaled = 0;
    irq_lock_release(&scheduler_lock, flags);
    return 1;
  }
  irq_lock_release(&scheduler_lock, flags);

  return wait_queue_wait_timeout(&event->waiters, max_ticks);
}

/* ============================================================
 * PHASE 17: Blocking IPC with timeout support
 * Ensures no busy-waiting in user-facing operations.
 * ============================================================ */

int ipc_send_wait(int channel, int value) {
  if (channel < 0 || channel >= IPC_CHANNELS) {
    return 0;
  }

  ipc_init_once();

  while (1) {
    if (ipc_send(channel, value)) {
      return 1;
    }
    wait_queue_wait(&ipc_send_waiters[channel]);
  }
}

int ipc_send_timeout(int channel, int value, unsigned int max_ticks) {
  uint32_t start;
  uint32_t elapsed;
  uint32_t remaining;

  if (channel < 0 || channel >= IPC_CHANNELS || max_ticks == 0) {
    return 0;
  }

  ipc_init_once();

  start = scheduler_now_ticks();
  while (1) {
    if (ipc_send(channel, value)) {
      return 1;
    }

    elapsed = scheduler_now_ticks() - start;
    if (elapsed >= max_ticks) {
      return 0;
    }

    remaining = max_ticks - elapsed;
    if (!wait_queue_wait_timeout(&ipc_send_waiters[channel], remaining)) {
      return 0;
    }
  }
}

int ipc_recv_wait(int channel, int *out_value) {
  if (channel < 0 || channel >= IPC_CHANNELS || out_value == 0) {
    return 0;
  }

  ipc_init_once();

  while (1) {
    if (ipc_recv(channel, out_value)) {
      return 1;
    }
    wait_queue_wait(&ipc_recv_waiters[channel]);
  }
}

int ipc_recv_timeout(int channel, int *out_value, unsigned int max_ticks) {
  uint32_t start;
  uint32_t elapsed;
  uint32_t remaining;

  if (channel < 0 || channel >= IPC_CHANNELS || out_value == 0 || max_ticks == 0) {
    return 0;
  }

  ipc_init_once();

  start = scheduler_now_ticks();
  while (1) {
    if (ipc_recv(channel, out_value)) {
      return 1;
    }

    elapsed = scheduler_now_ticks() - start;
    if (elapsed >= max_ticks) {
      return 0;
    }

    remaining = max_ticks - elapsed;
    if (!wait_queue_wait_timeout(&ipc_recv_waiters[channel], remaining)) {
      return 0;
    }
  }
}

void switch_tasks(void) {
  for (int p = 0; p < 2; p++) {
    for (int t = 0; t < 2; t++) {
      if (task_list[t].target_pixel == p) {
        for (int n = 0; n < 5; n++) {
          task_list[t].saved_voltages[n] = os_memory_map[p].neurons[n].voltage;
          task_list[t].saved_weights[n] =
              os_memory_map[p].neurons[n].synaptic_weight;
          task_list[t].saved_thresholds[n] =
              os_memory_map[p].neurons[n].dynamic_threshold;
        }
        break;
      }
    }
  }

  TaskControlBlock temp = task_list[0];
  task_list[0] = task_list[1];
  task_list[1] = temp;
  task_list[0].target_pixel = 0;
  task_list[1].target_pixel = 1;

  for (int p = 0; p < 2; p++) {
    for (int t = 0; t < 2; t++) {
      if (task_list[t].target_pixel == p) {
        for (int n = 0; n < 5; n++) {
          os_memory_map[p].neurons[n].voltage = task_list[t].saved_voltages[n];
          os_memory_map[p].neurons[n].synaptic_weight =
              task_list[t].saved_weights[n];
          os_memory_map[p].neurons[n].dynamic_threshold =
              task_list[t].saved_thresholds[n];
        }
        break;
      }
    }
  }
}
