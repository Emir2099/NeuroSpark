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
#define SYS_STAT 24
#define SYS_FSTAT 25
#define SYS_LSTAT 26
#define SYS_ACCESS 27
#define SYS_CHMOD 28
#define SYS_CHOWN 29
#define SYS_UNLINK 30
#define SYS_RENAME 31
#define SYS_MKDIR 32
#define SYS_RMDIR 33
#define SYS_TRUNCATE 34
#define SYS_FTRUNCATE 35
#define SYS_LINK 36
#define SYS_SYMLINK 37
#define SYS_READLINK 38
#define SYS_GETPID 39
#define SYS_GETPPID 40
#define SYS_GETUID 41
#define SYS_GETEUID 42
#define SYS_GETGID 43
#define SYS_GETEGID 44
#define SYS_SETUID 45
#define SYS_SETGID 46
#define SYS_MMAP 47
#define SYS_MUNMAP 48
#define SYS_MPROTECT 49
#define SYS_BRK 50
#define SYS_SBRK 51
#define SYS_TIME 52
#define SYS_GETTIMEOFDAY 53
#define SYS_CLOCK_GETTIME 54
#define SYS_NANOSLEEP 55
#define SYS_ALARM 56
#define SYS_TIMER_CREATE 57
#define SYS_PIPE 58
#define SYS_IOCTL 59
#define SYS_GETRUSAGE 60
#define SYS_UNAME 61

#define SIGKILL 9
#define SIGTERM 15
#define WNOHANG 1

#define NS_ERR_NOT_IMPLEMENTED ((uint32_t)-38)

#define MMAP_ANON_BASE 0x50000000u
#define MMAP_ANON_LIMIT 0xB0000000u
#define BRK_DEFAULT_BASE 0x48000000u
#define BRK_DEFAULT_LIMIT 0x4F000000u

#define PAGE_SIZE_BYTES 4096u
#define PAGE_MASK_BYTES 0xFFFFF000u

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1

#define PROT_WRITE 0x2

#define NS_BOOT_EPOCH_SECONDS 1767225600u /* 2026-01-01T00:00:00Z */
#define NS_TICKS_PER_SECOND 100u
#define NS_NSEC_PER_SEC 1000000000u
#define NS_USEC_PER_SEC 1000000u

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
extern void *pmm_alloc_page(void);
extern void pmm_free_page(uint32_t page_addr);

typedef struct {
  uint32_t st_size;
  uint32_t st_flags;
} NsStat;

typedef struct {
  uint32_t tv_sec;
  uint32_t tv_usec;
} NsTimeval;

typedef struct {
  uint32_t tv_sec;
  uint32_t tv_nsec;
} NsTimespec;

typedef struct {
  NsTimeval ru_utime;
  NsTimeval ru_stime;
  uint32_t ru_maxrss;
  uint32_t ru_nvcsw;
} NsRusage;

typedef struct {
  char sysname[32];
  char nodename[32];
  char release[32];
  char version[32];
  char machine[32];
} NsUtsname;

typedef struct {
  uint32_t entry;
  uint32_t stack;
  uint32_t retval;
} ForkStartCtx;

static ForkStartCtx fork_start_ctx[MAX_TASKS];
static uint32_t task_uid[MAX_TASKS];
static uint32_t task_gid[MAX_TASKS];
static uint32_t task_brk_base[MAX_TASKS];
static uint32_t task_brk_current[MAX_TASKS];
static uint32_t task_mmap_next[MAX_TASKS];
static uint32_t task_alarm_tick[MAX_TASKS];

static void mem_zero32(void *dst, uint32_t size) {
  uint8_t *d = (uint8_t *)dst;
  for (uint32_t i = 0; i < size; i++) {
    d[i] = 0;
  }
}

static void copy_text32(char *dst, uint32_t cap, const char *src) {
  uint32_t i = 0;
  if (cap == 0) {
    return;
  }
  while (i + 1 < cap && src[i] != '\0') {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';
}

static uint32_t align_up_page(uint32_t value) {
  return (value + (PAGE_SIZE_BYTES - 1u)) & PAGE_MASK_BYTES;
}

static uint32_t page_floor(uint32_t value) {
  return value & PAGE_MASK_BYTES;
}

static void ensure_task_identity_defaults(int task_id) {
  if (task_id < 0 || task_id >= MAX_TASKS) {
    return;
  }
  if (task_brk_base[task_id] == 0) {
    task_uid[task_id] = 0;
    task_gid[task_id] = 0;
    task_brk_base[task_id] = BRK_DEFAULT_BASE;
    task_brk_current[task_id] = BRK_DEFAULT_BASE;
    task_mmap_next[task_id] = MMAP_ANON_BASE;
    task_alarm_tick[task_id] = 0;
  }
}

static int map_user_pages_for_range(uint32_t pd, uint32_t start, uint32_t length) {
  uint32_t first;
  uint32_t end;

  if (length == 0) {
    return 1;
  }

  first = page_floor(start);
  end = page_floor(start + length - 1u);
  for (uint32_t va = first;; va += PAGE_SIZE_BYTES) {
    uint32_t phys;
    if (resolve_user_phys(pd, va) != 0) {
      if (va >= end) {
        break;
      }
      continue;
    }

    phys = (uint32_t)pmm_alloc_page();
    if (phys == 0) {
      return 0;
    }
    if (!map_user_page(pd, phys, va)) {
      pmm_free_page(phys & PAGE_MASK_BYTES);
      return 0;
    }

    if (va >= end) {
      break;
    }
  }

  return 1;
}

static int unmap_user_pages_for_range(uint32_t pd, uint32_t start, uint32_t length) {
  uint32_t first;
  uint32_t end;

  if (length == 0) {
    return 1;
  }

  first = page_floor(start);
  end = page_floor(start + length - 1u);
  for (uint32_t va = first;; va += PAGE_SIZE_BYTES) {
    if (!unmap_user_page(pd, va)) {
      return 0;
    }
    if (va >= end) {
      break;
    }
  }

  return 1;
}

static int set_user_writable_for_range(uint32_t pd, uint32_t start, uint32_t length,
                                       int writable) {
  uint32_t first;
  uint32_t end;

  if (length == 0) {
    return 1;
  }

  first = page_floor(start);
  end = page_floor(start + length - 1u);
  for (uint32_t va = first;; va += PAGE_SIZE_BYTES) {
    if (!set_user_page_writable(pd, va, writable)) {
      return 0;
    }
    if (va >= end) {
      break;
    }
  }

  return 1;
}

static void realtime_now(uint32_t *sec_out, uint32_t *nsec_out) {
  uint32_t ticks = scheduler_now_ticks();
  uint32_t sec = ticks / NS_TICKS_PER_SECOND;
  uint32_t rem_ticks = ticks % NS_TICKS_PER_SECOND;
  uint32_t nsec = rem_ticks * (NS_NSEC_PER_SEC / NS_TICKS_PER_SECOND);
  *sec_out = NS_BOOT_EPOCH_SECONDS + sec;
  *nsec_out = nsec;
}

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

    ensure_task_identity_defaults(parent);
    ensure_task_identity_defaults(child);
    task_uid[child] = task_uid[parent];
    task_gid[child] = task_gid[parent];
    task_brk_base[child] = task_brk_base[parent];
    task_brk_current[child] = task_brk_current[parent];
    task_mmap_next[child] = task_mmap_next[parent];
    task_alarm_tick[child] = task_alarm_tick[parent];

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

  case SYS_STAT:
  case SYS_LSTAT: {
    const char *path = (const char *)regs[4];
    NsStat *user_stat = (NsStat *)regs[6];
    VfsFileStat st;
    int rc;

    if (cpl == 3) {
      if (!validate_user_string(path) || !validate_user_ptr(user_stat, sizeof(NsStat))) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }
    }

    rc = vfs_stat(path, &st);
    if (rc < 0) {
      syscall_complete(regs, syscall_num, (uint32_t)rc);
      break;
    }

    user_stat->st_size = st.size;
    user_stat->st_flags = st.flags;
    syscall_complete(regs, syscall_num, NS_OK);
    break;
  }

  case SYS_FSTAT: {
    int fd = (int)regs[4];
    NsStat *user_stat = (NsStat *)regs[6];
    int end;
    int cur;

    if (cpl == 3 && !validate_user_ptr(user_stat, sizeof(NsStat))) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    cur = vfs_lseek(fd, 0, VFS_SEEK_CUR);
    end = vfs_lseek(fd, 0, VFS_SEEK_END);
    if (cur < 0 || end < 0) {
      syscall_complete(regs, syscall_num, NS_ERR_NOT_IMPLEMENTED);
      break;
    }
    vfs_lseek(fd, cur, VFS_SEEK_SET);

    user_stat->st_size = (uint32_t)end;
    user_stat->st_flags = 1;
    syscall_complete(regs, syscall_num, NS_OK);
    break;
  }

  case SYS_ACCESS: {
    const char *path = (const char *)regs[4];
    VfsFileStat st;
    int rc;

    if (cpl == 3 && !validate_user_string(path)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    rc = vfs_stat(path, &st);
    syscall_complete(regs, syscall_num, rc < 0 ? (uint32_t)rc : NS_OK);
    break;
  }

  case SYS_LINK: {
    const char *old_path = (const char *)regs[4];
    const char *new_path = (const char *)regs[6];
    int rc;

    if (cpl == 3 && (!validate_user_string(old_path) || !validate_user_string(new_path))) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    rc = vfs_link(old_path, new_path);
    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  case SYS_SYMLINK: {
    const char *target = (const char *)regs[4];
    const char *link_path = (const char *)regs[6];
    int rc;

    if (cpl == 3 && (!validate_user_string(target) || !validate_user_string(link_path))) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    rc = vfs_symlink(target, link_path);
    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  case SYS_READLINK: {
    const char *link_path = (const char *)regs[4];
    char *target_buf = (char *)regs[6];
    uint32_t max_len = regs[5];
    int rc;

    if (cpl == 3 && (!validate_user_string(link_path) || !validate_user_ptr(target_buf, max_len))) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    rc = vfs_readlink(link_path, target_buf, max_len);
    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  case SYS_IOCTL:
  case SYS_TIMER_CREATE:
    syscall_complete(regs, syscall_num, NS_ERR_NOT_IMPLEMENTED);
    break;

  case SYS_CHMOD: {
    const char *path = (const char *)regs[4];
    int mode = (int)regs[6];
    int rc;

    if (cpl == 3 && !validate_user_string(path)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    rc = vfs_chmod(path, mode);
    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  case SYS_CHOWN: {
    const char *path = (const char *)regs[4];
    int uid = (int)regs[6];
    int gid = (int)regs[5];
    int rc;

    if (cpl == 3 && !validate_user_string(path)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    rc = vfs_chown(path, uid, gid);
    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  case SYS_PIPE: {
    int *pipefd = (int *)regs[4];
    int rfd = -1;
    int wfd = -1;
    int rc;

    if (cpl == 3 && !validate_user_ptr(pipefd, sizeof(int) * 2u)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    rc = vfs_pipe(&rfd, &wfd);
    if (rc < 0) {
      syscall_complete(regs, syscall_num, (uint32_t)rc);
      break;
    }

    pipefd[0] = rfd;
    pipefd[1] = wfd;
    syscall_complete(regs, syscall_num, NS_OK);
    break;
  }

  case SYS_RENAME: {
    const char *old_path = (const char *)regs[4];
    const char *new_path = (const char *)regs[6];
    int rc;

    if (cpl == 3 && (!validate_user_string(old_path) || !validate_user_string(new_path))) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    rc = vfs_rename(old_path, new_path);
    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  case SYS_MKDIR: {
    const char *path = (const char *)regs[4];
    int mode = (int)regs[6];
    int rc;

    if (cpl == 3 && !validate_user_string(path)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    rc = vfs_mkdir(path, mode);
    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  case SYS_RMDIR: {
    const char *path = (const char *)regs[4];
    int rc;

    if (cpl == 3 && !validate_user_string(path)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    rc = vfs_rmdir(path);
    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  case SYS_TRUNCATE: {
    const char *path = (const char *)regs[4];
    uint32_t size = regs[6];
    int rc;

    if (cpl == 3 && !validate_user_string(path)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    rc = vfs_truncate(path, size);
    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  case SYS_FTRUNCATE: {
    int fd = (int)regs[4];
    uint32_t size = regs[6];
    int rc = vfs_ftruncate(fd, size);
    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  case SYS_UNLINK: {
    const char *path = (const char *)regs[4];
    int rc;

    if (cpl == 3 && !validate_user_string(path)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    rc = vfs_delete(path);
    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  case SYS_GETPID:
    syscall_complete(regs, syscall_num, (uint32_t)os_current_task);
    break;

  case SYS_GETPPID:
    syscall_complete(regs, syscall_num, (uint32_t)os_tasks[os_current_task].parent_pid);
    break;

  case SYS_GETUID:
  case SYS_GETEUID:
    ensure_task_identity_defaults(os_current_task);
    syscall_complete(regs, syscall_num, task_uid[os_current_task]);
    break;

  case SYS_GETGID:
  case SYS_GETEGID:
    ensure_task_identity_defaults(os_current_task);
    syscall_complete(regs, syscall_num, task_gid[os_current_task]);
    break;

  case SYS_SETUID: {
    uint32_t uid = regs[4];
    ensure_task_identity_defaults(os_current_task);
    if (task_uid[os_current_task] != 0) {
      syscall_complete(regs, syscall_num, NS_ERR_PERMISSION);
      break;
    }
    task_uid[os_current_task] = uid;
    syscall_complete(regs, syscall_num, NS_OK);
    break;
  }

  case SYS_SETGID: {
    uint32_t gid = regs[4];
    ensure_task_identity_defaults(os_current_task);
    if (task_uid[os_current_task] != 0) {
      syscall_complete(regs, syscall_num, NS_ERR_PERMISSION);
      break;
    }
    task_gid[os_current_task] = gid;
    syscall_complete(regs, syscall_num, NS_OK);
    break;
  }

  case SYS_MMAP: {
    uint32_t addr = regs[4];
    uint32_t len = regs[6];
    int prot = (int)regs[5];
    uint32_t base;
    uint32_t mapped;
    uint32_t pd;

    ensure_task_identity_defaults(os_current_task);
    if (len == 0) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    mapped = align_up_page(len);
    if (addr == 0) {
      base = align_up_page(task_mmap_next[os_current_task]);
    } else {
      base = page_floor(addr);
    }

    if (base < MMAP_ANON_BASE || (base + mapped) > MMAP_ANON_LIMIT ||
        (base + mapped) < base) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    pd = current_task_page_dir();
    if (!map_user_pages_for_range(pd, base, mapped)) {
      syscall_complete(regs, syscall_num, (uint32_t)VFS_ERR_NO_SPACE);
      break;
    }

    if (!set_user_writable_for_range(pd, base, mapped, (prot & PROT_WRITE) != 0)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    if (addr == 0) {
      task_mmap_next[os_current_task] = base + mapped;
    }
    syscall_complete(regs, syscall_num, base);
    break;
  }

  case SYS_MUNMAP: {
    uint32_t addr = regs[4];
    uint32_t len = regs[6];
    uint32_t base;
    uint32_t mapped;
    uint32_t pd;

    if (len == 0) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    base = page_floor(addr);
    mapped = align_up_page(len);
    if (base < MMAP_ANON_BASE || (base + mapped) > MMAP_ANON_LIMIT ||
        (base + mapped) < base) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    pd = current_task_page_dir();
    if (!unmap_user_pages_for_range(pd, base, mapped)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    syscall_complete(regs, syscall_num, NS_OK);
    break;
  }

  case SYS_MPROTECT: {
    uint32_t addr = regs[4];
    uint32_t len = regs[6];
    int prot = (int)regs[5];
    uint32_t base;
    uint32_t mapped;
    uint32_t pd;

    if (len == 0) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    base = page_floor(addr);
    mapped = align_up_page(len);
    pd = current_task_page_dir();
    if (!set_user_writable_for_range(pd, base, mapped, (prot & PROT_WRITE) != 0)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    syscall_complete(regs, syscall_num, NS_OK);
    break;
  }

  case SYS_BRK: {
    uint32_t requested = regs[4];
    uint32_t current;
    uint32_t pd;

    ensure_task_identity_defaults(os_current_task);
    current = task_brk_current[os_current_task];
    if (requested == 0) {
      syscall_complete(regs, syscall_num, current);
      break;
    }

    if (requested < task_brk_base[os_current_task] || requested > BRK_DEFAULT_LIMIT) {
      syscall_complete(regs, syscall_num, current);
      break;
    }

    pd = current_task_page_dir();
    if (requested > current) {
      uint32_t delta = requested - current;
      if (!map_user_pages_for_range(pd, current, delta)) {
        syscall_complete(regs, syscall_num, current);
        break;
      }
    } else if (requested < current) {
      uint32_t delta = current - requested;
      if (!unmap_user_pages_for_range(pd, requested, delta)) {
        syscall_complete(regs, syscall_num, current);
        break;
      }
    }

    task_brk_current[os_current_task] = requested;
    syscall_complete(regs, syscall_num, requested);
    break;
  }

  case SYS_SBRK: {
    int increment = (int)regs[4];
    uint32_t current;
    uint32_t target;

    ensure_task_identity_defaults(os_current_task);
    current = task_brk_current[os_current_task];
    target = current + (uint32_t)increment;
    if ((increment > 0 && target < current) || (increment < 0 && target > current)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    regs[4] = target;
    syscall_num = SYS_BRK;
    regs[7] = syscall_num;
    /* Tail-call brk behavior to keep one growth/shrink path. */
    {
      uint32_t requested = regs[4];
      uint32_t pd;
      if (requested < task_brk_base[os_current_task] || requested > BRK_DEFAULT_LIMIT) {
        syscall_complete(regs, SYS_SBRK, NS_ERR_BAD_POINTER);
        break;
      }
      pd = current_task_page_dir();
      if (requested > current) {
        if (!map_user_pages_for_range(pd, current, requested - current)) {
          syscall_complete(regs, SYS_SBRK, NS_ERR_BAD_POINTER);
          break;
        }
      } else if (requested < current) {
        if (!unmap_user_pages_for_range(pd, requested, current - requested)) {
          syscall_complete(regs, SYS_SBRK, NS_ERR_BAD_POINTER);
          break;
        }
      }
      task_brk_current[os_current_task] = requested;
      syscall_complete(regs, SYS_SBRK, current);
    }
    break;
  }

  case SYS_TIME: {
    uint32_t *time_out = (uint32_t *)regs[4];
    uint32_t sec;
    uint32_t nsec;
    realtime_now(&sec, &nsec);

    if (time_out != 0) {
      if (cpl == 3 && !validate_user_ptr(time_out, sizeof(uint32_t))) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }
      *time_out = sec;
    }
    syscall_complete(regs, syscall_num, sec);
    break;
  }

  case SYS_GETTIMEOFDAY: {
    NsTimeval *tv = (NsTimeval *)regs[4];
    uint32_t sec;
    uint32_t nsec;

    if (cpl == 3 && !validate_user_ptr(tv, sizeof(NsTimeval))) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    realtime_now(&sec, &nsec);
    tv->tv_sec = sec;
    tv->tv_usec = nsec / 1000u;
    syscall_complete(regs, syscall_num, NS_OK);
    break;
  }

  case SYS_CLOCK_GETTIME: {
    int clock_id = (int)regs[4];
    NsTimespec *ts = (NsTimespec *)regs[6];

    if (cpl == 3 && !validate_user_ptr(ts, sizeof(NsTimespec))) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    if (clock_id == CLOCK_REALTIME) {
      realtime_now(&ts->tv_sec, &ts->tv_nsec);
      syscall_complete(regs, syscall_num, NS_OK);
      break;
    }
    if (clock_id == CLOCK_MONOTONIC) {
      uint32_t ticks = scheduler_now_ticks();
      ts->tv_sec = ticks / NS_TICKS_PER_SECOND;
      ts->tv_nsec = (ticks % NS_TICKS_PER_SECOND) * (NS_NSEC_PER_SEC / NS_TICKS_PER_SECOND);
      syscall_complete(regs, syscall_num, NS_OK);
      break;
    }

    syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
    break;
  }

  case SYS_NANOSLEEP: {
    const NsTimespec *req = (const NsTimespec *)regs[4];
    NsTimespec *rem = (NsTimespec *)regs[6];
    uint32_t total_ns;
    uint32_t ticks;

    if (cpl == 3 && !validate_user_ptr(req, sizeof(NsTimespec))) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }
    if (req->tv_nsec >= NS_NSEC_PER_SEC) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    total_ns = req->tv_sec * NS_NSEC_PER_SEC + req->tv_nsec;
    ticks = (total_ns + (NS_NSEC_PER_SEC / NS_TICKS_PER_SECOND) - 1u) /
            (NS_NSEC_PER_SEC / NS_TICKS_PER_SECOND);
    if (ticks == 0) {
      ticks = 1;
    }

    if (rem != 0) {
      if (cpl == 3 && !validate_user_ptr(rem, sizeof(NsTimespec))) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }
      rem->tv_sec = 0;
      rem->tv_nsec = 0;
    }

    task_sleep(ticks);
    syscall_complete(regs, syscall_num, NS_OK);
    break;
  }

  case SYS_ALARM: {
    uint32_t seconds = regs[4];
    uint32_t now = scheduler_now_ticks();
    uint32_t prev = 0;

    ensure_task_identity_defaults(os_current_task);
    if (task_alarm_tick[os_current_task] > now) {
      prev = (task_alarm_tick[os_current_task] - now + (NS_TICKS_PER_SECOND - 1u)) /
             NS_TICKS_PER_SECOND;
    }

    if (seconds == 0) {
      task_alarm_tick[os_current_task] = 0;
    } else {
      task_alarm_tick[os_current_task] = now + (seconds * NS_TICKS_PER_SECOND);
    }
    syscall_complete(regs, syscall_num, prev);
    break;
  }

  case SYS_GETRUSAGE: {
    int who = (int)regs[4];
    NsRusage *usage = (NsRusage *)regs[6];
    uint32_t ticks;

    if (who != 0) {
      syscall_complete(regs, syscall_num, NS_ERR_NOT_IMPLEMENTED);
      break;
    }
    if (cpl == 3 && !validate_user_ptr(usage, sizeof(NsRusage))) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    mem_zero32(usage, sizeof(NsRusage));
    ticks = os_tasks[os_current_task].runtime_ticks;
    usage->ru_utime.tv_sec = ticks / NS_TICKS_PER_SECOND;
    usage->ru_utime.tv_usec = (ticks % NS_TICKS_PER_SECOND) * (NS_USEC_PER_SEC / NS_TICKS_PER_SECOND);
    usage->ru_nvcsw = os_tasks[os_current_task].context_switches;
    syscall_complete(regs, syscall_num, NS_OK);
    break;
  }

  case SYS_UNAME: {
    NsUtsname *u = (NsUtsname *)regs[4];
    if (cpl == 3 && !validate_user_ptr(u, sizeof(NsUtsname))) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    mem_zero32(u, sizeof(NsUtsname));
    copy_text32(u->sysname, sizeof(u->sysname), "NeuroSpark");
    copy_text32(u->nodename, sizeof(u->nodename), "neurospark-host");
    copy_text32(u->release, sizeof(u->release), "0.4.0");
    copy_text32(u->version, sizeof(u->version), "Phase22");
    copy_text32(u->machine, sizeof(u->machine), "i686");
    syscall_complete(regs, syscall_num, NS_OK);
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