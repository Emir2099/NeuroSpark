/* syscall.c – NeuroSpark system call dispatcher */

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

#include "paging.h"
#include "task.h"
#include "scheduler.h"
#include "vfs.h"

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
#define SYS_OPEN 13
#define SYS_LSEEK 14
#define SYS_DUP 15
#define SYS_DUP2 16
#define SYS_FCNTL 17
#define SYS_WAIT4 18
#define SYS_WAITPID 19
#define SYS_SIGNAL 20
#define SYS_SIGACTION 21
#define SYS_SIGPROCMASK 22
#define SYS_KILL 23

#define SIGKILL 9
#define SIGTERM 15
#define WNOHANG 1

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
extern void enter_user_mode_retval(uint32_t entry_eip, uint32_t user_stack,
                                   uint32_t page_dir, uint32_t eax_ret);

typedef struct {
  uint32_t entry;
  uint32_t stack;
  uint32_t retval;
} ForkStartCtx;

static ForkStartCtx fork_start_ctx[MAX_TASKS];

static void fork_child_entry(void) {
  int tid = os_current_task;
  if (tid >= 0 && tid < MAX_TASKS) {
    enter_user_mode_retval(fork_start_ctx[tid].entry, fork_start_ctx[tid].stack,
                           os_tasks[tid].page_dir, fork_start_ctx[tid].retval);
  }
  task_terminate_current();
}

static int pick_unmasked_signal(uint32_t pending_mask, uint32_t blocked_mask) {
  uint32_t active = pending_mask & ~blocked_mask;
  for (int sig = 1; sig < TASK_SIGNAL_MAX; sig++) {
    if (active & (1u << sig)) {
      return sig;
    }
  }
  return 0;
}

static void syscall_dispatch_pending_signals(uint32_t *regs, uint32_t cpl) {
  TCB *task;
  int sig;
  uint32_t handler;
  uint32_t *iret_frame;

  if (cpl != 3 || os_current_task < 0 || os_current_task >= MAX_TASKS) {
    return;
  }

  task = &os_tasks[os_current_task];
  sig = pick_unmasked_signal(task->signal_pending, task->signal_mask);
  if (sig == 0) {
    return;
  }

  task->signal_pending &= ~(1u << sig);
  handler = task->signal_handler[sig];

  if (sig == SIGKILL || sig == SIGTERM || handler == 0 ||
      !is_user_range((const void *)handler, 1)) {
    task_exit_with_code(128 + sig);
    return;
  }

  iret_frame = (uint32_t *)((uint8_t *)regs + (8 * sizeof(uint32_t)));
  iret_frame[0] = handler; /* Override return EIP to signal handler. */
  regs[7] = (uint32_t)sig;
}

static int task_has_unreaped_child(int parent_pid) {
  for (int i = 0; i < MAX_TASKS; i++) {
    if (os_tasks[i].parent_pid == parent_pid && !os_tasks[i].waited) {
      return 1;
    }
  }
  return 0;
}

static int reap_child_for_parent(int parent_pid, int wanted_pid, int *status_out) {
  for (int i = 0; i < MAX_TASKS; i++) {
    if (i == 0) {
      continue;
    }
    if (os_tasks[i].parent_pid != parent_pid || os_tasks[i].waited || !os_tasks[i].exited) {
      continue;
    }
    if (wanted_pid > 0 && i != wanted_pid) {
      continue;
    }

    os_tasks[i].waited = 1;
    if (status_out != 0) {
      *status_out = os_tasks[i].exit_status;
    }
    return i;
  }
  return -1;
}

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

  /* ---- SYS_FORK (2): clone current user task and return child pid ---- */
  case SYS_FORK: {
    int parent = os_current_task;
    int child;
    uint32_t child_pd;
    uint32_t *iret_frame = (uint32_t *)((uint8_t *)regs + (8 * sizeof(uint32_t)));
    uint32_t ret_eip = iret_frame[0];
    uint32_t ret_esp = iret_frame[3];

    if (cpl != 3) {
      syscall_complete(regs, syscall_num, NS_ERR_PERMISSION);
      break;
    }

    child = task_alloc_slot();
    if (child < 0) {
      syscall_complete(regs, syscall_num, (uint32_t)VFS_ERR_NO_SPACE);
      break;
    }

    child_pd = clone_user_address_space(os_tasks[parent].page_dir);
    if (child_pd == 0) {
      syscall_complete(regs, syscall_num, (uint32_t)VFS_ERR_IO);
      break;
    }

    create_task(child, fork_child_entry, child_pd);
    os_tasks[child].parent_pid = parent;
    os_tasks[child].signal_mask = os_tasks[parent].signal_mask;
    for (int s = 0; s < TASK_SIGNAL_MAX; s++) {
      os_tasks[child].signal_handler[s] = os_tasks[parent].signal_handler[s];
    }

    fork_start_ctx[child].entry = ret_eip;
    fork_start_ctx[child].stack = ret_esp;
    fork_start_ctx[child].retval = 0;

    vfs_clone_task_fds(parent, child);
    syscall_complete(regs, syscall_num, (uint32_t)child);
    break;
  }

  /* ---- SYS_READ (3): read from VFS FD ---- */
  case SYS_READ:
    {
      int fd = (int)regs[4];
      void *buf = (void *)regs[6];
      uint32_t size = regs[5];
      int rc;
      if (cpl == 3 && !validate_user_ptr(buf, size)) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }
      rc = vfs_read(fd, buf, size);
      syscall_complete(regs, syscall_num, (uint32_t)rc);
    }
    break;

  /* ---- SYS_CLOSE (6): close a VFS FD ---- */
  case SYS_CLOSE:
    {
      int fd = (int)regs[4];
      int rc = vfs_close(fd);
      syscall_complete(regs, syscall_num, (uint32_t)rc);
    }
    break;

  /* ---- SYS_OPEN (13): open path with VFS flags ---- */
  case SYS_OPEN: {
    const char *path = (const char *)regs[4];
    int flags = (int)regs[6];
    int rc;
    if (cpl == 3 && !validate_user_string(path)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }
    rc = vfs_open(path, flags);
    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  /* ---- SYS_LSEEK (14): move read/write cursor ---- */
  case SYS_LSEEK: {
    int fd = (int)regs[4];
    int offset = (int)regs[6];
    int whence = (int)regs[5];
    int rc = vfs_lseek(fd, offset, whence);
    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  /* ---- SYS_DUP (15): duplicate file descriptor ---- */
  case SYS_DUP: {
    int fd = (int)regs[4];
    int rc = vfs_dup(fd);
    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  /* ---- SYS_DUP2 (16): duplicate into specific descriptor ---- */
  case SYS_DUP2: {
    int fd = (int)regs[4];
    int newfd = (int)regs[6];
    int rc = vfs_dup2(fd, newfd);
    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  /* ---- SYS_FCNTL (17): descriptor flag operations ---- */
  case SYS_FCNTL: {
    int fd = (int)regs[4];
    int cmd = (int)regs[6];
    int arg = (int)regs[5];
    int rc = vfs_fcntl(fd, cmd, arg);
    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  /* ---- SYS_WAIT4 (18): wait for child exit with optional WNOHANG ---- */
  case SYS_WAIT4:
  case SYS_WAITPID: {
    int wanted_pid = (int)regs[4];
    int *status_ptr = (int *)regs[6];
    int options = (int)regs[5];
    int status = 0;
    int child = -1;

    if (cpl == 3 && status_ptr != 0 && !validate_user_int_ptr(status_ptr)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    while (1) {
      child = reap_child_for_parent(os_current_task, wanted_pid, &status);
      if (child >= 0) {
        if (status_ptr != 0) {
          *status_ptr = status;
        }
        syscall_complete(regs, syscall_num, (uint32_t)child);
        break;
      }

      if (!task_has_unreaped_child(os_current_task)) {
        syscall_complete(regs, syscall_num, NS_ERR_INVALID_SYSCALL);
        break;
      }

      if (options & WNOHANG) {
        syscall_complete(regs, syscall_num, 0);
        break;
      }

      task_sleep(1);
    }
    break;
  }

  /* ---- SYS_SIGNAL (20): register basic signal handler ---- */
  case SYS_SIGNAL:
  case SYS_SIGACTION: {
    int sig = (int)regs[4];
    uint32_t handler = regs[6];
    uint32_t old;

    if (sig <= 0 || sig >= TASK_SIGNAL_MAX || sig == SIGKILL) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    if (cpl == 3 && handler != 0 && !is_user_range((const void *)handler, 1)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    old = os_tasks[os_current_task].signal_handler[sig];
    os_tasks[os_current_task].signal_handler[sig] = handler;
    syscall_complete(regs, syscall_num, old);
    break;
  }

  /* ---- SYS_SIGPROCMASK (22): set/block/unblock signal mask ---- */
  case SYS_SIGPROCMASK: {
    int how = (int)regs[4];
    uint32_t set_mask = regs[6];
    uint32_t *old_mask_ptr = (uint32_t *)regs[5];
    uint32_t old_mask = os_tasks[os_current_task].signal_mask;

    if (cpl == 3 && old_mask_ptr != 0 && !validate_user_ptr(old_mask_ptr, sizeof(uint32_t))) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    if (how == 0) {
      os_tasks[os_current_task].signal_mask = set_mask;
    } else if (how == 1) {
      os_tasks[os_current_task].signal_mask |= set_mask;
    } else if (how == 2) {
      os_tasks[os_current_task].signal_mask &= ~set_mask;
    } else {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    if (old_mask_ptr != 0) {
      *old_mask_ptr = old_mask;
    }
    syscall_complete(regs, syscall_num, NS_OK);
    break;
  }

  /* ---- SYS_KILL (23): queue signal to target process ---- */
  case SYS_KILL: {
    int pid = (int)regs[4];
    int sig = (int)regs[6];

    if (pid < 0 || pid >= MAX_TASKS || sig <= 0 || sig >= TASK_SIGNAL_MAX) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    if (sig == SIGKILL) {
      int rc = task_kill(pid, 128 + SIGKILL);
      syscall_complete(regs, syscall_num, rc ? NS_OK : NS_ERR_INVALID_SYSCALL);
    } else {
      os_tasks[pid].signal_pending |= (1u << sig);
      syscall_complete(regs, syscall_num, NS_OK);
    }
    break;
  }

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

  /* ---- SYS_WRITE (4): write to FD or fallback to legacy gprint ---- */
  case SYS_WRITE: {
    int fd = (int)regs[4];
    const void *buf = (const void *)regs[6];
    uint32_t size = regs[5];
    int wrote = VFS_ERR_INVALID_ARG;

    if (fd >= 0 && buf != 0 && size > 0) {
      if (cpl == 3 && !validate_user_ptr(buf, size)) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }
      wrote = vfs_write(fd, buf, size);
      if (wrote >= 0) {
        syscall_complete(regs, syscall_num, (uint32_t)wrote);
        break;
      }
    }

    {
      char *msg = (char *)regs[4]; /* EBX */
      if (cpl == 3 && !validate_user_string(msg)) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }
      gprint(msg, 0x00FF00);
      syscall_complete(regs, syscall_num, NS_OK);
    }
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

  syscall_dispatch_pending_signals(regs, cpl);
}