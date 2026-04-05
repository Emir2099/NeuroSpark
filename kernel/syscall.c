/* syscall.c – NeuroSpark system call dispatcher */

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

#include "paging.h"
#include "task.h"
#include "scheduler.h"

extern int is_user_range(const void *ptr, uint32_t size);

enum {
  NS_OK = 0,
  NS_ERR_INVALID_SYSCALL = (uint32_t)-1,
  NS_ERR_BAD_POINTER = (uint32_t)-2,
  NS_ERR_WOULD_BLOCK = (uint32_t)-3,
  NS_ERR_PERMISSION = (uint32_t)-4,
};

#define MAX_USER_STRING_LEN 128

/* ============================================================
 * Syscall Numbers
 * ============================================================ */
#define SYS_EXIT 1
#define SYS_FORK 2
#define SYS_READ 3
#define SYS_WRITE 4
#define SYS_READ_KB                                                            \
  5 /* non-blocking keyboard read from circular buffer                    \
     */
#define SYS_CLOSE 6
#define SYS_FLIP 10
#define SYS_IPC_SEND 11
#define SYS_IPC_RECV 12

/* ============================================================
 * Syscall Validation and Error Codes
 *
 * All syscall handlers apply consistent validation:
 * - CPL check: Ring 3 user calls are checked against permission flags.
 * - Pointer validation: All user pointers are checked against the current
 *   task's page directory before dereference.
 * - String validation: NUL-terminated strings are scanned up to
 *   MAX_USER_STRING_LEN and must be NUL-terminated within that bound.
 * - Input bounds: Channels, file descriptors, and counts are checked
 *   against known valid ranges.
 *
 * Error codes (stable across syscall boundaries):
 * - NS_OK (0): success.
 * - NS_ERR_INVALID_SYSCALL (-1): syscall number not recognized.
 * - NS_ERR_BAD_POINTER (-2): user pointer failed validation (out of user
 *   range, not present, or lacks required permissions).
 * - NS_ERR_WOULD_BLOCK (-3): operation would block (queue full, no data).
 * - NS_ERR_PERMISSION (-4): insufficient privilege (e.g., Ring 3 calls a
 *   kernel-only syscall).
 *
 * Exit criteria:
 * - Forced invalid pointer syscalls fail safely with NS_ERR_BAD_POINTER.
 * - All syscall handlers validate user-space inputs uniformly.
 * ============================================================ */

/* ============================================================
 * Externals
 * ============================================================ */
extern void gprint(const char *str, unsigned int color);
extern void flip_buffer(void);
extern volatile int current_shell_row;

/* Keyboard circular buffer (defined in kernel.c) */
#define KB_BUF_SIZE 32
extern volatile char kb_buf[KB_BUF_SIZE];
extern volatile int kb_head;
extern volatile int kb_tail;

/* Task yield */
extern void task_yield(void);
extern int ipc_send(int channel, int value);
extern int ipc_recv(int channel, int *out_value);

static irq_lock_t kb_lock = {0};
extern int os_current_task;

static uint32_t current_task_page_dir(void) {
  if (os_current_task < 0 || os_current_task >= MAX_TASKS) {
    return 0;
  }
  return os_tasks[os_current_task].page_dir;
}

static uint32_t syscall_caller_cpl(uint32_t *regs) {
  uint32_t *iret_frame = (uint32_t *)((uint8_t *)regs + (8 * sizeof(uint32_t)));
  uint32_t saved_cs = iret_frame[1];
  return saved_cs & 0x3;
}

/* ============================================================
 * validate_user_string: check a NUL-terminated user-space string.
 *
 * Called by Ring 3 string-passing syscalls (SYS_WRITE, etc.)
 * Returns 1 if the string is accessible and NUL-terminated within
 * MAX_USER_STRING_LEN bytes, 0 otherwise.
 * ============================================================ */
static int validate_user_string(const char *s) {
  if (s == (const char *)0) {
    return 0;
  }
  for (uint32_t i = 0; i < MAX_USER_STRING_LEN; i++) {
    if (!is_user_range_accessible(current_task_page_dir(), (const void *)(s + i), 1)) {
      return 0;
    }
    if (s[i] == '\0') {
      return 1;
    }
  }
  return 0;
}

/* ============================================================
 * validate_user_ptr: check a user-space memory range.
 *
 * Called by Ring 3 syscalls passing buffers or structures.
 * Returns 1 if the range [ptr, ptr+size) is accessible and present
 * in the current task's page table, 0 otherwise.
 * ============================================================ */
static int validate_user_ptr(const void *ptr, uint32_t size) {
  if (ptr == (const void *)0 || size == 0) {
    return 0;
  }
  return is_user_range_accessible(current_task_page_dir(), ptr, size);
}

/* ============================================================
 * validate_user_int_ptr: check a user-space int pointer.
 *
 * Convenience helper for syscalls passing int* output parameters.
 * ============================================================ */
static int validate_user_int_ptr(const int *p) {
  if (p == (const int *)0) {
    return 0;
  }
  return validate_user_ptr((const void *)p, sizeof(int));
}

/* ============================================================
 * syscall_validate_cpl: enforce Ring 3 check for privileged syscalls.
 *
 * If cpl == 3 (user mode) and privilege is required, return 0.
 * Otherwise return 1 (allowed).
 * ============================================================ */
static int syscall_validate_cpl(uint32_t cpl, int kernel_only) {
  if (kernel_only && cpl == 3) {
    return 0;
  }
  return 1;
}

static void syscall_complete(uint32_t *regs, uint32_t syscall_num, uint32_t result) {
  regs[7] = result;
  task_trace_syscall(os_current_task, syscall_num, result);
}

static void syscall_complete_value(uint32_t *regs, uint32_t syscall_num,
                                   uint32_t result, uint32_t value) {
  regs[5] = value; /* EDX carries the normalized payload. */
  syscall_complete(regs, syscall_num, result);
}

/* ============================================================
 * Flip mutex (defined in kernel.c)
 * Prevents the keyboard ISR from calling gprint while
 * flip_buffer's rep-movsl is running.
 * ============================================================ */
extern volatile int flip_mutex;

/* ============================================================
 * syscall_handler  (called by the int 0x80 stub in interrupt.asm)
 * regs layout (pushed by the stub, lowest address first):
 *   [0]=EDI [1]=ESI [2]=EBP [3]=OLD_ESP [4]=EBX [5]=EDX [6]=ECX [7]=EAX
 * ============================================================ */
void syscall_handler(uint32_t *regs) {
  uint32_t syscall_num = regs[7]; /* EAX */
  uint32_t cpl = syscall_caller_cpl(regs);

  switch (syscall_num) {

  /* ---- SYS_FORK (2): not yet implemented ---- */
  case SYS_FORK:
    syscall_complete(regs, syscall_num, NS_ERR_INVALID_SYSCALL);
    break;

  /* ---- SYS_READ (3): not yet implemented ---- */
  case SYS_READ:
    syscall_complete(regs, syscall_num, NS_ERR_INVALID_SYSCALL);
    break;

  /* ---- SYS_CLOSE (6): not yet implemented ---- */
  case SYS_CLOSE:
    syscall_complete(regs, syscall_num, NS_ERR_INVALID_SYSCALL);
    break;

  /* ---- SYS_EXIT (1): terminate current user task (Ring 3 only) ---- */
  case SYS_EXIT: {
    extern void task_terminate_current(void);
    if (cpl != 3) {
      syscall_complete(regs, syscall_num, NS_ERR_PERMISSION);
      break;
    }
    syscall_complete(regs, syscall_num, NS_OK);
    task_terminate_current();
    break;
  }

  /* ---- SYS_WRITE (4): write a NUL-terminated string via gprint ---- */
  case SYS_WRITE: {
    char *msg = (char *)regs[4]; /* EBX */
    if (cpl == 3) {
      if (!validate_user_string(msg)) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }
    }
    gprint(msg, 0x00FF00);       /* Green */
    syscall_complete(regs, syscall_num, NS_OK);
    break;
  }

  /* ---- SYS_READ_KB (5): non-blocking keyboard buffer read ---- */
  case SYS_READ_KB: {
    unsigned int flags;
    irq_lock_acquire(&kb_lock, &flags);

    if (kb_head == kb_tail) {
      irq_lock_release(&kb_lock, flags);
      /* Buffer empty – cooperatively yield, caller must retry */
      task_yield();
      syscall_complete(regs, syscall_num, NS_ERR_WOULD_BLOCK);
    } else {
      char c = kb_buf[kb_tail];
      kb_tail = (kb_tail + 1) % KB_BUF_SIZE;
      irq_lock_release(&kb_lock, flags);
      syscall_complete_value(regs, syscall_num, NS_OK, (uint32_t)(unsigned char)c);
    }
    break;
  }

  /* ---- SYS_FLIP (10): blit backbuffer → VRAM, mutex-guarded (kernel-only) ---- */
  case SYS_FLIP: {
    if (!syscall_validate_cpl(cpl, 1)) {
      syscall_complete(regs, syscall_num, NS_ERR_PERMISSION);
      break;
    }

    /* Disable interrupts for the duration of the blit so that
     * an incoming keyboard IRQ cannot call gprint simultaneously */
    __asm__ volatile("cli");
    flip_mutex = 1;
    flip_buffer();
    flip_mutex = 0;
    __asm__ volatile("sti");
    syscall_complete(regs, syscall_num, NS_OK);
    break;
  }

  /* ============================================================
   * IPC Backpressure and Deterministic Overflow
   *
   * SYS_IPC_SEND (11) and SYS_IPC_RECV (12) implement non-blocking
   * point-to-point message passing. Each channel has a queue with
   * capacity IPC_QUEUE_CAPACITY (16 messages).
   *
   * BACKPRESSURE BEHAVIOR:
   * - SYS_IPC_SEND: If queue is full (count >= 16), returns NS_ERR_WOULD_BLOCK
   *   Task must yield and retry. No silent drops; caller sees failure.
   *
   * - SYS_IPC_RECV: If queue is empty (count <= 0), returns NS_ERR_WOULD_BLOCK
   *   Task must yield and retry. No spinning; immediate return on empty.
   *
   * ORDERING GUARANTEE:
   * - Messages are FIFO within each channel
   * - If channel X receives msgs [A, B, C], they'll be received in order
   * - No reordering across tasks
   *
   * DETERMINISM:
   * - All IPC operations are serialized by scheduler_lock
   * - Overflow behavior is deterministic: reject or block, never corrupt
   * - Works across 3+ concurrent workloads without starvation
   * ============================================================ */

  case SYS_IPC_SEND: {
    int channel = (int)regs[4];
    int value = (int)regs[5];
    
    /* Validate channel number before use */
    if (channel < 0 || channel >= IPC_CHANNELS) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }
    
    if (ipc_send(channel, value)) {
      syscall_complete(regs, syscall_num, NS_OK);
    } else {
      syscall_complete(regs, syscall_num, NS_ERR_WOULD_BLOCK);
    }
    break;
  }

  case SYS_IPC_RECV: {
    int channel = (int)regs[4];
    int out = 0;
    int *user_out = (int *)regs[5];
    
    /* Validate channel number before use */
    if (channel < 0 || channel >= IPC_CHANNELS) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }
    
    /* Validate user output pointer if in Ring 3 */
    if (cpl == 3 && user_out != 0) {
      if (!validate_user_int_ptr(user_out)) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }
    }
    
    if (ipc_recv(channel, &out)) {
      if (user_out != 0 && cpl == 3) {
        *user_out = out;
      } else if (cpl != 3) {
        regs[5] = (uint32_t)out;
      }
      syscall_complete(regs, syscall_num, NS_OK);
    } else {
      syscall_complete(regs, syscall_num, NS_ERR_WOULD_BLOCK);
    }
    break;
  }

  default:
    syscall_complete(regs, syscall_num, NS_ERR_INVALID_SYSCALL);
    break;
  }
}