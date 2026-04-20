/* syscall.c – NeuroSpark system call dispatcher */

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

#include "paging.h"
#include "task.h"
#include "scheduler.h"
#include "vfs.h"
#include "net.h"
#include "disk.h"
#include "module_loader.h"
#include "posix.h"

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
#define SYS_SOCKET 62
#define SYS_BIND 63
#define SYS_LISTEN 64
#define SYS_ACCEPT 65
#define SYS_CONNECT 66
#define SYS_SENDTO 67
#define SYS_RECVFROM 68
#define SYS_SELECT 69
#define SYS_POLL 70
#define SYS_EPOLL 71
#define SYS_DLOPEN 72
#define SYS_DLSYM 73
#define SYS_INSMOD 74
#define SYS_RMMOD 75
#define SYS_DLCLOSE 76
#define SYS_DLERROR 77
#define SYS_TCSETPGRP 78
#define SYS_TCGETPGRP 79
#define SYS_IO_SETUP 80
#define SYS_IO_SUBMIT 81
#define SYS_IO_GETEVENTS 82
#define SYS_TRACE 83
#define SYS_PTRACE 84
#define SYS_SETSOCKOPT 85
#define SYS_GETSOCKOPT 86
#define SYS_SHUTDOWN 87

#define SIGKILL 9
#define SIGINT 2
#define SIGCONT 18
#define SIGSTOP 19
#define SIGTSTP 20
#define SIGTERM 15
#define WNOHANG 1

#define NS_ERR_NOT_IMPLEMENTED ((uint32_t)-38)

#define NS_ERR_INVALID_ARG ((uint32_t)-22)

#define NS_SOCKET_FD_BASE 1024
#define NS_SOCKET_MAX_PER_TASK 64

#define NS_O_NONBLOCK 0x800

#define NS_POLLIN 0x0001u
#define NS_POLLOUT 0x0004u
#define NS_POLLERR 0x0008u

#define NS_EPOLL_OP_CREATE 1
#define NS_EPOLL_OP_CTL 2
#define NS_EPOLL_OP_WAIT 3
#define NS_EPOLL_OP_CLOSE 4

#define NS_EPOLL_CTL_ADD 1
#define NS_EPOLL_CTL_MOD 2
#define NS_EPOLL_CTL_DEL 3

#define NS_EPOLL_FD_BASE 2048
#define NS_EPOLL_MAX_PER_TASK 8
#define NS_EPOLL_MAX_WATCH 64

#define NS_AF_INET 2
#define NS_SOCK_STREAM 1
#define NS_SOCK_DGRAM 2

#define NS_SOL_SOCKET 1
#define NS_SO_REUSEADDR 2
#define NS_SO_RCVTIMEO 3
#define NS_SO_SNDTIMEO 4
#define NS_SO_KEEPALIVE 5

#define NS_SHUT_RD 0
#define NS_SHUT_WR 1
#define NS_SHUT_RDWR 2

#define MMAP_ANON_BASE 0x50000000u
#define MMAP_ANON_LIMIT 0xB0000000u
#define BRK_DEFAULT_BASE 0x48000000u
#define BRK_DEFAULT_LIMIT 0x4F000000u

#define PAGE_SIZE_BYTES 4096u
#define PAGE_MASK_BYTES 0xFFFFF000u

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1

#define PROT_WRITE 0x2

#define NS_IOCTL_DISK_GET_GEOMETRY 0x1001
#define NS_IOCTL_DISK_GET_HEALTH 0x1002
#define NS_IOCTL_DISK_SET_BACKEND 0x1003
#define NS_IOCTL_DISK_GET_BACKEND 0x1004
#define NS_IOCTL_DISK_RESET_IO_STATS 0x1005
#define NS_IOCTL_DISK_GET_IO_STATS 0x1006

#define NS_IOCTL_NIC_GET_INFO 0x2001
#define NS_IOCTL_NIC_SET_MAC 0x2002

#define NS_AIO_MAX_CTX 4
#define NS_AIO_MAX_REQ 64
#define NS_AIO_OP_READ 0u
#define NS_AIO_OP_WRITE 1u

#define NS_PTRACE_GET_INFO 0u
#define NS_PTRACE_GET_STACK 1u
#define NS_PTRACE_SET_TRACE 2u
#define NS_PTRACE_SET_PRIORITY 3u

#define NS_PTRACE_STACK_DEPTH 6u

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
  uint32_t total_sectors_low;
  uint32_t total_sectors_high;
  uint32_t sector_size;
  uint32_t backend;
  char model[41];
} NsDiskGeometry;

typedef struct {
  uint32_t available;
  uint32_t backend;
  uint32_t preferred_backend;
  uint32_t ahci_ready_ports;
  uint32_t smart_supported;
  uint32_t smart_enabled;
  uint32_t temperature_c;
  uint32_t life_percent;
  uint32_t read_error_count;
  uint32_t write_error_count;
  uint32_t last_error_code;
} NsDiskHealth;

typedef struct {
  uint32_t uncached_reads;
  uint32_t uncached_writes;
  uint32_t cached_reads;
  uint32_t cached_writes;
} NsDiskIoStats;

typedef struct {
  uint32_t ready;
  uint32_t nic_index;
  uint32_t io_base;
  uint32_t link_mbps;
  uint8_t mac[6];
  char driver[16];
} NsNicInfo;

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
static uint32_t task_brk_base[MAX_TASKS];
static uint32_t task_brk_current[MAX_TASKS];
static uint32_t task_mmap_next[MAX_TASKS];
static uint32_t task_alarm_tick[MAX_TASKS];

typedef struct {
  uint16_t sin_family;
  uint16_t sin_port;
  uint32_t sin_addr;
} NsSockAddrIn;

typedef struct {
  int used;
  int type;
  int protocol;
  int flags;
  int bound;
  uint16_t local_port;
  uint32_t peer_ip;
  uint16_t peer_port;
  int listening;
  int net_conn_id;
  uint32_t rcv_timeout_ticks;
  uint32_t snd_timeout_ticks;
  uint8_t reuse_addr;
  uint8_t keepalive;
  uint8_t shut_rd;
  uint8_t shut_wr;
} NsSocketEntry;

typedef struct {
  int fd;
  uint16_t events;
  uint16_t revents;
} NsPollFd;

typedef struct {
  int op;
  NsPollFd event;
} NsEpollCtlReq;

typedef struct {
  NsPollFd *events;
  uint32_t maxevents;
  uint32_t timeout_ticks;
} NsEpollWaitReq;

typedef struct {
  int used;
  int fd;
  uint16_t events;
} NsEpollWatch;

typedef struct {
  int used;
  NsEpollWatch watches[NS_EPOLL_MAX_WATCH];
} NsEpollObject;

typedef struct {
  uint32_t opcode;
  uint32_t lba;
  uint32_t buf;
  uint32_t nbytes;
  uint32_t user_data;
} NsIoCb;

typedef struct {
  uint32_t task_id;
  uint32_t state;
  uint32_t priority;
  uint32_t workload_class;
  uint32_t runtime_ticks;
  uint32_t context_switches;
  uint32_t trace_enabled;
  uint32_t trace_event_count;
  uint32_t trace_last_event;
  uint32_t trace_last_arg;
  uint32_t trace_last_syscall;
  uint32_t trace_last_result;
  uint32_t trace_last_arg0;
  uint32_t trace_last_arg1;
  uint32_t trace_last_arg2;
  uint32_t esp;
  uint32_t ebp;
  uint32_t eip;
  uint32_t fault_code;
  uint32_t fault_addr;
  uint32_t fault_eip;
  uint32_t wake_tick;
  uint32_t wait_reason;
  uint32_t sched_wait_ticks;
  uint32_t sched_wake_boost;
  uint32_t sched_last_run_tick;
} NsPtraceInfo;

typedef struct {
  uint32_t task_id;
  uint32_t eip;
  uint32_t ebp;
  uint32_t depth;
  uint32_t frames[NS_PTRACE_STACK_DEPTH];
} NsPtraceStack;

typedef struct {
  uint32_t user_data;
  int result;
} NsIoEvent;

typedef struct {
  int used;
  uint32_t opcode;
  uint32_t lba;
  uint32_t user_buf;
  uint32_t nbytes;
  uint32_t user_data;
  int result;
  uint16_t sector[256];
} NsAioReq;

typedef struct {
  int used;
  int owner_task;
  uint32_t maxevents;
  uint32_t req_count;
  NsAioReq reqs[NS_AIO_MAX_REQ];
} NsAioCtx;

static NsSocketEntry task_sockets[MAX_TASKS][NS_SOCKET_MAX_PER_TASK];
static NsEpollObject task_epolls[MAX_TASKS][NS_EPOLL_MAX_PER_TASK];
static NsAioCtx aio_contexts[NS_AIO_MAX_CTX];

static int validate_user_ptr(const void *ptr, uint32_t size);

static uint16_t ns_bswap16(uint16_t v) {
  return (uint16_t)((v << 8) | (v >> 8));
}

static uint32_t ns_bswap32(uint32_t v) {
  return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8) |
         ((v & 0x00FF0000u) >> 8) | ((v & 0xFF000000u) >> 24);
}

static int socket_fd_to_slot(int fd) {
  int slot = fd - NS_SOCKET_FD_BASE;
  if (slot < 0 || slot >= NS_SOCKET_MAX_PER_TASK) {
    return -1;
  }
  return slot;
}

static NsSocketEntry *socket_entry_for_fd(int task_id, int fd) {
  int slot;
  if (task_id < 0 || task_id >= MAX_TASKS) {
    return 0;
  }
  slot = socket_fd_to_slot(fd);
  if (slot < 0 || !task_sockets[task_id][slot].used) {
    return 0;
  }
  return &task_sockets[task_id][slot];
}

static int socket_alloc_fd(int task_id, int type, int protocol) {
  if (task_id < 0 || task_id >= MAX_TASKS) {
    return -1;
  }
  for (int i = 0; i < NS_SOCKET_MAX_PER_TASK; i++) {
    if (!task_sockets[task_id][i].used) {
      task_sockets[task_id][i].used = 1;
      task_sockets[task_id][i].type = type;
      task_sockets[task_id][i].protocol = protocol;
      task_sockets[task_id][i].flags = 0;
      task_sockets[task_id][i].bound = 0;
      task_sockets[task_id][i].local_port = 0;
      task_sockets[task_id][i].peer_ip = 0;
      task_sockets[task_id][i].peer_port = 0;
      task_sockets[task_id][i].listening = 0;
      task_sockets[task_id][i].net_conn_id = -1;
      task_sockets[task_id][i].rcv_timeout_ticks = 0xFFFFFFFFu;
      task_sockets[task_id][i].snd_timeout_ticks = 0xFFFFFFFFu;
      task_sockets[task_id][i].reuse_addr = 0;
      task_sockets[task_id][i].keepalive = 0;
      task_sockets[task_id][i].shut_rd = 0;
      task_sockets[task_id][i].shut_wr = 0;
      return NS_SOCKET_FD_BASE + i;
    }
  }
  return -1;
}

static void socket_close_entry(NsSocketEntry *s) {
  if (s == 0 || !s->used) {
    return;
  }
  if (s->type == NS_SOCK_DGRAM && s->bound) {
    (void)net_udp_unbind(s->local_port);
  }
  if (s->type == NS_SOCK_STREAM) {
    if (s->listening && s->bound) {
      (void)net_tcp_unlisten(s->local_port);
    } else if (s->net_conn_id >= 0) {
      (void)net_tcp_close(s->net_conn_id);
    }
  }
  s->used = 0;
  s->type = 0;
  s->protocol = 0;
  s->flags = 0;
  s->bound = 0;
  s->local_port = 0;
  s->peer_ip = 0;
  s->peer_port = 0;
  s->listening = 0;
  s->net_conn_id = -1;
  s->rcv_timeout_ticks = 0xFFFFFFFFu;
  s->snd_timeout_ticks = 0xFFFFFFFFu;
  s->reuse_addr = 0;
  s->keepalive = 0;
  s->shut_rd = 0;
  s->shut_wr = 0;
}

static int socket_is_nonblocking(const NsSocketEntry *s) {
  return s != 0 && (s->flags & NS_O_NONBLOCK);
}

static int socket_ready_for_read(const NsSocketEntry *s) {
  if (s == 0 || !s->used) {
    return 0;
  }
  if (s->shut_rd) {
    return 0;
  }
  if (s->type == NS_SOCK_DGRAM) {
    return s->bound && net_udp_has_data(s->local_port);
  }
  if (s->type == NS_SOCK_STREAM) {
    if (s->listening) {
      return net_tcp_accept_ready(s->local_port);
    }
    return s->net_conn_id >= 0 && net_tcp_can_recv(s->net_conn_id);
  }
  return 0;
}

static int socket_ready_for_write(const NsSocketEntry *s) {
  if (s == 0 || !s->used) {
    return 0;
  }
  if (s->shut_wr) {
    return 0;
  }
  if (s->type == NS_SOCK_DGRAM) {
    return s->peer_ip != 0u && s->peer_port != 0u;
  }
  if (s->type == NS_SOCK_STREAM) {
    return s->net_conn_id >= 0 && net_tcp_can_send(s->net_conn_id);
  }
  return 0;
}

static int socket_wait_for_readable(const NsSocketEntry *s, uint32_t timeout_ticks) {
  uint32_t deadline = 0;
  int infinite = (timeout_ticks == 0xFFFFFFFFu);

  if (!infinite) {
    deadline = scheduler_now_ticks() + timeout_ticks;
  }

  while (!socket_ready_for_read(s)) {
    if (!infinite && scheduler_now_ticks() >= deadline) {
      return 0;
    }
    task_sleep(1);
  }
  return 1;
}

static int socket_wait_for_writable(const NsSocketEntry *s, uint32_t timeout_ticks) {
  uint32_t deadline = 0;
  int infinite = (timeout_ticks == 0xFFFFFFFFu);

  if (!infinite) {
    deadline = scheduler_now_ticks() + timeout_ticks;
  }

  while (!socket_ready_for_write(s)) {
    if (!infinite && scheduler_now_ticks() >= deadline) {
      return 0;
    }
    task_sleep(1);
  }
  return 1;
}

static int epoll_fd_to_slot(int fd) {
  int slot = fd - NS_EPOLL_FD_BASE;

  if (slot < 0 || slot >= NS_EPOLL_MAX_PER_TASK) {
    return -1;
  }
  return slot;
}

static NsEpollObject *epoll_object_for_fd(int task_id, int fd) {
  int slot;

  if (task_id < 0 || task_id >= MAX_TASKS) {
    return 0;
  }
  slot = epoll_fd_to_slot(fd);
  if (slot < 0 || !task_epolls[task_id][slot].used) {
    return 0;
  }
  return &task_epolls[task_id][slot];
}

static int epoll_alloc_fd(int task_id) {
  if (task_id < 0 || task_id >= MAX_TASKS) {
    return -1;
  }
  for (int i = 0; i < NS_EPOLL_MAX_PER_TASK; i++) {
    if (!task_epolls[task_id][i].used) {
      task_epolls[task_id][i].used = 1;
      for (int j = 0; j < NS_EPOLL_MAX_WATCH; j++) {
        task_epolls[task_id][i].watches[j].used = 0;
        task_epolls[task_id][i].watches[j].fd = -1;
        task_epolls[task_id][i].watches[j].events = 0;
      }
      return NS_EPOLL_FD_BASE + i;
    }
  }
  return -1;
}

static void epoll_close_object(NsEpollObject *obj) {
  if (obj == 0 || !obj->used) {
    return;
  }
  obj->used = 0;
  for (int i = 0; i < NS_EPOLL_MAX_WATCH; i++) {
    obj->watches[i].used = 0;
    obj->watches[i].fd = -1;
    obj->watches[i].events = 0;
  }
}

static void epoll_close_all_for_task(int task_id) {
  if (task_id < 0 || task_id >= MAX_TASKS) {
    return;
  }
  for (int i = 0; i < NS_EPOLL_MAX_PER_TASK; i++) {
    if (task_epolls[task_id][i].used) {
      epoll_close_object(&task_epolls[task_id][i]);
    }
  }
}

static int aio_ctx_handle_to_slot(uint32_t handle) {
  int slot = (int)handle - 1;
  if (slot < 0 || slot >= NS_AIO_MAX_CTX) {
    return -1;
  }
  return slot;
}

static NsAioCtx *aio_ctx_from_handle(uint32_t handle) {
  int slot = aio_ctx_handle_to_slot(handle);
  if (slot < 0) {
    return 0;
  }
  if (!aio_contexts[slot].used || aio_contexts[slot].owner_task != os_current_task) {
    return 0;
  }
  return &aio_contexts[slot];
}

static uint32_t aio_alloc_context(uint32_t maxevents) {
  for (int i = 0; i < NS_AIO_MAX_CTX; i++) {
    if (!aio_contexts[i].used) {
      aio_contexts[i].used = 1;
      aio_contexts[i].owner_task = os_current_task;
      aio_contexts[i].maxevents = maxevents;
      aio_contexts[i].req_count = 0;
      for (int r = 0; r < NS_AIO_MAX_REQ; r++) {
        aio_contexts[i].reqs[r].used = 0;
        aio_contexts[i].reqs[r].result = NS_ERR_WOULD_BLOCK;
      }
      return (uint32_t)(i + 1);
    }
  }
  return 0;
}

static int aio_submit_one(NsAioCtx *ctx, const NsIoCb *cb) {
  NsAioReq *req;

  if (ctx == 0 || cb == 0 || ctx->req_count >= NS_AIO_MAX_REQ) {
    return 0;
  }
  if ((cb->opcode != NS_AIO_OP_READ && cb->opcode != NS_AIO_OP_WRITE) ||
      cb->nbytes != 512u) {
    return 0;
  }
  if (!validate_user_ptr((const void *)cb->buf, cb->nbytes)) {
    return 0;
  }

  req = &ctx->reqs[ctx->req_count];
  req->used = 1;
  req->opcode = cb->opcode;
  req->lba = cb->lba;
  req->user_buf = cb->buf;
  req->nbytes = cb->nbytes;
  req->user_data = cb->user_data;
  req->result = NS_ERR_WOULD_BLOCK;

  if (cb->opcode == NS_AIO_OP_WRITE) {
    const uint8_t *src = (const uint8_t *)(cb->buf);
    uint8_t *dst = (uint8_t *)req->sector;
    for (uint32_t i = 0; i < cb->nbytes; i++) {
      dst[i] = src[i];
    }
  }

  ctx->req_count++;
  return 1;
}

static int ptrace_task_allowed(uint32_t cpl, int task_id) {
  if (task_id < 0 || task_id >= MAX_TASKS) {
    return 0;
  }
  if (cpl != 3u) {
    return 1;
  }
  return task_id == os_current_task;
}

static void ptrace_fill_info(int task_id, NsPtraceInfo *info) {
  TCB *task;

  if (info == 0 || task_id < 0 || task_id >= MAX_TASKS) {
    return;
  }

  task = &os_tasks[task_id];
  info->task_id = (uint32_t)task_id;
  info->state = task->state;
  info->priority = task->priority;
  info->workload_class = task->workload_class;
  info->runtime_ticks = task->runtime_ticks;
  info->context_switches = task->context_switches;
  info->trace_enabled = task->trace_enabled;
  info->trace_event_count = task->trace_event_count;
  info->trace_last_event = task->trace_last_event;
  info->trace_last_arg = task->trace_last_arg;
  info->trace_last_syscall = task->trace_last_syscall;
  info->trace_last_result = task->trace_last_result;
  info->trace_last_arg0 = task->trace_last_arg0;
  info->trace_last_arg1 = task->trace_last_arg1;
  info->trace_last_arg2 = task->trace_last_arg2;
  info->esp = task->esp;
  info->ebp = task->ebp;
  info->eip = task->eip;
  info->fault_code = task->fault_code;
  info->fault_addr = task->fault_addr;
  info->fault_eip = task->fault_eip;
  info->wake_tick = task->wake_tick;
  info->wait_reason = task->wait_reason;
  info->sched_wait_ticks = task->sched_wait_ticks;
  info->sched_wake_boost = task->sched_wake_boost;
  info->sched_last_run_tick = task->sched_last_run_tick;
}

static void ptrace_fill_stack(int task_id, NsPtraceStack *stack) {
  uint32_t *frame;

  if (stack == 0 || task_id < 0 || task_id >= MAX_TASKS) {
    return;
  }

  stack->task_id = (uint32_t)task_id;
  stack->eip = os_tasks[task_id].eip;
  stack->ebp = os_tasks[task_id].ebp;
  stack->depth = 0;
  for (uint32_t i = 0; i < NS_PTRACE_STACK_DEPTH; i++) {
    stack->frames[i] = 0;
  }

  frame = (uint32_t *)os_tasks[task_id].ebp;
  while (frame != 0 && stack->depth < NS_PTRACE_STACK_DEPTH) {
    uint32_t next = frame[0];
    uint32_t ret = frame[1];

    stack->frames[stack->depth++] = ret;
    if (next == 0u || next <= (uint32_t)frame || (next - (uint32_t)frame) > 0x4000u) {
      break;
    }
    frame = (uint32_t *)next;
  }
}

static int aio_process_events(NsAioCtx *ctx, NsIoEvent *events, uint32_t maxevents) {
  DiskBatchRequest batch[NS_AIO_MAX_REQ];
  uint32_t to_process;

  if (ctx == 0 || events == 0 || maxevents == 0u || ctx->req_count == 0u) {
    return 0;
  }

  to_process = ctx->req_count;
  if (to_process > maxevents) {
    to_process = maxevents;
  }

  for (uint32_t i = 0; i < to_process; i++) {
    batch[i].lba = ctx->reqs[i].lba;
    batch[i].buffer = ctx->reqs[i].sector;
    batch[i].write = (uint8_t)(ctx->reqs[i].opcode == NS_AIO_OP_WRITE);
    batch[i].result = NS_ERR_WOULD_BLOCK;
  }

  (void)disk_process_batch(batch, to_process);

  for (uint32_t i = 0; i < to_process; i++) {
    NsAioReq *req = &ctx->reqs[i];
    req->result = batch[i].result;
    if (req->result == DISK_IO_OK && req->opcode == NS_AIO_OP_READ) {
      uint8_t *dst = (uint8_t *)req->user_buf;
      uint8_t *src = (uint8_t *)req->sector;
      for (uint32_t k = 0; k < req->nbytes; k++) {
        dst[k] = src[k];
      }
    }
    events[i].user_data = req->user_data;
    events[i].result = req->result;
  }

  for (uint32_t i = to_process; i < ctx->req_count; i++) {
    ctx->reqs[i - to_process] = ctx->reqs[i];
  }
  ctx->req_count -= to_process;

  return (int)to_process;
}

static int fd_poll_revents(int fd, uint16_t events, uint16_t *revents_out) {
  NsSocketEntry *s = socket_entry_for_fd(os_current_task, fd);
  uint16_t revents = 0;

  if (s != 0) {
    if (socket_ready_for_read(s) && (events & NS_POLLIN) != 0u) {
      revents |= NS_POLLIN;
    }
    if (socket_ready_for_write(s) && (events & NS_POLLOUT) != 0u) {
      revents |= NS_POLLOUT;
    }
    if (revents_out != 0) {
      *revents_out = revents;
    }
    return revents != 0u;
  }

  if (vfs_fcntl(fd, VFS_F_GETFL, 0) >= 0) {
    if ((events & NS_POLLIN) != 0u) {
      revents |= NS_POLLIN;
    }
    if ((events & NS_POLLOUT) != 0u) {
      revents |= NS_POLLOUT;
    }
    if (revents_out != 0) {
      *revents_out = revents;
    }
    return revents != 0u;
  }

  if (revents_out != 0) {
    *revents_out = NS_POLLERR;
  }
  return 0;
}

static int poll_socket_array(NsPollFd *fds, uint32_t count) {
  int ready = 0;

  for (uint32_t i = 0; i < count; i++) {
    fds[i].revents = 0;
    (void)fd_poll_revents(fds[i].fd, fds[i].events, &fds[i].revents);
    if (fds[i].revents != 0u) {
      ready++;
    }
  }

  return ready;
}

static int poll_socket_array_wait(NsPollFd *fds, uint32_t count, uint32_t timeout_ticks) {
  uint32_t deadline = 0;

  if (timeout_ticks != 0u) {
    deadline = scheduler_now_ticks() + timeout_ticks;
  }

  while (1) {
    int ready = poll_socket_array(fds, count);
    if (ready > 0 || timeout_ticks == 0u) {
      return ready;
    }
    if (scheduler_now_ticks() >= deadline) {
      return 0;
    }
    task_sleep(1);
  }
}

static int epoll_ctl_apply(NsEpollObject *obj, const NsEpollCtlReq *req) {
  int free_slot = -1;
  int found_slot = -1;

  if (obj == 0 || req == 0 || req->event.fd < 0) {
    return (int)NS_ERR_INVALID_ARG;
  }

  for (int i = 0; i < NS_EPOLL_MAX_WATCH; i++) {
    if (!obj->watches[i].used && free_slot < 0) {
      free_slot = i;
    }
    if (obj->watches[i].used && obj->watches[i].fd == req->event.fd) {
      found_slot = i;
      break;
    }
  }

  if (req->op == NS_EPOLL_CTL_ADD) {
    if (found_slot >= 0) {
      return (int)NS_ERR_INVALID_ARG;
    }
    if (free_slot < 0) {
      return (int)VFS_ERR_NO_SPACE;
    }
    obj->watches[free_slot].used = 1;
    obj->watches[free_slot].fd = req->event.fd;
    obj->watches[free_slot].events = req->event.events;
    return NS_OK;
  }

  if (req->op == NS_EPOLL_CTL_MOD) {
    if (found_slot < 0) {
      return (int)NS_ERR_INVALID_ARG;
    }
    obj->watches[found_slot].events = req->event.events;
    return NS_OK;
  }

  if (req->op == NS_EPOLL_CTL_DEL) {
    if (found_slot < 0) {
      return (int)NS_ERR_INVALID_ARG;
    }
    obj->watches[found_slot].used = 0;
    obj->watches[found_slot].fd = -1;
    obj->watches[found_slot].events = 0;
    return NS_OK;
  }

  return (int)NS_ERR_INVALID_ARG;
}

static int epoll_collect_ready(NsEpollObject *obj, NsPollFd *out, uint32_t maxevents) {
  int ready = 0;

  if (obj == 0 || out == 0 || maxevents == 0u) {
    return 0;
  }

  for (int i = 0; i < NS_EPOLL_MAX_WATCH; i++) {
    uint16_t revents = 0;

    if (!obj->watches[i].used) {
      continue;
    }
    (void)fd_poll_revents(obj->watches[i].fd, obj->watches[i].events, &revents);
    if (revents == 0u) {
      continue;
    }

    out[ready].fd = obj->watches[i].fd;
    out[ready].events = obj->watches[i].events;
    out[ready].revents = revents;
    ready++;
    if ((uint32_t)ready >= maxevents) {
      break;
    }
  }

  return ready;
}

static int epoll_wait_ready(NsEpollObject *obj, NsPollFd *out, uint32_t maxevents,
                            uint32_t timeout_ticks) {
  uint32_t deadline = 0;
  int infinite = (timeout_ticks == 0xFFFFFFFFu);

  if (!infinite && timeout_ticks != 0u) {
    deadline = scheduler_now_ticks() + timeout_ticks;
  }

  while (1) {
    int ready = epoll_collect_ready(obj, out, maxevents);
    if (ready > 0 || timeout_ticks == 0u) {
      return ready;
    }
    if (!infinite && scheduler_now_ticks() >= deadline) {
      return 0;
    }
    task_sleep(1);
  }
}

static void socket_close_all_for_task(int task_id) {
  if (task_id < 0 || task_id >= MAX_TASKS) {
    return;
  }
  for (int i = 0; i < NS_SOCKET_MAX_PER_TASK; i++) {
    if (task_sockets[task_id][i].used) {
      socket_close_entry(&task_sockets[task_id][i]);
    }
  }
}

static void aio_close_all_for_task(int task_id) {
  if (task_id < 0 || task_id >= MAX_TASKS) {
    return;
  }
  for (int i = 0; i < NS_AIO_MAX_CTX; i++) {
    if (aio_contexts[i].used && aio_contexts[i].owner_task == task_id) {
      aio_contexts[i].used = 0;
      aio_contexts[i].owner_task = -1;
      aio_contexts[i].maxevents = 0;
      aio_contexts[i].req_count = 0;
    }
  }
}

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
    posix_task_init(task_id, -1);
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

  if (sig == SIGCONT) {
    if (task->state == TASK_STATE_BLOCKED && task->wait_reason == TASK_WAIT_NONE) {
      task->state = TASK_STATE_READY;
      task->wake_tick = 0;
    }
    return;
  }

  if (sig == SIGSTOP || sig == SIGTSTP) {
    task->state = TASK_STATE_BLOCKED;
    task->wake_tick = 0;
    task->wait_reason = TASK_WAIT_NONE;
    return;
  }

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
  task_trace_syscall_ex(os_current_task, syscall_num, result, regs[4], regs[5], regs[6]);
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
    posix_task_on_fork(parent, child);
    task_brk_base[child] = task_brk_base[parent];
    task_brk_current[child] = task_brk_current[parent];
    task_mmap_next[child] = task_mmap_next[parent];
    task_alarm_tick[child] = task_alarm_tick[parent];

    socket_close_all_for_task(child);
    epoll_close_all_for_task(child);

    vfs_clone_task_fds(parent, child);
    posix_clone_fd_rights(parent, child);
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
      NsSocketEntry *s;
      if (cpl == 3 && !validate_user_ptr(buf, size)) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }
      s = socket_entry_for_fd(os_current_task, fd);
      if (s != 0) {
        if (s->shut_rd) {
          syscall_complete(regs, syscall_num, (uint32_t)NS_ERR_INVALID_ARG);
          break;
        }
        if (!socket_ready_for_read(s)) {
          if (socket_is_nonblocking(s)) {
            syscall_complete(regs, syscall_num, NS_ERR_WOULD_BLOCK);
            break;
          }
          if (!socket_wait_for_readable(s, s->rcv_timeout_ticks)) {
            syscall_complete(regs, syscall_num, NS_ERR_WOULD_BLOCK);
            break;
          }
        }
        if (s->type == NS_SOCK_STREAM && s->net_conn_id >= 0) {
          rc = net_tcp_recv(s->net_conn_id, buf, (uint16_t)size);
        } else if (s->type == NS_SOCK_DGRAM && s->bound) {
          rc = net_udp_recv(s->local_port, 0, 0, buf, (uint16_t)size);
        } else {
          rc = (int)NS_ERR_INVALID_ARG;
        }
        if (rc == 0) {
          rc = (int)NS_ERR_WOULD_BLOCK;
        }
      } else {
        if (posix_check_fd_permission(os_current_task, fd, POSIX_ACC_READ) != VFS_OK) {
          syscall_complete(regs, syscall_num, NS_ERR_PERMISSION);
          break;
        }
        rc = vfs_read(fd, buf, size);
      }
      syscall_complete(regs, syscall_num, (uint32_t)rc);
    }
    break;

  /* ---- SYS_CLOSE (6): close a VFS FD ---- */
  case SYS_CLOSE:
    {
      int fd = (int)regs[4];
      int rc;
      NsSocketEntry *s = socket_entry_for_fd(os_current_task, fd);
      NsEpollObject *ep = epoll_object_for_fd(os_current_task, fd);
      if (s != 0) {
        socket_close_entry(s);
        rc = NS_OK;
      } else if (ep != 0) {
        epoll_close_object(ep);
        rc = NS_OK;
      } else {
        rc = vfs_close(fd);
        if (rc == VFS_OK) {
          posix_track_fd_close(os_current_task, fd);
        }
      }
      syscall_complete(regs, syscall_num, (uint32_t)rc);
    }
    break;

  /* ---- SYS_OPEN (13): open path with VFS flags ---- */
  case SYS_OPEN: {
    const char *path = (const char *)regs[4];
    int flags = (int)regs[6];
    int rc;
    int access_mask = 0;
    if (cpl == 3 && !validate_user_string(path)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }
    if ((flags & VFS_O_WRONLY) != 0) {
      access_mask = POSIX_ACC_WRITE;
    } else if ((flags & VFS_O_RDWR) == VFS_O_RDWR) {
      access_mask = POSIX_ACC_READ | POSIX_ACC_WRITE;
    } else {
      access_mask = POSIX_ACC_READ;
    }
    if (posix_check_path_permission(os_current_task, path, access_mask) != VFS_OK) {
      syscall_complete(regs, syscall_num, NS_ERR_PERMISSION);
      break;
    }
    rc = vfs_open(path, flags);
    if (rc >= 0) {
      posix_track_fd_open(os_current_task, rc, path, flags);
    }
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
    NsSocketEntry *s = socket_entry_for_fd(os_current_task, fd);
    int rc;

    if (s != 0) {
      if (cmd == VFS_F_GETFL) {
        rc = s->flags;
      } else if (cmd == VFS_F_SETFL) {
        s->flags = arg;
        rc = NS_OK;
      } else {
        rc = NS_ERR_INVALID_ARG;
      }
    } else {
      rc = vfs_fcntl(fd, cmd, arg);
    }
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
    socket_close_all_for_task(os_current_task);
    epoll_close_all_for_task(os_current_task);
    aio_close_all_for_task(os_current_task);
    posix_clear_task_fds(os_current_task);
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

  case SYS_IOCTL: {
    int fd = (int)regs[4];
    int cmd = (int)regs[5];
    uint32_t arg = regs[6];
    (void)fd;

    if (cmd == NS_IOCTL_DISK_GET_GEOMETRY) {
      NsDiskGeometry *out = (NsDiskGeometry *)arg;
      DiskGeometryInfo info;

      if (out == 0) {
        syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
        break;
      }
      if (cpl == 3 && !validate_user_ptr(out, sizeof(NsDiskGeometry))) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }

      (void)disk_get_geometry(&info);
      out->total_sectors_low = info.total_sectors_low;
      out->total_sectors_high = info.total_sectors_high;
      out->sector_size = info.sector_size;
      out->backend = info.backend;
      copy_text32(out->model, sizeof(out->model), info.model);
      syscall_complete(regs, syscall_num, NS_OK);
      break;
    }

    if (cmd == NS_IOCTL_DISK_GET_HEALTH) {
      NsDiskHealth *out = (NsDiskHealth *)arg;
      DiskHealthInfo info;

      if (out == 0) {
        syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
        break;
      }
      if (cpl == 3 && !validate_user_ptr(out, sizeof(NsDiskHealth))) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }

      (void)disk_get_health(&info);
      out->available = info.available;
      out->backend = info.backend;
      out->preferred_backend = info.preferred_backend;
      out->ahci_ready_ports = info.ahci_ready_ports;
      out->smart_supported = info.smart_supported;
      out->smart_enabled = info.smart_enabled;
      out->temperature_c = info.temperature_c;
      out->life_percent = info.life_percent;
      out->read_error_count = info.read_error_count;
      out->write_error_count = info.write_error_count;
      out->last_error_code = info.last_error_code;
      syscall_complete(regs, syscall_num, NS_OK);
      break;
    }

    if (cmd == NS_IOCTL_DISK_SET_BACKEND) {
      uint8_t backend = (uint8_t)arg;
      disk_set_preferred_backend(backend);
      syscall_complete(regs, syscall_num, NS_OK);
      break;
    }

    if (cmd == NS_IOCTL_DISK_GET_BACKEND) {
      syscall_complete(regs, syscall_num, (uint32_t)disk_get_preferred_backend());
      break;
    }

    if (cmd == NS_IOCTL_DISK_RESET_IO_STATS) {
      disk_reset_io_stats();
      syscall_complete(regs, syscall_num, NS_OK);
      break;
    }

    if (cmd == NS_IOCTL_DISK_GET_IO_STATS) {
      NsDiskIoStats *out = (NsDiskIoStats *)arg;
      DiskIoStats info;

      if (out == 0) {
        syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
        break;
      }
      if (cpl == 3 && !validate_user_ptr(out, sizeof(NsDiskIoStats))) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }

      disk_get_io_stats(&info);
      out->uncached_reads = info.uncached_reads;
      out->uncached_writes = info.uncached_writes;
      out->cached_reads = info.cached_reads;
      out->cached_writes = info.cached_writes;
      syscall_complete(regs, syscall_num, NS_OK);
      break;
    }

    if (cmd == NS_IOCTL_NIC_GET_INFO) {
      NsNicInfo *out = (NsNicInfo *)arg;

      if (out == 0) {
        syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
        break;
      }
      if (cpl == 3 && !validate_user_ptr(out, sizeof(NsNicInfo))) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }

      out->ready = net_is_ready() ? 1u : 0u;
      out->nic_index = (uint32_t)net_nic_index();
      out->io_base = net_nic_io_base();
      out->link_mbps = (uint32_t)net_link_speed_mbps();
      net_get_mac(out->mac);
      copy_text32(out->driver, sizeof(out->driver), net_driver_name());
      syscall_complete(regs, syscall_num, NS_OK);
      break;
    }

    if (cmd == NS_IOCTL_NIC_SET_MAC) {
      uint8_t *mac = (uint8_t *)arg;

      if (mac == 0) {
        syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
        break;
      }
      if (cpl == 3 && !validate_user_ptr(mac, 6)) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }
      syscall_complete(regs, syscall_num,
                       net_set_mac(mac) ? NS_OK : NS_ERR_INVALID_SYSCALL);
      break;
    }

    syscall_complete(regs, syscall_num, NS_ERR_NOT_IMPLEMENTED);
    break;
  }

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

    rc = posix_path_chmod(os_current_task, path, mode);
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

    rc = posix_path_chown(os_current_task, path, uid, gid);
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

    if (posix_check_path_permission(os_current_task, path, POSIX_ACC_WRITE) != VFS_OK) {
      syscall_complete(regs, syscall_num, NS_ERR_PERMISSION);
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
    ensure_task_identity_defaults(os_current_task);
    syscall_complete(regs, syscall_num, posix_get_uid(os_current_task));
    break;

  case SYS_GETEUID:
    ensure_task_identity_defaults(os_current_task);
    syscall_complete(regs, syscall_num, posix_get_euid(os_current_task));
    break;

  case SYS_GETGID:
    ensure_task_identity_defaults(os_current_task);
    syscall_complete(regs, syscall_num, posix_get_gid(os_current_task));
    break;

  case SYS_GETEGID:
    ensure_task_identity_defaults(os_current_task);
    syscall_complete(regs, syscall_num, posix_get_egid(os_current_task));
    break;

  case SYS_SETUID: {
    uint32_t uid = regs[4];
    int rc;
    ensure_task_identity_defaults(os_current_task);
    rc = posix_set_uid(os_current_task, uid);
    syscall_complete(regs, syscall_num, rc == VFS_OK ? NS_OK : NS_ERR_PERMISSION);
    break;
  }

  case SYS_SETGID: {
    uint32_t gid = regs[4];
    int rc;
    ensure_task_identity_defaults(os_current_task);
    rc = posix_set_gid(os_current_task, gid);
    syscall_complete(regs, syscall_num, rc == VFS_OK ? NS_OK : NS_ERR_PERMISSION);
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

  case SYS_SOCKET: {
    int domain = (int)regs[4];
    int type = (int)regs[5];
    int protocol = (int)regs[6];
    int fd;

    if (domain != NS_AF_INET || (type != NS_SOCK_STREAM && type != NS_SOCK_DGRAM)) {
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }

    fd = socket_alloc_fd(os_current_task, type, protocol);
    if (fd < 0) {
      syscall_complete(regs, syscall_num, (uint32_t)VFS_ERR_NO_SPACE);
      break;
    }
    syscall_complete(regs, syscall_num, (uint32_t)fd);
    break;
  }

  case SYS_BIND: {
    int fd = (int)regs[4];
    const NsSockAddrIn *addr = (const NsSockAddrIn *)regs[6];
    NsSocketEntry *s;
    uint16_t port;
    int rc;

    if (cpl == 3 && !validate_user_ptr(addr, sizeof(NsSockAddrIn))) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    s = socket_entry_for_fd(os_current_task, fd);
    if (s == 0 || addr == 0 || addr->sin_family != NS_AF_INET) {
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }

    port = ns_bswap16(addr->sin_port);
    if (port == 0u) {
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }

    if (s->type == NS_SOCK_DGRAM) {
      rc = net_udp_bind(port) ? (int)NS_OK : (int)VFS_ERR_NO_SPACE;
      if (rc == NS_OK) {
        s->bound = 1;
        s->local_port = port;
      }
    } else {
      s->bound = 1;
      s->local_port = port;
      rc = NS_OK;
    }

    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  case SYS_LISTEN: {
    int fd = (int)regs[4];
    NsSocketEntry *s = socket_entry_for_fd(os_current_task, fd);
    int conn_id;

    if (s == 0 || s->type != NS_SOCK_STREAM || !s->bound) {
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }

    conn_id = net_tcp_listen(s->local_port);
    if (conn_id < 0) {
      syscall_complete(regs, syscall_num, (uint32_t)VFS_ERR_NO_SPACE);
      break;
    }

    s->listening = 1;
    s->net_conn_id = conn_id;
    syscall_complete(regs, syscall_num, NS_OK);
    break;
  }

  case SYS_ACCEPT: {
    int fd = (int)regs[4];
    NsSockAddrIn *peer = (NsSockAddrIn *)regs[6];
    NsSocketEntry *s = socket_entry_for_fd(os_current_task, fd);
    uint32_t peer_ip = 0;
    uint16_t peer_port = 0;
    int conn_id;
    int newfd;
    NsSocketEntry *child;

    if (cpl == 3 && peer != 0 && !validate_user_ptr(peer, sizeof(NsSockAddrIn))) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }
    if (s == 0 || s->type != NS_SOCK_STREAM || !s->listening) {
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }

    if (!socket_ready_for_read(s)) {
      if (socket_is_nonblocking(s)) {
        syscall_complete(regs, syscall_num, NS_ERR_WOULD_BLOCK);
        break;
      }
      if (!socket_wait_for_readable(s, s->rcv_timeout_ticks)) {
        syscall_complete(regs, syscall_num, NS_ERR_WOULD_BLOCK);
        break;
      }
    }

    conn_id = net_tcp_accept(s->local_port, &peer_ip, &peer_port);
    if (conn_id < 0) {
      syscall_complete(regs, syscall_num, NS_ERR_WOULD_BLOCK);
      break;
    }

    newfd = socket_alloc_fd(os_current_task, NS_SOCK_STREAM, 0);
    if (newfd < 0) {
      (void)net_tcp_close(conn_id);
      syscall_complete(regs, syscall_num, (uint32_t)VFS_ERR_NO_SPACE);
      break;
    }

    child = socket_entry_for_fd(os_current_task, newfd);
    child->bound = 1;
    child->local_port = s->local_port;
    child->peer_ip = peer_ip;
    child->peer_port = peer_port;
    child->listening = 0;
    child->net_conn_id = conn_id;
    child->flags = s->flags;
    child->rcv_timeout_ticks = s->rcv_timeout_ticks;
    child->snd_timeout_ticks = s->snd_timeout_ticks;
    child->reuse_addr = s->reuse_addr;
    child->keepalive = s->keepalive;
    child->shut_rd = 0;
    child->shut_wr = 0;

    if (peer != 0) {
      peer->sin_family = NS_AF_INET;
      peer->sin_port = ns_bswap16(peer_port);
      peer->sin_addr = ns_bswap32(peer_ip);
    }

    syscall_complete(regs, syscall_num, (uint32_t)newfd);
    break;
  }

  case SYS_CONNECT: {
    int fd = (int)regs[4];
    const NsSockAddrIn *addr = (const NsSockAddrIn *)regs[6];
    NsSocketEntry *s = socket_entry_for_fd(os_current_task, fd);
    uint16_t port;
    uint32_t ip;
    int conn_id;

    if (cpl == 3 && !validate_user_ptr(addr, sizeof(NsSockAddrIn))) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }
    if (s == 0 || addr == 0 || addr->sin_family != NS_AF_INET) {
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }

    port = ns_bswap16(addr->sin_port);
    ip = ns_bswap32(addr->sin_addr);
    if (port == 0u || ip == 0u) {
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }

    if (s->type == NS_SOCK_DGRAM) {
      s->peer_ip = ip;
      s->peer_port = port;
      syscall_complete(regs, syscall_num, NS_OK);
      break;
    }

    if (!s->bound) {
      s->bound = 1;
      s->local_port = (uint16_t)(40000 + (os_current_task & 0x7F));
    }

    conn_id = net_tcp_connect(ip, port, s->local_port);
    if (conn_id < 0) {
      syscall_complete(regs, syscall_num, NS_ERR_WOULD_BLOCK);
      break;
    }

    s->peer_ip = ip;
    s->peer_port = port;
    s->net_conn_id = conn_id;
    s->listening = 0;
    if (socket_is_nonblocking(s)) {
      syscall_complete(regs, syscall_num, NS_ERR_WOULD_BLOCK);
      break;
    }

    {
      uint32_t deadline = 0;
      int infinite = (s->snd_timeout_ticks == 0xFFFFFFFFu);
      if (!infinite) {
        deadline = scheduler_now_ticks() + s->snd_timeout_ticks;
      }
      while (!net_tcp_is_established(conn_id)) {
        if (!infinite && scheduler_now_ticks() >= deadline) {
          syscall_complete(regs, syscall_num, NS_ERR_WOULD_BLOCK);
          break;
        }
        task_sleep(1);
      }
      if (!net_tcp_is_established(conn_id)) {
        break;
      }
    }
    syscall_complete(regs, syscall_num, NS_OK);
    break;
  }

  case SYS_SENDTO: {
    int fd = (int)regs[4];
    const void *buf = (const void *)regs[6];
    uint32_t len = regs[5];
    NsSocketEntry *s = socket_entry_for_fd(os_current_task, fd);
    int rc = (int)NS_ERR_INVALID_ARG;

    if (cpl == 3 && !validate_user_ptr(buf, len)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }
    if (s == 0 || buf == 0 || len == 0u) {
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }
    if (s->shut_wr) {
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }

    if (s->type == NS_SOCK_DGRAM) {
      if (s->peer_ip != 0 && s->peer_port != 0) {
        rc = net_udp_send(s->peer_ip,
                          s->bound ? s->local_port : 40000u,
                          s->peer_port,
                          buf,
                          (uint16_t)len);
      }
      rc = rc ? (int)len : (int)NS_ERR_WOULD_BLOCK;
    } else if (s->type == NS_SOCK_STREAM && s->net_conn_id >= 0) {
      if (!socket_ready_for_write(s)) {
        if (socket_is_nonblocking(s)) {
          syscall_complete(regs, syscall_num, NS_ERR_WOULD_BLOCK);
          break;
        }
        if (!socket_wait_for_writable(s, s->snd_timeout_ticks)) {
          syscall_complete(regs, syscall_num, NS_ERR_WOULD_BLOCK);
          break;
        }
      }
      rc = net_tcp_send(s->net_conn_id, buf, (uint16_t)len);
    }

    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  case SYS_RECVFROM: {
    int fd = (int)regs[4];
    void *buf = (void *)regs[6];
    uint32_t len = regs[5];
    NsSocketEntry *s = socket_entry_for_fd(os_current_task, fd);
    int rc = (int)NS_ERR_INVALID_ARG;

    if (cpl == 3 && !validate_user_ptr(buf, len)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }
    if (s == 0 || buf == 0 || len == 0u) {
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }
    if (s->shut_rd) {
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }

    if (s->type == NS_SOCK_DGRAM && s->bound) {
      uint32_t src_ip = 0;
      uint16_t src_port = 0;
      if (!socket_ready_for_read(s)) {
        if (socket_is_nonblocking(s)) {
          syscall_complete(regs, syscall_num, NS_ERR_WOULD_BLOCK);
          break;
        }
        if (!socket_wait_for_readable(s, s->rcv_timeout_ticks)) {
          syscall_complete(regs, syscall_num, NS_ERR_WOULD_BLOCK);
          break;
        }
      }
      rc = net_udp_recv(s->local_port, &src_ip, &src_port, buf, (uint16_t)len);
      if (rc > 0 && (s->peer_ip == 0 || s->peer_port == 0)) {
        s->peer_ip = src_ip;
        s->peer_port = src_port;
      }
    } else if (s->type == NS_SOCK_STREAM && s->net_conn_id >= 0) {
      if (!socket_ready_for_read(s)) {
        if (socket_is_nonblocking(s)) {
          syscall_complete(regs, syscall_num, NS_ERR_WOULD_BLOCK);
          break;
        }
        if (!socket_wait_for_readable(s, s->rcv_timeout_ticks)) {
          syscall_complete(regs, syscall_num, NS_ERR_WOULD_BLOCK);
          break;
        }
      }
      rc = net_tcp_recv(s->net_conn_id, buf, (uint16_t)len);
    }

    if (rc == 0) {
      rc = (int)NS_ERR_WOULD_BLOCK;
    }
    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  case SYS_SETSOCKOPT: {
    int fd = (int)regs[4];
    int opt = (int)regs[5];
    uint32_t val = regs[6];
    NsSocketEntry *s = socket_entry_for_fd(os_current_task, fd);

    if (s == 0) {
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }

    switch (opt) {
    case NS_SO_REUSEADDR:
      s->reuse_addr = (uint8_t)(val ? 1 : 0);
      syscall_complete(regs, syscall_num, NS_OK);
      break;
    case NS_SO_KEEPALIVE:
      s->keepalive = (uint8_t)(val ? 1 : 0);
      syscall_complete(regs, syscall_num, NS_OK);
      break;
    case NS_SO_RCVTIMEO:
      s->rcv_timeout_ticks = val;
      syscall_complete(regs, syscall_num, NS_OK);
      break;
    case NS_SO_SNDTIMEO:
      s->snd_timeout_ticks = val;
      syscall_complete(regs, syscall_num, NS_OK);
      break;
    default:
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }
    break;
  }

  case SYS_GETSOCKOPT: {
    int fd = (int)regs[4];
    int opt = (int)regs[5];
    NsSocketEntry *s = socket_entry_for_fd(os_current_task, fd);
    uint32_t out = 0;

    if (s == 0) {
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }

    switch (opt) {
    case NS_SO_REUSEADDR:
      out = (uint32_t)s->reuse_addr;
      break;
    case NS_SO_KEEPALIVE:
      out = (uint32_t)s->keepalive;
      break;
    case NS_SO_RCVTIMEO:
      out = s->rcv_timeout_ticks;
      break;
    case NS_SO_SNDTIMEO:
      out = s->snd_timeout_ticks;
      break;
    default:
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }
    if (opt == NS_SO_REUSEADDR || opt == NS_SO_KEEPALIVE ||
        opt == NS_SO_RCVTIMEO || opt == NS_SO_SNDTIMEO) {
      syscall_complete(regs, syscall_num, out);
    }
    break;
  }

  case SYS_SHUTDOWN: {
    int fd = (int)regs[4];
    int how = (int)regs[5];
    NsSocketEntry *s = socket_entry_for_fd(os_current_task, fd);

    if (s == 0 || (how != NS_SHUT_RD && how != NS_SHUT_WR && how != NS_SHUT_RDWR)) {
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }

    if (how == NS_SHUT_RD || how == NS_SHUT_RDWR) {
      s->shut_rd = 1;
    }
    if (how == NS_SHUT_WR || how == NS_SHUT_RDWR) {
      s->shut_wr = 1;
      if (s->type == NS_SOCK_STREAM && s->net_conn_id >= 0) {
        (void)net_tcp_close(s->net_conn_id);
      }
    }

    syscall_complete(regs, syscall_num, NS_OK);
    break;
  }

  case SYS_SELECT:
  case SYS_POLL: {
    NsPollFd *fds = (NsPollFd *)regs[4];
    uint32_t count = regs[5];
    uint32_t timeout_ticks = regs[6];
    int rc;

    if (count == 0u) {
      syscall_complete(regs, syscall_num, NS_OK);
      break;
    }
    if (count > TASK_MAX_FDS || fds == 0) {
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }
    if (cpl == 3 && !validate_user_ptr(fds, sizeof(NsPollFd) * count)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    rc = poll_socket_array_wait(fds, count, timeout_ticks);
    syscall_complete(regs, syscall_num, (uint32_t)rc);
    break;
  }

  case SYS_EPOLL: {
    int op = (int)regs[4];
    int epfd = (int)regs[5];
    uint32_t arg = regs[6];

    if (op == NS_EPOLL_OP_CREATE) {
      int newfd = epoll_alloc_fd(os_current_task);
      if (newfd < 0) {
        syscall_complete(regs, syscall_num, (uint32_t)VFS_ERR_NO_SPACE);
      } else {
        syscall_complete(regs, syscall_num, (uint32_t)newfd);
      }
      break;
    }

    if (op == NS_EPOLL_OP_CLOSE) {
      NsEpollObject *obj = epoll_object_for_fd(os_current_task, epfd);
      if (obj == 0) {
        syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      } else {
        epoll_close_object(obj);
        syscall_complete(regs, syscall_num, NS_OK);
      }
      break;
    }

    if (op == NS_EPOLL_OP_CTL) {
      NsEpollObject *obj = epoll_object_for_fd(os_current_task, epfd);
      NsEpollCtlReq *req = (NsEpollCtlReq *)arg;
      int rc;

      if (obj == 0 || req == 0) {
        syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
        break;
      }
      if (cpl == 3 && !validate_user_ptr(req, sizeof(NsEpollCtlReq))) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }

      rc = epoll_ctl_apply(obj, req);
      syscall_complete(regs, syscall_num, (uint32_t)rc);
      break;
    }

    if (op == NS_EPOLL_OP_WAIT) {
      NsEpollObject *obj = epoll_object_for_fd(os_current_task, epfd);
      NsEpollWaitReq *req = (NsEpollWaitReq *)arg;
      int rc;

      if (obj == 0 || req == 0) {
        syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
        break;
      }
      if (cpl == 3 && !validate_user_ptr(req, sizeof(NsEpollWaitReq))) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }
      if (req->events == 0 || req->maxevents == 0u || req->maxevents > TASK_MAX_FDS) {
        syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
        break;
      }
      if (cpl == 3 && !validate_user_ptr(req->events, sizeof(NsPollFd) * req->maxevents)) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }

      rc = epoll_wait_ready(obj, req->events, req->maxevents, req->timeout_ticks);
      syscall_complete(regs, syscall_num, (uint32_t)rc);
      break;
    }

    syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
    break;
  }

  case SYS_DLOPEN: {
    const char *path = (const char *)regs[4];
    int handle;

    if (cpl == 3 && !validate_user_string(path)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    handle = module_dlopen_for_task(os_current_task, path);
    if (handle < 0) {
      syscall_complete(regs, syscall_num, NS_ERR_NOT_IMPLEMENTED);
    } else {
      syscall_complete(regs, syscall_num, (uint32_t)handle);
    }
    break;
  }

  case SYS_DLSYM: {
    int handle = (int)regs[4];
    const char *symbol = (const char *)regs[6];
    uint32_t addr;

    if (cpl == 3 && !validate_user_string(symbol)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    addr = module_dlsym(handle, symbol);
    if (addr == 0) {
      syscall_complete(regs, syscall_num, NS_ERR_NOT_IMPLEMENTED);
    } else {
      syscall_complete(regs, syscall_num, addr);
    }
    break;
  }

  case SYS_INSMOD: {
    const char *path = (const char *)regs[4];
    int rc;

    if (cpl == 3 && !validate_user_string(path)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    rc = module_insmod(path);
    syscall_complete(regs, syscall_num,
                     rc == 0 ? NS_OK : NS_ERR_NOT_IMPLEMENTED);
    break;
  }

  case SYS_RMMOD: {
    const char *path_or_handle = (const char *)regs[4];
    int rc;

    if (cpl == 3 && !validate_user_string(path_or_handle)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    rc = module_rmmod(path_or_handle);
    syscall_complete(regs, syscall_num,
                     rc == 0 ? NS_OK : NS_ERR_NOT_IMPLEMENTED);
    break;
  }

  case SYS_DLCLOSE: {
    int handle = (int)regs[4];
    int rc = module_dlclose_for_task(os_current_task, handle);
    syscall_complete(regs, syscall_num,
                     rc == 0 ? NS_OK : NS_ERR_NOT_IMPLEMENTED);
    break;
  }

  case SYS_TCSETPGRP: {
    int tty_id = (int)regs[4];
    int pgid = (int)regs[6];
    int rc = posix_tty_tcsetpgrp(os_current_task, tty_id, pgid);
    syscall_complete(regs, syscall_num, rc == VFS_OK ? NS_OK : NS_ERR_PERMISSION);
    break;
  }

  case SYS_TCGETPGRP: {
    int tty_id = (int)regs[4];
    int rc = posix_tty_tcgetpgrp(tty_id);
    syscall_complete(regs, syscall_num,
                     rc >= 0 ? (uint32_t)rc : NS_ERR_INVALID_ARG);
    break;
  }

  case SYS_TRACE: {
    int task_id = (int)regs[4];
    int mode = (int)regs[5];

    if (!ptrace_task_allowed(cpl, task_id)) {
      syscall_complete(regs, syscall_num, NS_ERR_PERMISSION);
      break;
    }

    if (mode > 0) {
      os_tasks[task_id].trace_enabled = 1;
    } else if (mode < 0) {
      os_tasks[task_id].trace_enabled = 0;
    }

    syscall_complete(regs, syscall_num, os_tasks[task_id].trace_enabled);
    break;
  }

  case SYS_PTRACE: {
    int task_id = (int)regs[4];
    uint32_t request = regs[5];
    uint32_t arg = regs[6];

    if (!ptrace_task_allowed(cpl, task_id)) {
      syscall_complete(regs, syscall_num, NS_ERR_PERMISSION);
      break;
    }

    if (request == NS_PTRACE_GET_INFO) {
      NsPtraceInfo *info = (NsPtraceInfo *)arg;

      if (cpl == 3u && !validate_user_ptr(info, sizeof(NsPtraceInfo))) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }

      ptrace_fill_info(task_id, info);
      syscall_complete(regs, syscall_num, NS_OK);
      break;
    }

    if (request == NS_PTRACE_GET_STACK) {
      NsPtraceStack *stack = (NsPtraceStack *)arg;

      if (cpl == 3u && !validate_user_ptr(stack, sizeof(NsPtraceStack))) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }

      ptrace_fill_stack(task_id, stack);
      syscall_complete(regs, syscall_num, NS_OK);
      break;
    }

    if (request == NS_PTRACE_SET_TRACE) {
      os_tasks[task_id].trace_enabled = arg ? 1u : 0u;
      syscall_complete(regs, syscall_num, os_tasks[task_id].trace_enabled);
      break;
    }

    if (request == NS_PTRACE_SET_PRIORITY) {
      if (arg > 7u) {
        syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
        break;
      }
      os_tasks[task_id].priority = arg;
      task_trace_event(task_id, TASK_TRACE_EVT_PRIORITY, arg);
      syscall_complete(regs, syscall_num, NS_OK);
      break;
    }

    syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
    break;
  }

  case SYS_IO_SETUP: {
    uint32_t maxevents = regs[4];
    uint32_t *ctx_out = (uint32_t *)regs[6];
    uint32_t handle;

    if (maxevents == 0u || maxevents > NS_AIO_MAX_REQ ||
        (cpl == 3 && !validate_user_ptr(ctx_out, sizeof(uint32_t)))) {
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }

    handle = aio_alloc_context(maxevents);
    if (handle == 0u) {
      syscall_complete(regs, syscall_num, VFS_ERR_NO_SPACE);
      break;
    }

    *ctx_out = handle;
    syscall_complete(regs, syscall_num, NS_OK);
    break;
  }

  case SYS_IO_SUBMIT: {
    uint32_t handle = regs[4];
    uint32_t nr = regs[5];
    NsIoCb *iocb = (NsIoCb *)regs[6];
    NsAioCtx *ctx = aio_ctx_from_handle(handle);
    uint32_t accepted = 0;

    if (ctx == 0 || nr == 0u || nr > NS_AIO_MAX_REQ ||
        (cpl == 3 && !validate_user_ptr(iocb, nr * sizeof(NsIoCb)))) {
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }

    for (uint32_t i = 0; i < nr; i++) {
      if (!aio_submit_one(ctx, &iocb[i])) {
        break;
      }
      accepted++;
    }

    syscall_complete(regs, syscall_num, accepted > 0u ? accepted : NS_ERR_WOULD_BLOCK);
    break;
  }

  case SYS_IO_GETEVENTS: {
    uint32_t handle = regs[4];
    uint32_t maxevents = regs[5];
    NsIoEvent *events = (NsIoEvent *)regs[6];
    NsAioCtx *ctx = aio_ctx_from_handle(handle);
    int done;

    if (ctx == 0 || maxevents == 0u ||
        (cpl == 3 && !validate_user_ptr(events, maxevents * sizeof(NsIoEvent)))) {
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }

    if (maxevents > ctx->maxevents) {
      maxevents = ctx->maxevents;
    }
    done = aio_process_events(ctx, events, maxevents);
    syscall_complete(regs, syscall_num, (uint32_t)done);
    break;
  }

  case SYS_DLERROR: {
    char *buf = (char *)regs[4];
    uint32_t cap = regs[6];
    int copied;

    if (buf == 0 || cap == 0u) {
      syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
      break;
    }
    if (cpl == 3 && !validate_user_ptr(buf, cap)) {
      syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
      break;
    }

    copied = module_copy_dlerror_for_task(os_current_task, buf, cap);
    syscall_complete(regs, syscall_num,
                     copied >= 0 ? (uint32_t)copied : NS_ERR_NOT_IMPLEMENTED);
    break;
  }

  /* ---- SYS_WRITE (4): write to FD or fallback to legacy gprint ---- */
  case SYS_WRITE: {
    int fd = (int)regs[4];
    const void *buf = (const void *)regs[6];
    uint32_t size = regs[5];
    int wrote = VFS_ERR_INVALID_ARG;
    NsSocketEntry *s = socket_entry_for_fd(os_current_task, fd);

    if (s != 0 && buf != 0 && size > 0) {
      if (cpl == 3 && !validate_user_ptr(buf, size)) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }
      if (s->shut_wr) {
        syscall_complete(regs, syscall_num, NS_ERR_INVALID_ARG);
        break;
      }

      if (s->type == NS_SOCK_STREAM && s->net_conn_id >= 0) {
        if (!socket_ready_for_write(s)) {
          if (socket_is_nonblocking(s)) {
            syscall_complete(regs, syscall_num, NS_ERR_WOULD_BLOCK);
            break;
          }
          if (!socket_wait_for_writable(s, s->snd_timeout_ticks)) {
            syscall_complete(regs, syscall_num, NS_ERR_WOULD_BLOCK);
            break;
          }
        }
        wrote = net_tcp_send(s->net_conn_id, buf, (uint16_t)size);
      } else if (s->type == NS_SOCK_DGRAM && s->peer_ip != 0 && s->peer_port != 0) {
        wrote = net_udp_send(s->peer_ip,
                             s->bound ? s->local_port : 40000u,
                             s->peer_port,
                             buf,
                             (uint16_t)size)
                    ? (int)size
                    : (int)NS_ERR_WOULD_BLOCK;
      }

      syscall_complete(regs, syscall_num, (uint32_t)wrote);
      break;
    }

    if (fd >= 0 && buf != 0 && size > 0) {
      if (cpl == 3 && !validate_user_ptr(buf, size)) {
        syscall_complete(regs, syscall_num, NS_ERR_BAD_POINTER);
        break;
      }
      if (posix_check_fd_permission(os_current_task, fd, POSIX_ACC_WRITE) != VFS_OK) {
        syscall_complete(regs, syscall_num, NS_ERR_PERMISSION);
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