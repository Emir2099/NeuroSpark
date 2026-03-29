#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"

#define SCHED_TIME_SLICE_TICKS 5
#define IPC_CHANNELS 4
#define IPC_QUEUE_CAPACITY 16

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
void task_terminate_current(void);

void wait_queue_init(wait_queue_t *queue);
void wait_queue_wait(wait_queue_t *queue);
int wait_queue_wake_one(wait_queue_t *queue);
int wait_queue_wake_all(wait_queue_t *queue);

void sched_event_init(sched_event_t *event);
void sched_event_wait(sched_event_t *event);
void sched_event_signal(sched_event_t *event);
void sched_event_broadcast(sched_event_t *event);

int ipc_send(int channel, int value);
int ipc_recv(int channel, int *out_value);
int ipc_queue_depth(int channel);

#endif
