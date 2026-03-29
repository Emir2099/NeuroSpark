/* syscall.c – NeuroSpark system call dispatcher */

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

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

static uint32_t syscall_caller_cpl(uint32_t *regs) {
  uint32_t *iret_frame = (uint32_t *)((uint8_t *)regs + (8 * sizeof(uint32_t)));
  uint32_t saved_cs = iret_frame[1];
  return saved_cs & 0x3;
}

static int validate_user_string(const char *s) {
  for (uint32_t i = 0; i < MAX_USER_STRING_LEN; i++) {
    if (!is_user_range((const void *)(s + i), 1)) {
      return 0;
    }
    if (s[i] == '\0') {
      return 1;
    }
  }
  return 0;
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

  /* ---- SYS_WRITE (4): write a NUL-terminated string via gprint ---- */
  case SYS_WRITE: {
    char *msg = (char *)regs[4]; /* EBX */
    if (cpl == 3 && !validate_user_string(msg)) {
      regs[7] = NS_ERR_BAD_POINTER;
      break;
    }
    gprint(msg, 0x00FF00);       /* Green */
    regs[7] = NS_OK;
    task_trace_syscall(os_current_task, syscall_num, regs[7]);
    break;
  }

  /* ---- SYS_EXIT (1): terminate current user task ---- */
  case SYS_EXIT: {
    extern void task_terminate_current(void);
    if (cpl != 3) {
      regs[7] = NS_ERR_PERMISSION;
      task_trace_syscall(os_current_task, syscall_num, regs[7]);
      break;
    }
    regs[7] = NS_OK;
    task_trace_syscall(os_current_task, syscall_num, regs[7]);
    task_terminate_current();
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
      regs[7] = NS_ERR_WOULD_BLOCK;
    } else {
      char c = kb_buf[kb_tail];
      kb_tail = (kb_tail + 1) % KB_BUF_SIZE;
      irq_lock_release(&kb_lock, flags);
      regs[7] = (uint32_t)(unsigned char)c; /* Return ASCII code */
    }
    task_trace_syscall(os_current_task, syscall_num, regs[7]);
    break;
  }

  /* ---- SYS_FLIP (10): blit backbuffer → VRAM, mutex-guarded ---- */
  case SYS_FLIP: {
    if (cpl == 3) {
      regs[7] = NS_ERR_PERMISSION;
      task_trace_syscall(os_current_task, syscall_num, regs[7]);
      break;
    }

    /* Disable interrupts for the duration of the blit so that
     * an incoming keyboard IRQ cannot call gprint simultaneously */
    __asm__ volatile("cli");
    flip_mutex = 1;
    flip_buffer();
    flip_mutex = 0;
    __asm__ volatile("sti");
    regs[7] = NS_OK;
    task_trace_syscall(os_current_task, syscall_num, regs[7]);
    break;
  }

  case SYS_IPC_SEND: {
    int channel = (int)regs[4];
    int value = (int)regs[5];
    if (channel < 0 || channel >= IPC_CHANNELS) {
      regs[7] = NS_ERR_BAD_POINTER;
    } else if (ipc_send(channel, value)) {
      regs[7] = NS_OK;
    } else {
      regs[7] = NS_ERR_WOULD_BLOCK;
    }
    task_trace_syscall(os_current_task, syscall_num, regs[7]);
    break;
  }

  case SYS_IPC_RECV: {
    int channel = (int)regs[4];
    int out = 0;
    if (channel < 0 || channel >= IPC_CHANNELS) {
      regs[7] = NS_ERR_BAD_POINTER;
    } else if (ipc_recv(channel, &out)) {
      regs[7] = (uint32_t)out;
    } else {
      regs[7] = NS_ERR_WOULD_BLOCK;
    }
    task_trace_syscall(os_current_task, syscall_num, regs[7]);
    break;
  }

  default:
    regs[7] = NS_ERR_INVALID_SYSCALL;
    task_trace_syscall(os_current_task, syscall_num, regs[7]);
    break;
  }
}