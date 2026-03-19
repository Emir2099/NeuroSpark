/* syscall.c – NeuroSpark system call dispatcher */

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

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
extern void irq_lock_acquire(void *lock, unsigned int *flags_out);
extern void irq_lock_release(void *lock, unsigned int flags);

typedef struct {
  volatile int locked;
} irq_lock_t;

static irq_lock_t kb_lock = {0};

/* ============================================================
 * Flip mutex (defined in kernel.c)
 * Prevents the keyboard ISR from calling gprint while
 * flip_buffer's rep-movsl is running.
 * ============================================================ */
extern volatile int flip_mutex;

/* ============================================================
 * syscall_handler  (called by the int 0x80 stub in interrupt.asm)
 * regs layout (pushed by the stub, lowest address first):
 *   [0]=EAX [1]=EBX [2]=ECX [3]=EDX [4]=ESI [5]=EDI [6]=ESP [7]=EBP
 * NOTE: EAX is regs[7] according to the original convention kept here.
 * ============================================================ */
void syscall_handler(uint32_t *regs) {
  uint32_t syscall_num = regs[7]; /* EAX */

  switch (syscall_num) {

  /* ---- SYS_WRITE (4): write a NUL-terminated string via gprint ---- */
  case SYS_WRITE: {
    char *msg = (char *)regs[6]; /* EBX */
    gprint(msg, 0x00FF00);       /* Green */
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
      regs[7] = 0; /* Return 0 = no key available */
    } else {
      char c = kb_buf[kb_tail];
      kb_tail = (kb_tail + 1) % KB_BUF_SIZE;
      irq_lock_release(&kb_lock, flags);
      regs[7] = (uint32_t)(unsigned char)c; /* Return ASCII code */
    }
    break;
  }

  /* ---- SYS_FLIP (10): blit backbuffer → VRAM, mutex-guarded ---- */
  case SYS_FLIP: {
    /* Disable interrupts for the duration of the blit so that
     * an incoming keyboard IRQ cannot call gprint simultaneously */
    __asm__ volatile("cli");
    flip_mutex = 1;
    flip_buffer();
    flip_mutex = 0;
    __asm__ volatile("sti");
    break;
  }

  default:
    break;
  }
}