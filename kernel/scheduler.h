#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"

#define SCHED_TIME_SLICE_TICKS 5
#define IPC_CHANNELS 4
#define IPC_QUEUE_CAPACITY 16

/* ============================================================
 * Locking Discipline and IRQ-Safe Annotations
 *
 * CRITICAL SECTIONS (protected by scheduler_lock):
 * - Task state transitions (READY, RUNNING, BLOCKED, SLEEPING, TERMINATED)
 * - Scheduler tick counter (scheduler_ticks)
 * - IPC channel queues (all 4 channels)
 * - Wait queue operations (task_ids, head, tail, count)
 * - Event signaling state
 *
 * INVARIANTS:
 * 1. All scheduler_lock acquisitions must be in pairs: irq_lock_acquire/release
 * 2. Interrupts are disabled during lock hold (cli implicit in irq_save)
 * 3. No sleeping, blocking, or rescheduling while holding scheduler_lock
 * 4. Lock is held for minimal duration - acquire, modify, release immediately
 *
 * RULE: If modifying os_task*, ipc_channels, wait_queue, or sched_event
 *       state, you need to hold scheduler_lock with irq_save/restore.
 * ============================================================ */

typedef struct {
	volatile int locked;
} irq_lock_t;

typedef struct {
	int task_ids[MAX_TASKS];
	int head;
	int tail;
	int count;
} wait_queue_t;

typedef struct {
	wait_queue_t waiters;
	int signaled;
} sched_event_t;

unsigned int irq_save(void);
void irq_restore(unsigned int flags);
void irq_lock_acquire(irq_lock_t *lock, unsigned int *flags_out);
void irq_lock_release(irq_lock_t *lock, unsigned int flags);

void schedule(void);
void scheduler_timer_tick(void);
uint32_t scheduler_now_ticks(void);

void task_yield(void);
void task_sleep(unsigned int ticks);
int task_sleep_timeout(unsigned int ticks, unsigned int max_ticks);
void task_terminate_current(void);
void task_exit_with_code(int code);
int task_kill(int task_id, int code);

void wait_queue_init(wait_queue_t *queue);
void wait_queue_wait(wait_queue_t *queue);
int wait_queue_wait_timeout(wait_queue_t *queue, unsigned int max_ticks);
int wait_queue_wake_one(wait_queue_t *queue);
int wait_queue_wake_all(wait_queue_t *queue);

void sched_event_init(sched_event_t *event);
void sched_event_wait(sched_event_t *event);
int sched_event_wait_timeout(sched_event_t *event, unsigned int max_ticks);
void sched_event_signal(sched_event_t *event);
void sched_event_broadcast(sched_event_t *event);

/* Basic non-blocking IPC */
int ipc_send(int channel, int value);
int ipc_recv(int channel, int *out_value);
int ipc_queue_depth(int channel);

/* Blocking IPC with optional timeout */
int ipc_send_wait(int channel, int value);
int ipc_send_timeout(int channel, int value, unsigned int max_ticks);
int ipc_recv_wait(int channel, int *out_value);
int ipc_recv_timeout(int channel, int *out_value, unsigned int max_ticks);

#endif
