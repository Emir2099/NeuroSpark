/* 1. Global Definitions and Types */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

/* External assembly wrapper for the timer */
extern void timer_wrapper(void);
extern void keyboard_wrapper(void);
extern void mouse_wrapper(void);

// External PMM functions
extern void init_pmm();
extern void *pmm_alloc_page();
extern void pmm_free_page(uint32_t page_addr);
extern void pmm_print_map();
extern void init_paging();
extern void wm_init(void);
extern void wm_render(void);
extern int storage_manifest_load(const char *path);
extern int wm_focused_needs_keyboard(void);

extern void enablePaging();
extern uint32_t page_directory[1024];

// Link to the address saved by the bootloader
extern uint32_t vbe_framebuffer;
static int graphics_enabled = 0;

// This will be our working pointer in the kernel
uint32_t *lfb;

// ============================================================================
// KERNEL-SPACE GDT
// The bootloader's GDT lives at 0x7C00+ which gets overwritten by the kernel's
// BSS (first_page_table is placed at 0x7000 by the linker). We must define our
// own GDT in kernel memory and reload GDTR before init_paging() corrupts it.
// ============================================================================
struct gdt_entry_struct {
  uint16_t limit_lo;
  uint16_t base_lo;
  uint8_t base_mid;
  uint8_t access;
  uint8_t flags_limit_hi;
  uint8_t base_hi;
} __attribute__((packed));

struct gdt_ptr_struct {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

struct tss_entry_struct {
  uint32_t prev_tss;
  uint32_t esp0;
  uint32_t ss0;
  uint32_t esp1;
  uint32_t ss1;
  uint32_t esp2;
  uint32_t ss2;
  uint32_t cr3;
  uint32_t eip;
  uint32_t eflags;
  uint32_t eax;
  uint32_t ecx;
  uint32_t edx;
  uint32_t ebx;
  uint32_t esp;
  uint32_t ebp;
  uint32_t esi;
  uint32_t edi;
  uint32_t es;
  uint32_t cs;
  uint32_t ss;
  uint32_t ds;
  uint32_t fs;
  uint32_t gs;
  uint32_t ldt;
  uint16_t trap;
  uint16_t iomap_base;
} __attribute__((packed));

static struct tss_entry_struct kernel_tss;

// 6 entries: null, kernel code/data, user code/data, TSS
struct gdt_entry_struct kernel_gdt[6] = {
    // Null descriptor
    {0, 0, 0, 0, 0, 0},
    // Code segment: base=0, limit=0xFFFFF, present, ring 0, exec, readable,
    // 32-bit, 4K granularity
    {0xFFFF, 0x0000, 0x00, 0x9A, 0xCF, 0x00},
    // Data segment: base=0, limit=0xFFFFF, present, ring 0, writable, 32-bit,
    // 4K granularity
    {0xFFFF, 0x0000, 0x00, 0x92, 0xCF, 0x00},
    // User code segment (ring 3)
    {0xFFFF, 0x0000, 0x00, 0xFA, 0xCF, 0x00},
    // User data segment (ring 3)
    {0xFFFF, 0x0000, 0x00, 0xF2, 0xCF, 0x00},
    // TSS descriptor, set at runtime
    {0, 0, 0, 0, 0, 0},
};

struct gdt_ptr_struct kernel_gdtp = {
    sizeof(kernel_gdt) - 1,
    0 // Will be set at runtime
};

void load_kernel_gdt() {
  uint32_t tss_base = (uint32_t)&kernel_tss;
  uint32_t tss_limit = sizeof(kernel_tss) - 1;

  kernel_tss.prev_tss = 0;
  kernel_tss.esp0 = 0;
  kernel_tss.ss0 = 0x10;
  kernel_tss.esp1 = 0;
  kernel_tss.ss1 = 0;
  kernel_tss.esp2 = 0;
  kernel_tss.ss2 = 0;
  kernel_tss.cr3 = 0;
  kernel_tss.eip = 0;
  kernel_tss.eflags = 0;
  kernel_tss.eax = 0;
  kernel_tss.ecx = 0;
  kernel_tss.edx = 0;
  kernel_tss.ebx = 0;
  kernel_tss.esp = 0;
  kernel_tss.ebp = 0;
  kernel_tss.esi = 0;
  kernel_tss.edi = 0;
  kernel_tss.es = 0x13;
  kernel_tss.cs = 0x0B;
  kernel_tss.ss = 0x13;
  kernel_tss.ds = 0x13;
  kernel_tss.fs = 0x13;
  kernel_tss.gs = 0x13;
  kernel_tss.ldt = 0;
  kernel_tss.trap = 0;
  kernel_tss.iomap_base = sizeof(kernel_tss);

  kernel_gdt[5].limit_lo = (uint16_t)(tss_limit & 0xFFFF);
  kernel_gdt[5].base_lo = (uint16_t)(tss_base & 0xFFFF);
  kernel_gdt[5].base_mid = (uint8_t)((tss_base >> 16) & 0xFF);
  kernel_gdt[5].access = 0x89;
  kernel_gdt[5].flags_limit_hi = (uint8_t)((tss_limit >> 16) & 0x0F);
  kernel_gdt[5].base_hi = (uint8_t)((tss_base >> 24) & 0xFF);

  kernel_gdtp.base = (uint32_t)&kernel_gdt;
  __asm__ volatile("lgdt (%0)\n"
                   // Reload all segment registers with the new GDT's selectors
                   "ljmp $0x08, $1f\n"
                   "1:\n"
                   "mov $0x10, %%ax\n"
                   "mov %%ax, %%ds\n"
                   "mov %%ax, %%es\n"
                   "mov %%ax, %%fs\n"
                   "mov %%ax, %%gs\n"
                   "mov %%ax, %%ss\n"
                   :
                   : "r"(&kernel_gdtp)
                   : "ax");

  __asm__ volatile("ltr %%ax" : : "a"(0x28));
}

void tss_set_kernel_stack(uint32_t kernel_stack_top) {
  kernel_tss.esp0 = kernel_stack_top;
}

// External Tasking
extern void create_task(int index, void (*func_ptr)(), uint32_t page_dir);
void pulse_neurons(); // Forward declaration

/* External reference to ATA Disk Driver */
extern void disk_write_sectors(uint32_t lba, uint8_t count, uint32_t *buffer);
/* External reference for loading */
extern void disk_read_sector(uint32_t lba, uint16_t *buffer);

#define THRESHOLD 1000        // Membrane potential required to spike
#define DECAY 5               // Voltage lost per clock tick (leaky behavior)
#define GRID_SIZE 10          // Number of neurons in our initial grid
#define REFRACTORY_TICKS 10   // 10 ticks = 100ms of "silence" after a spike
#define CRITICAL_THRESHOLD 15 // Spikes per second to trigger phase change
#define PIXELS_COUNT 2
#define NEURONS_PER_PIXEL 5
#define TASK_IO 0
#define TASK_COMPUTE 1
#define GLOBAL_MAX_SPIKES 5000 // If total spikes exceed this, initiate decay
#define SYSTEM_DECAY_RATE                                                      \
  10 // Amount of synaptic weight lost during global decay

#define MAX_FILES 4
#define FILENAME_LEN 8

#define COMMAND_MAX_LEN 32

#define AUTO_LAUNCH_USER_PROCESS 0
#define AUTO_EXEC_DISK_ON_BOOT 0

#define SHELL_START_ROW 15
#define SHELL_MIN_ROW 15
#define SHELL_MAX_ROW 24

/* ============================================================
 * Keyboard Circular Buffer
 * keyboard_handler pushes here; SYS_READ_KB (syscall 5) pops.
 * ============================================================ */
#define KB_BUF_SIZE 32
volatile char kb_buf[KB_BUF_SIZE];
volatile int kb_head = 0; /* write index – advanced by keyboard_handler */
volatile int kb_tail = 0; /* read  index – advanced by SYS_READ_KB      */

/* flip_mutex: 1 while flip_buffer() rep-movsl is running.
 * keyboard_handler checks this before calling gprint, so the
 * two operations never race on the backbuffer. */
volatile int flip_mutex = 0;

/* shell_dirty: set to 1 by keyboard_handler when a key is pressed.
 * Cleared by shell_render() after redrawing the input line.     */
volatile int shell_dirty = 0;

/* Command output persistence: store last command response */
#define CMD_OUTPUT_LEN 256
char cmd_output[CMD_OUTPUT_LEN] = {0};
int cmd_output_valid = 0;
volatile int cmd_output_timeout = 0;  /* Frames left to display output */

void set_cmd_output(const char *text) {
  if (text == 0) { cmd_output_valid = 0; return; }
  int len = 0;
  while (text[len] && len < CMD_OUTPUT_LEN - 1) len++;
  for (int i = 0; i < len; i++) cmd_output[i] = text[i];
  cmd_output[len] = '\0';
  cmd_output_valid = 1;
  cmd_output_timeout = 300;  /* Show for ~3 seconds at 100 FPS */
}

/* VESA Graphics Globals */
// This should be set by bootloader ideally, or hardcoded for now
extern uint32_t vbe_framebuffer;
extern uint32_t vbe_pitch;
extern uint32_t vbe_bpp;
extern int screen_width;
extern int screen_height;

// External graphics functions
extern void gprint(char *str, uint32_t color);
void list_files_gfx(); // Forward declaration

/* Shell input cursor (defined in graphics.c) */
extern int shell_cursor_x;
extern int shell_cursor_y;
extern void draw_cursor(uint32_t tick);
extern int cursor_x;
extern int cursor_y;
extern void gprint_hex(uint32_t val, int digits, uint32_t color);

/* Neuromorphic Globals */
#define NEURON_COUNT 16
#define THRESHOLD 1000
#define LEAK_RATE 2

#include "disk.h"
#include "pci.h" /* PciDevice, pci_found[], pci_found_count */
#include "scheduler.h"
#include "shell.h"
#include "input.h"
#include "dashboard.h"
#include "ahci.h"
#include "storage_manager.h"
#include "klog.h"
#include "paging.h"
#include "usermode.h"
#include "vfs.h"
#include "net.h"
#include "profiling.h"
#include "model_manager.h"

volatile int current_shell_row = SHELL_START_ROW;

int recent_spikes = 0;
uint8_t current_bg_color = 0x1F; // Default Blue

/* Waveform zoom state */
int zoom_level = 1;  /* 1 = full view (16 neurons), 2 = 8, 3 = ~5, 4 = 4 */
int zoom_offset = 0; /* first neuron index visible when zoomed in           */

// Forward declarations
void update_monitor();
void kprint(const char *str, int row, int col, unsigned char color);

int potentials[NEURON_COUNT] = {0}; // Current charge of each neuron

typedef struct {
  int voltage;           /* Current membrane potential */
  int spike_count;       /* Total spikes fired by this neuron */
  int id;                /* Unique identifier */
  int synaptic_weight;   /* Strength of connection to the next neuron */
  int refractory_timer;  /* Ticks remaining in recovery */
  int dynamic_threshold; /* Per-neuron local threshold */
} Neuron;

typedef struct {
  Neuron neurons[NEURONS_PER_PIXEL];
  uint8_t current_phase;   /* 0 = FLUID (Responsive), 1 = RIGID (Protected) */
  int pixel_recent_spikes; /* Local activity for this cluster */
} NeuralPixel;

NeuralPixel os_memory_map[PIXELS_COUNT];

typedef struct {
  int task_id;
  int priority;     /* 0 = Low, 1 = High */
  int target_pixel; /* Which cluster this task is currently assigned to */

  // Process-specific "Biological" parameters
  int base_integration;
  int fire_threshold;

  const char *task_name;

  // Stores the 'learned' weights and current potential for 5 neurons
  int saved_voltages[NEURONS_PER_PIXEL];
  int saved_weights[NEURONS_PER_PIXEL];

  int saved_thresholds[NEURONS_PER_PIXEL]; // For Adaptive Thresholding

  int last_spike_count;  /* Spike count from the previous second */
  int spikes_per_second; /* Calculated real-time throughput */
  int state;             /* Task lifecycle state (0=Active) */
} TaskControlBlock;

// For now, let's define two tasks
TaskControlBlock task_list[2];

typedef struct {
  char name[FILENAME_LEN];
  int is_used;
  char snapshot_tag[16];
  // Data payload: The saved state of 5 neurons
  int voltages[NEURONS_PER_PIXEL];
  int weights[NEURONS_PER_PIXEL];
  int thresholds[NEURONS_PER_PIXEL];
} VirtualFile;

volatile VirtualFile synapse_disk[MAX_FILES];

/* Tiny File System: Root directory (loaded from LBA 150) */
FileEntry root_directory[TFS_MAX_FILES];

volatile char input_buffer[COMMAND_MAX_LEN];
volatile int buffer_idx = 0;
int shell_line =
    15; // Start the shell on line 16 (80 * 15) to avoid the neural grid

/* 2. Low-level Hardware Helpers */
static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Additional I/O functions for disk operations
static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
  __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
  uint16_t ret;
  __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

/* 3. The IDT Entry Structure */
struct idt_entry {
  uint16_t base_lo;
  uint16_t sel; /* Kernel segment selector (0x08) */
  uint8_t always0;
  uint8_t flags;
  uint16_t base_hi;
} __attribute__((packed));

struct idt_ptr {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

struct idt_entry idt[256];
struct idt_ptr idtp;

/* 4. Interrupt Logic */
void set_idt_gate(int n, uint32_t handler, uint16_t sel, uint8_t flags) {
  idt[n].base_lo = handler & 0xFFFF;
  idt[n].sel = sel; /* Code segment */
  idt[n].always0 = 0;
  idt[n].flags = flags; /* Type and Attributes */
  idt[n].base_hi = (handler >> 16) & 0xFFFF;
}

extern void exception0_wrapper(void);
extern void exception1_wrapper(void);
extern void exception2_wrapper(void);
extern void exception3_wrapper(void);
extern void exception4_wrapper(void);
extern void exception5_wrapper(void);
extern void exception6_wrapper(void);
extern void exception7_wrapper(void);
extern void exception8_wrapper(void);
extern void exception9_wrapper(void);
extern void exception10_wrapper(void);
extern void exception11_wrapper(void);
extern void exception12_wrapper(void);
extern void exception13_wrapper(void);
extern void exception14_wrapper(void);
extern void exception15_wrapper(void);
extern void exception16_wrapper(void);
extern void exception17_wrapper(void);
extern void exception18_wrapper(void);
extern void exception19_wrapper(void);
extern void exception20_wrapper(void);
extern void exception21_wrapper(void);
extern void exception22_wrapper(void);
extern void exception23_wrapper(void);
extern void exception24_wrapper(void);
extern void exception25_wrapper(void);
extern void exception26_wrapper(void);
extern void exception27_wrapper(void);
extern void exception28_wrapper(void);
extern void exception29_wrapper(void);
extern void exception30_wrapper(void);
extern void exception31_wrapper(void);

static const uint32_t exception_wrappers[32] = {
    (uint32_t)exception0_wrapper,  (uint32_t)exception1_wrapper,
    (uint32_t)exception2_wrapper,  (uint32_t)exception3_wrapper,
    (uint32_t)exception4_wrapper,  (uint32_t)exception5_wrapper,
    (uint32_t)exception6_wrapper,  (uint32_t)exception7_wrapper,
    (uint32_t)exception8_wrapper,  (uint32_t)exception9_wrapper,
    (uint32_t)exception10_wrapper, (uint32_t)exception11_wrapper,
    (uint32_t)exception12_wrapper, (uint32_t)exception13_wrapper,
    (uint32_t)exception14_wrapper, (uint32_t)exception15_wrapper,
    (uint32_t)exception16_wrapper, (uint32_t)exception17_wrapper,
    (uint32_t)exception18_wrapper, (uint32_t)exception19_wrapper,
    (uint32_t)exception20_wrapper, (uint32_t)exception21_wrapper,
    (uint32_t)exception22_wrapper, (uint32_t)exception23_wrapper,
    (uint32_t)exception24_wrapper, (uint32_t)exception25_wrapper,
    (uint32_t)exception26_wrapper, (uint32_t)exception27_wrapper,
    (uint32_t)exception28_wrapper, (uint32_t)exception29_wrapper,
    (uint32_t)exception30_wrapper, (uint32_t)exception31_wrapper,
};

static const char *exception_name(uint32_t vector) {
  switch (vector) {
  case 0: return "#DE Divide Error";
  case 1: return "#DB Debug";
  case 2: return "NMI";
  case 3: return "#BP Breakpoint";
  case 4: return "#OF Overflow";
  case 5: return "#BR Bound Range";
  case 6: return "#UD Invalid Opcode";
  case 7: return "#NM Device Not Available";
  case 8: return "#DF Double Fault";
  case 9: return "Coprocessor Segment";
  case 10: return "#TS Invalid TSS";
  case 11: return "#NP Segment Not Present";
  case 12: return "#SS Stack Fault";
  case 13: return "#GP General Protection";
  case 14: return "#PF Page Fault";
  case 16: return "#MF x87 FP";
  case 17: return "#AC Alignment Check";
  case 18: return "#MC Machine Check";
  case 19: return "#XM SIMD FP";
  default: return "#EXC Unknown";
  }
}

static int exception_has_error_code(uint32_t vector) {
  return vector == 8 || vector == 10 || vector == 11 || vector == 12 ||
         vector == 13 || vector == 14 || vector == 17;
}

static void exception_report(uint32_t vector, uint32_t err, uint32_t eip,
                             uint32_t cs, uint32_t eflags, uint32_t cr2) {
  unsigned short *video = (unsigned short *)0xB8000;
  const char *name = exception_name(vector);

  for (int i = 0; i < 80 * 6; i++) {
    video[i] = 0x1F00 | ' ';
  }

  cursor_x = 0;
  cursor_y = 0;
  gprint("EXCEPTION ", 0x4F00);
  gprint((char *)name, 0x4F00);
  gprint(" VEC:", 0x4F00);
  gprint_hex(vector, 2, 0x4F00);

  cursor_x = 0;
  cursor_y = 16;
  gprint("EIP:", 0xFFFFFF);
  gprint_hex(eip, 8, 0x88FFAA);
  gprint(" CS:", 0xFFFFFF);
  gprint_hex(cs, 4, 0x88FFAA);
  gprint(" EFL:", 0xFFFFFF);
  gprint_hex(eflags, 8, 0x88FFAA);

  cursor_x = 0;
  cursor_y = 32;
  gprint("ERR:", 0xFFFFFF);
  gprint_hex(err, 8, 0xFFCC66);
  if (vector == 14) {
    gprint(" CR2:", 0xFFFFFF);
    gprint_hex(cr2, 8, 0xFFCC66);
  }

  if (lfb != 0) {
    for (int y = 240; y < 360; y++) {
      for (int x = 180; x < 620; x++) {
        lfb[y * 800 + x] = 0x441111;
      }
    }
  }
}

void exception_handler(uint32_t vector, uint32_t err, uint32_t *stack_frame) {
  uint32_t cr2 = 0;
  uint32_t eip = 0;
  uint32_t cs = 0;
  uint32_t eflags = 0;
  uint32_t frame_index = exception_has_error_code(vector) ? 1 : 0;

  __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
  if (stack_frame != 0) {
    eip = stack_frame[frame_index + 0];
    cs = stack_frame[frame_index + 1];
    eflags = stack_frame[frame_index + 2];
  }

  /* Ring 3 faults should kill only the current user task, not the kernel. */
  if ((cs & 0x3) == 0x3 && os_current_task > 0 && os_current_task < os_task_count) {
    os_tasks[os_current_task].fault_code = ((vector & 0xFFu) << 24) | (err & 0x00FFFFFFu);
    os_tasks[os_current_task].fault_addr = cr2;
    os_tasks[os_current_task].fault_eip = eip;
    task_trace_event(os_current_task, TASK_TRACE_EVT_FAULT,
                     os_tasks[os_current_task].fault_code);
    exception_report(vector, err, eip, cs, eflags, cr2);
    task_terminate_current();
    return;
  }

  exception_report(vector, err, eip, cs, eflags, cr2);

  while (1) {
    __asm__ volatile("hlt");
  }
}

void init_idt() {
  /* CRITICAL: Mask ALL PIC interrupts FIRST to prevent any interrupt
     from firing while we're setting up the new IDT. */
  outb(0x21, 0xFF);     /* Mask all on Master PIC */
  outb(0xA1, 0xFF);     /* Mask all on Slave PIC */

  idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
  idtp.base = (uint32_t)&idt;

  for (int i = 0; i < 32; i++) {
    set_idt_gate(i, exception_wrappers[i], 0x08, 0x8E);
  }

  /* Remap the PIC (Programmable Interrupt Controller) */
  /* Hardware interrupts IRQ 0-7 normally conflict with CPU exceptions. */
  /* We remap them to start at IDT index 32. */
  outb(0x20, 0x11);
  outb(0xA0, 0x11);
  outb(0x21, 0x20);
  outb(0xA1, 0x28);
  outb(0x21, 0x04);
  outb(0xA1, 0x02);
  outb(0x21, 0x01);
  outb(0xA1, 0x01);

  /* Now mask (disable) only the IRQs we don't want,
   * keeping IRQ0(timer), IRQ1(keyboard), IRQ2(cascade) unmasked.
   * Master PIC (0x21): IRQ0, IRQ1, IRQ2 enabled = 11111000 = 0xF8
   * Slave PIC (0xA1): IRQ12 (mouse) enabled = 11101111 = 0xEF
   */
  outb(0x21, 0xF8);
  outb(0xA1, 0xEF);

  /* Register our timer wrapper (IRQ0 -> Index 32) */
  set_idt_gate(32, (uint32_t)timer_wrapper, 0x08, 0x8E);

  set_idt_gate(33, (uint32_t)keyboard_wrapper, 0x08, 0x8E); // Keyboard
  set_idt_gate(44, (uint32_t)mouse_wrapper, 0x08, 0x8E);    // Mouse IRQ12

  /* Load the IDT into the CPU and enable interrupts */
  __asm__ volatile("lidt (%0)" : : "r"(&idtp));
  __asm__ volatile("sti");
}



/* 5. Timer Implementation */
uint32_t tick = 0;
uint32_t render_frame = 0;
void timer_handler(void) {
  tick++;
  
  /* Decrement command output timeout if active */
  if (cmd_output_timeout > 0) {
    cmd_output_timeout--;
  } else if (cmd_output_timeout == 0) {
    cmd_output_valid = 0;
  }
  
  unsigned short *video = (unsigned short *)0xB8000;
  uint32_t spike_profile_stamp = profile_begin();

  // Iterate through each Pixelated Cluster
  for (int p = 0; p < PIXELS_COUNT; p++) {
    // Find which task is assigned to this pixel
    TaskControlBlock *current_task = 0;
    for (int t = 0; t < 2; t++) {
      if (task_list[t].target_pixel == p) {
        current_task = &task_list[t];
        break;
      }
    }
    // Iterate through neurons within the current pixel
    for (int i = 0; i < NEURONS_PER_PIXEL; i++) {
      Neuron *n = &os_memory_map[p].neurons[i];

      // Calculate screen position: Line 2 (offset 80) + Pixel grouping
      int screen_pos = 80 + (p * NEURONS_PER_PIXEL) + i;

      // --- Refractory Logic ---
      if (n->refractory_timer > 0) {
        n->refractory_timer--;
        n->voltage = 0; // Lock voltage at 0 during recovery

        // Visual: Show a '#' to indicate the neuron is "recovering"
        video[screen_pos] = 0x1800 | '#';
        continue; // Skip integration/firing for this neuron
      }

      // 1. Integration (Background Noise)
      // If the pixel is RIGID (Phase 1), we still use a hardcoded dampener (10)
      // to ensure stability. If it is FLUID (Phase 0), we use the dynamic
      // 'base_integration' from the TCB.
        int base_gain = (os_memory_map[p].current_phase == 1)
                  ? 10
                  : current_task->base_integration;
        int task_idx = current_task->task_id;
        int adjusted_gain =
          model_adjust_base_gain(task_idx, i, base_gain, tick);

      // --- CONSOLIDATED INTEGRATION LOGIC ---
      // We apply task-specific rhythms based on the TASK assigned to this pixel
      if (current_task->task_id == TASK_IO) {
        // I/O Pixel: Fires based on high-frequency external stimulus
        if (tick % (i + 2) == 0)
          n->voltage += adjusted_gain;
      } else {
        // Compute Pixel: Fires in a slow, stable rhythmic pulse
        // if (tick % 20 == 0) n->voltage += (base_gain / 2);
        if (tick % 4 == 0)
          n->voltage += (adjusted_gain / 2) + 10;
      }

      // 2. Leakage (Bio-decay)
      if (n->voltage > 0)
        n->voltage -= DECAY;

      // 2.5 Weight Decay (Homeostasis)
      if (tick % 100 == 0 && n->synaptic_weight > 100) {
        n->synaptic_weight -= 5;
      }

      // 3. Fire & Synaptic Propagation
      // Use n->dynamic_threshold instead of the THRESHOLD constant
      if (n->voltage >= n->dynamic_threshold) {
        n->voltage = 0;
        n->spike_count++;
        n->refractory_timer = model_get_refractory_ticks(task_idx, i);

        // ADAPTATION: Increase threshold on every spike (Heavier to fire again)
        if (n->dynamic_threshold < 5000) {
          n->dynamic_threshold += model_get_threshold_raise_step(task_idx, i);
        }

        // Propagate within the same pixel cluster
        int next = (i + 1) % NEURONS_PER_PIXEL;
        os_memory_map[p].neurons[next].voltage += n->synaptic_weight;

        // Model-specific plasticity and optional STDP update.
        model_on_spike(task_idx, p, i);

        unsigned char color = 0x1A; // Default Green on Blue
        if (n->synaptic_weight > 500)
          color = 0x1C; // Red on Blue
        else if (n->synaptic_weight > 300)
          color = 0x1E; // Yellow on Blue

        video[screen_pos] = (color << 8) | '!';
      } else {
        // --- HOMEOSTASIS ---
        // Slowly lower the threshold back to base THRESHOLD over time
        if (tick % 50 == 0 && n->dynamic_threshold > THRESHOLD) {
          n->dynamic_threshold -= model_get_threshold_decay_step(task_idx, i);
        }

        // Visualize potential levels
        if (n->voltage > 700)
          video[screen_pos] = 0x1700 | '^';
        else if (n->voltage > 300)
          video[screen_pos] = 0x1700 | '.';
        else
          video[screen_pos] = 0x1F20; // Clear
      }
    }
  }

  // Keep the master heartbeat blinking in the corner
  video[79] = (tick % 20 < 10) ? 0x1F2A : 0x1F20;

  // --- Global Neural Decay (Thermal Throttling) ---
  // Calculate total spikes across all pixels for this check
  int current_total = 0;
  for (int p = 0; p < PIXELS_COUNT; p++) {
    for (int n = 0; n < NEURONS_PER_PIXEL; n++) {
      current_total += os_memory_map[p].neurons[n].spike_count;
    }
  }

  // If the system is "overheating" (too much activity), weaken ALL synapses
  if (current_total > GLOBAL_MAX_SPIKES) {
    for (int p = 0; p < PIXELS_COUNT; p++) {
      for (int n = 0; n < NEURONS_PER_PIXEL; n++) {
        if (os_memory_map[p].neurons[n].synaptic_weight > 100) {
          os_memory_map[p].neurons[n].synaptic_weight -= SYSTEM_DECAY_RATE;
        }
      }
    }
    // Visual indicator: Flash the background or a corner character to show
    // throttling
    video[78] = 0x4C21; // Red '!' in the corner
  } else {
    video[78] = 0x1F20; // Clear indicator
  }

  if (tick % 10 == 0) { // Update monitor every 10 ticks to save cycles
    update_monitor();
  }

  profile_end(PROFILE_SLOT_SPIKE_UPDATE, spike_profile_stamp);

  /* Scheduler quantum accounting and preemption point (IRQ0 driven). */
  scheduler_timer_tick();
}

void init_graphics() {
  // 1. Initialize our kernel pointer with the physical address
  lfb = (uint32_t *)vbe_framebuffer;

  if (lfb == 0) {
    return;
  }

  // 2. Clear with a format-aware path so 24bpp modes do not produce stripes.
  if (vbe_bpp == 24) {
    uint8_t *dst = (uint8_t *)lfb;
    uint32_t pitch = vbe_pitch ? vbe_pitch : (uint32_t)(screen_width * 3);
    for (int y = 0; y < screen_height; y++) {
      uint8_t *row = dst + (y * pitch);
      for (int x = 0; x < screen_width; x++) {
        row[x * 3 + 0] = 0x44; // B
        row[x * 3 + 1] = 0x00; // G
        row[x * 3 + 2] = 0x00; // R
      }
    }
  } else {
    for (int i = 0; i < 800 * 600; i++) {
      lfb[i] = 0x000044; // Dark blue background
    }
  }
}

void init_timer(uint32_t frequency) {
  uint32_t divisor = 1193182 / frequency;
  outb(0x43, 0x36);
  outb(0x40, (uint8_t)(divisor & 0xFF));
  outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

void itoa(int n, char *str) {
  int i = 0, is_negative = 0;
  if (n == 0) {
    str[i++] = '0';
    str[i] = '\0';
    return;
  }
  if (n < 0) {
    is_negative = 1;
    n = -n;
  }
  while (n != 0) {
    str[i++] = (n % 10) + '0';
    n = n / 10;
  }
  if (is_negative)
    str[i++] = '-';
  str[i] = '\0';
  // Reverse the string
  for (int j = 0; j < i / 2; j++) {
    char temp = str[j];
    str[j] = str[i - j - 1];
    str[i - j - 1] = temp;
  }
}

void update_monitor() {
  int total_spikes = 0;
  unsigned short *video = (unsigned short *)0xB8000;

  // Loop through each Pixelated Cluster to manage local phases
  for (int p = 0; p < PIXELS_COUNT; p++) {
    // Find which task is assigned to this pixel
    TaskControlBlock *current_task = 0;
    for (int t = 0; t < 2; t++) {
      if (task_list[t].target_pixel == p) {
        current_task = &task_list[t];
        break;
      }
    }

    int pixel_spikes = 0;

    // Aggregate spikes for this specific pixel
    for (int n = 0; n < NEURONS_PER_PIXEL; n++) {
      pixel_spikes += os_memory_map[p].neurons[n].spike_count;
    }
    total_spikes += pixel_spikes;

    // Calculate local recent spikes (delta) for this pixel
    static int last_pixel_totals[PIXELS_COUNT] = {0, 0};
    int local_recent = pixel_spikes - last_pixel_totals[p];
    last_pixel_totals[p] = pixel_spikes;

    // --- Neural Logging (SPS Calculation) ---
    // Since update_monitor runs every 10 ticks (100ms),
    // we multiply by 10 to get the estimated Spikes Per Second.
    current_task->spikes_per_second = local_recent * 10;
    current_task->last_spike_count = pixel_spikes;

    // --- PIXELATED Phase Change Logic ---
    const char *pixel_msg;
    unsigned char attr;

    // Check criticality per pixel
    if (local_recent > (CRITICAL_THRESHOLD / 2)) {
      os_memory_map[p].current_phase = 1; // RIGID
      attr = 0x4E;                        // Yellow text on RED
      pixel_msg = "RIGID ";

    } else {
      os_memory_map[p].current_phase = 0; // FLUID
      attr = 0x1F;                        // White on BLUE
      pixel_msg = "FLUID ";
    }

    // LOCAL STIFFNESS: Suppress only this pixel's voltage
    if (os_memory_map[p].current_phase == 1) {
      for (int i = 0; i < NEURONS_PER_PIXEL; i++) {
        if (os_memory_map[p].neurons[i].voltage > 200) {
          os_memory_map[p].neurons[i].voltage -= 200;
        }
      }
    }

    // --- Print Task Name + Phase Status ---
    // Format: "IO: FLUID" or "COMP: RIGID"
    // We use a wider spacing (40 chars) to prevent overlap on Line 7
    int offset = (80 * 6) + (p * 40);
    const char *t_name = current_task->task_name;

    // Print Task Name
    for (int i = 0; t_name[i] != '\0'; i++) {
      video[offset + i] = (attr << 8) | t_name[i];
    }
    // Print Separator
    video[offset + 4] = (attr << 8) | ':';
    video[offset + 5] = (attr << 8) | ' ';

    // Print Phase Status
    for (int i = 0; pixel_msg[i] != '\0'; i++) {
      video[offset + 6 + i] = (attr << 8) | pixel_msg[i];
    }

    // Clean up: Clear a few trailing characters to ensure old text doesn't
    // ghost
    video[offset + 11] = (attr << 8) | ' ';
    video[offset + 12] = (attr << 8) | ' ';

    // --- Display Neural Logging (SPS) on Line 8 ---
    int log_offset = (80 * 7) + (p * 40);
    char sps_str[10];
    itoa(current_task->spikes_per_second, sps_str);

    const char *sps_label = "SPS: ";
    for (int i = 0; sps_label[i] != '\0'; i++) {
      video[log_offset + i] = 0x0700 | sps_label[i]; // Light Grey
    }
    for (int i = 0; sps_str[i] != '\0'; i++) {
      video[log_offset + 5 + i] = 0x0E00 | sps_str[i]; // Yellow
    }
    // Clear old digits to prevent 100 becoming 1000 accidentally
    video[log_offset + 8] = 0x0700 | ' ';
    video[log_offset + 9] = 0x0700 | ' ';

    // --- Display Adaptive Threshold (THR) on Line 11 ---
    int thr_offset = (80 * 10) + (p * 40);
    char thr_str[10];
    // We show the threshold of the first neuron in the pixel cluster as a
    // represantative
    itoa(os_memory_map[p].neurons[0].dynamic_threshold, thr_str);
    const char *thr_label = "THR: ";
    for (int i = 0; thr_label[i] != '\0'; i++)
      video[thr_offset + i] = 0x0700 | thr_label[i];
    for (int i = 0; thr_str[i] != '\0'; i++)
      video[thr_offset + 5 + i] = 0x0B00 | thr_str[i]; // Cyan
    video[thr_offset + 9] = 0x0700 | ' ';              // Clear trailing digits
  }

  // --- Global UI Reporting ---
  char spike_str[10];
  itoa(total_spikes, spike_str);

  const char *label = "TOTAL GRID SPIKES: ";

  // Print label on the 5th line (80 * 4)
  int offset = 80 * 4;
  for (int i = 0; label[i] != '\0'; i++) {
    video[offset + i] = 0x0F00 | label[i];
  }
  // Print the actual count
  for (int i = 0; spike_str[i] != '\0'; i++) {
    video[offset + 19 + i] = 0x0E00 | spike_str[i];
  }

  // --- DEBUG OUTPUT ---
  // Show which task is on which pixel on line 10
  int debug_line = 80 * 9;
  for (int p = 0; p < PIXELS_COUNT; p++) {
    char debug_msg[20];
    int idx = 0;

    // Find task for this pixel
    TaskControlBlock *dbg_task = 0;
    for (int t = 0; t < 2; t++) {
      if (task_list[t].target_pixel == p) {
        dbg_task = &task_list[t];
        break;
      }
    }

    // Format: "P0:T0 P1:T1"
    debug_msg[idx++] = 'P';
    debug_msg[idx++] = '0' + p;
    debug_msg[idx++] = ':';
    debug_msg[idx++] = 'T';
    debug_msg[idx++] = '0' + dbg_task->task_id;
    debug_msg[idx++] = ' ';
    debug_msg[idx] = '\0';

    for (int i = 0; debug_msg[i] != '\0'; i++) {
      video[debug_line + (p * 10) + i] = 0x0F00 | debug_msg[i];
    }
  }
}



char get_ascii(uint8_t scancode) {
  static char map[0x80] = {
      0,   27,  '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
      '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
      'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
      'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
      'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' '};
  return map[scancode];
}

// ============================================================================
// NEURAL PERSISTENCE FUNCTIONS (Save/Load with Disk)
// ============================================================================

#if 0

// SAVE: Copy a task's LIVE neural state into a "File"
void sys_save_task(int task_id, int slot) {
  if (slot >= MAX_FILES)
    return;

  volatile VirtualFile *f = &synapse_disk[slot];
  TaskControlBlock *t = &task_list[task_id];

  // Copy Metadata
  for (int i = 0; i < FILENAME_LEN; i++)
    f->name[i] = t->task_name[i];
  f->is_used = 1;

  // Copy LIVE Neural State from the pixel this task is assigned to
  int px = t->target_pixel;
  for (int n = 0; n < NEURONS_PER_PIXEL; n++) {
    f->voltages[n] = os_memory_map[px].neurons[n].voltage;
    f->weights[n] = os_memory_map[px].neurons[n].synaptic_weight;
    f->thresholds[n] = os_memory_map[px].neurons[n].dynamic_threshold;
  }

  // Force memory write before any further operations
  __asm__ volatile("" ::: "memory");

  // ===== WRITE TO DISK (only if controller is present) =====
  if (ata_disk_available) {
    disk_write_sector(DISK_DATA_OFFSET + slot, (uint16_t *)f);
  }

  // Visual feedback
  unsigned short *video = (unsigned short *)0xB8000;
  video[79] =
      ata_disk_available ? 0x2F44 : 0x2F4D; // 'D' for disk, 'M' for memory-only
}

// LOAD: Restore a task's persistence data from a "File"
// Returns 1 on success, 0 if slot is empty
int sys_load_task(int task_id, int slot) {
  if (slot >= MAX_FILES)
    return 0;

  // ===== READ FROM DISK (only if controller is present) =====
  if (ata_disk_available) {
    disk_read_sector(DISK_DATA_OFFSET + slot, (uint16_t *)&synapse_disk[slot]);
  }

  // Force memory read
  __asm__ volatile("" ::: "memory");

  // Check if the file exists
  int used_val = synapse_disk[slot].is_used;

  // Debug: show is_used value on line 12
  unsigned short *video_dbg = (unsigned short *)0xB8000;
  video_dbg[80 * 11 + 5] = 0x0C00 | 'L'; // Load
  video_dbg[80 * 11 + 6] = 0x0C00 | ('0' + slot);
  video_dbg[80 * 11 + 7] = 0x0C00 | ':';
  video_dbg[80 * 11 + 8] = 0x0C00 | ('0' + used_val);

  if (!used_val)
    return 0;

  volatile VirtualFile *f = &synapse_disk[slot];
  TaskControlBlock *t = &task_list[task_id];

  // Restore Neural State to LIVE pixel neurons
  int px = t->target_pixel;
  for (int n = 0; n < NEURONS_PER_PIXEL; n++) {
    os_memory_map[px].neurons[n].voltage = f->voltages[n];
    os_memory_map[px].neurons[n].synaptic_weight = f->weights[n];
    os_memory_map[px].neurons[n].dynamic_threshold = f->thresholds[n];
    // Also update the TCB snapshot for consistency
    t->saved_voltages[n] = f->voltages[n];
    t->saved_weights[n] = f->weights[n];
    t->saved_thresholds[n] = f->thresholds[n];
  }

  // Visual feedback
  unsigned short *video = (unsigned short *)0xB8000;
  video[79] = ata_disk_available ? 0x1F44 : 0x1F4D; // 'D' or 'M'
  return 1;
}

void scroll_shell() {
  unsigned short *video = (unsigned short *)0xB8000;

  // STEP A: Physical Memory Shift
  // We only touch the Shell Zone (Lines 15 to 24)
  for (int row = 15; row < 24; row++) {
    for (int col = 0; col < 80; col++) {
      // Copy line below into current line
      video[(row * 80) + col] = video[((row + 1) * 80) + col];
    }
  }

  // STEP B: Clear the absolute bottom line (Line 24)
  for (int col = 0; col < 80; col++) {
    video[(24 * 80) + col] = 0x0720; // Light grey on black
  }

  current_shell_row = 24;
}

void keyboard_handler(void) {
  /* ================================================================
   * ATOMICITY: This ISR only updates data structures (input_buffer,
   * kb_buf, neural state).  It NEVER calls gprint / flip_buffer /
   * writes to VGA text memory.  All rendering is driven by
   * neuro_task_entry → shell_render → draw_cursor → flip.
   * ================================================================ */
  uint8_t scancode = 0;
  __asm__ volatile("inb $0x60, %0" : "=a"(scancode));

  if (scancode < 0x80) {
    char c = get_ascii(scancode);

    if (!wm_focused_needs_keyboard()) {
      if (scancode != 0x3B && scancode != 0x3C && scancode != 0x3D &&
          scancode != 0x3E && scancode != 0x3F) {
        return;
      }
    }

    /* --- 1. Control Keys (High Priority) --- */
    if (scancode == 0x1C) { /* ENTER */
      input_buffer[buffer_idx] = '\0';
      char cmd_copy[COMMAND_MAX_LEN];
      for (int ci = 0; ci <= buffer_idx && ci < COMMAND_MAX_LEN; ci++)
        cmd_copy[ci] = input_buffer[ci];
      cmd_copy[COMMAND_MAX_LEN - 1] = '\0';
      process_command(cmd_copy);
      buffer_idx = 0;
      shell_dirty = 1;
      return;
    }
    if (scancode == 0x0E && buffer_idx > 0) { /* BACKSPACE */
      buffer_idx--;
      shell_dirty = 1;
      return;
    }

    /* --- 2. Neural Probe & System Keys (Function Keys) --- */
    if (scancode == 0x3B) { /* F1 – stim pixel 0 */
      for (int n = 0; n < NEURONS_PER_PIXEL; n++)
        os_memory_map[0].neurons[n].voltage += 300;
      return;
    }
    if (scancode == 0x3C) { /* F2 – switch tasks */
      switch_tasks();
      return;
    }
    if (scancode == 0x3D) { /* F3 – zap pixel 1 */
      for (int n = 0; n < NEURONS_PER_PIXEL; n++)
        os_memory_map[1].neurons[n].voltage += 1000;
      return;
    }
    if (scancode == 0x3E) { /* F4 – quick save */
      sys_save_task(0, 0);
      return;
    }
    if (scancode == 0x3F) { /* F5 – quick load */
      sys_load_task(0, 0);
      return;
    }

    /* --- 3. Standard Character Input --- */
    if (c && buffer_idx < COMMAND_MAX_LEN - 1) {
      input_buffer[buffer_idx++] = c;
      /* Push to circular buffer for SYS_READ_KB */
      int next_head = (kb_head + 1) % KB_BUF_SIZE;
      if (next_head != kb_tail) {
        kb_buf[kb_head] = c;
        kb_head = next_head;
      }
      shell_dirty = 1;
    }
    /* --- 4. Fallback (unmapped scancode) --- */
    else if (!c) {
      os_memory_map[0].neurons[0].voltage += 500;
    }
  }
}

// Helper: Convert hex address to string for display
void hex_to_str(uint32_t n, char *str) {
  char *hex = "0123456789ABCDEF";
  str[0] = '0';
  str[1] = 'x';
  for (int i = 7; i >= 0; i--) {
    str[i + 2] = hex[n & 0xF];
    n >>= 4;
  }
  str[10] = '\0';
}

uint32_t str_to_hex(char *str) {
  uint32_t val = 0;
  // Skip "0x" if present
  int i = (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) ? 2 : 0;

  while (str[i] != '\0' && str[i] != ' ') {
    uint32_t byte = str[i];
    if (byte >= '0' && byte <= '9')
      byte = byte - '0';
    else if (byte >= 'a' && byte <= 'f')
      byte = byte - 'a' + 10;
    else if (byte >= 'A' && byte <= 'F')
      byte = byte - 'A' + 10;
    else
      break;

    val = (val << 4) | (byte & 0xF);
    i++;
  }
  return val;
}

/* ============================================================
 * Dashboard – Status Bar (top 100px)
 * ============================================================ */
extern void clear_region(int x0, int y0, int x1, int y1, uint32_t color);
extern void draw_hline(int y, int x0, int x1, uint32_t color);
extern void put_pixel(int x, int y, uint32_t color);
extern void gprint_dec(int val, uint32_t color);
extern void gprint_hex(uint32_t val, int digits, uint32_t color);
extern int cursor_x;
extern int cursor_y;

extern int pmm_get_free_pages(void); /* defined in pmm.c */

void draw_status_bar() {
  /* Dark bar background */
  clear_region(0, 0, 800, 100, 0x001122);

  /* Cyan divider at y=100 */
  draw_hline(100, 0, 800, 0x00FFFF);
  draw_hline(101, 0, 800, 0x006666);

  /* Title */
  cursor_x = 4;
  cursor_y = 8;
  gprint("NEUROSPARK OS ", 0x00FFFF);
  gprint("| ", 0x446666);

  /* PCI device count */
  gprint("PCI: ", 0xFFFF00);
  gprint_dec(pci_found_count, 0xFFFFFF);
  gprint(" dev  ", 0xFFFF00);

  /* Mark any NVMe/AHCI found */
  for (int i = 0; i < pci_found_count; i++) {
    if (pci_found[i].class_code == 0x01 && pci_found[i].subclass == 0x08) {
      gprint("[NVMe@B", 0x00FF88);
      gprint_hex(pci_found[i].bus, 2, 0x00FF88);
      gprint("] ", 0x00FF88);
    } else if (pci_found[i].class_code == 0x01 &&
               pci_found[i].subclass == 0x06) {
      gprint("[AHCI@B", 0x00FFCC);
      gprint_hex(pci_found[i].bus, 2, 0x00FFCC);
      gprint("] ", 0x00FFCC);
    }
  }

  /* Line 2: memory usage */
  cursor_x = 4;
  cursor_y = 22;
  gprint("MEM FREE: ", 0xAAAAAA);
  int free_pages = pmm_get_free_pages();
  gprint_dec(free_pages, 0x88FF88);
  gprint(" pages (", 0xAAAAAA);
  gprint_dec(free_pages * 4, 0x88FF88);
  gprint(" KB)", 0xAAAAAA);

  /* Line 3: neural activity */
  cursor_x = 4;
  cursor_y = 36;
  gprint("NEURONS: ", 0xAAAAAA);
  gprint_dec(NEURON_COUNT, 0xFF8800);
  gprint("  TICK: ", 0xAAAAAA);
  gprint_dec((int)tick, 0xFF8800);
  gprint("  FRAME: ", 0xAAAAAA);
  gprint_dec((int)render_frame, 0x44FFAA);
  gprint("  SPS[0]: ", 0xAAAAAA);
  gprint_dec(task_list[0].spikes_per_second, 0xFF5500);
  gprint("  SPS[1]: ", 0xAAAAAA);
  gprint_dec(task_list[1].spikes_per_second, 0xFF5500);

  /* Line 4: zoom indicator */
  cursor_x = 4;
  cursor_y = 50;
  gprint("ZOOM: ", 0xAAAAAA);
  gprint_dec(zoom_level, 0x00FFFF);
  gprint("x  ", 0xAAAAAA);
  if (zoom_level > 1) {
    gprint("N[", 0xAAAAAA);
    gprint_dec(zoom_offset, 0x00FFFF);
    gprint("-", 0xAAAAAA);
    gprint_dec(zoom_offset + (NEURON_COUNT / zoom_level) - 1, 0x00FFFF);
    gprint("]  ", 0xAAAAAA);
  }

  /* Line 5: Phase status per pixel */
  cursor_x = 4;
  cursor_y = 64;
  for (int p = 0; p < PIXELS_COUNT; p++) {
    const char *ph_str =
        (os_memory_map[p].current_phase == 1) ? "RIGID" : "FLUID";
    uint32_t ph_clr =
        (os_memory_map[p].current_phase == 1) ? 0xFF4444 : 0x44FF44;
    gprint(task_list[p].task_name, 0xFFFF00);
    gprint(": ", 0x446666);
    gprint((char *)ph_str, ph_clr);
    gprint("  ", 0);
  }

  /* ---- Phase Transition Auto-Save ---- */
  for (int p = 0; p < PIXELS_COUNT; p++) {
    if (prev_phase[p] == 0 && os_memory_map[p].current_phase == 1) {
      /* FLUID -> RIGID transition detected — auto-save potentials[] */
      if (ata_disk_available) {
        disk_read_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
        int slot = find_free_slot(root_directory);
        if (slot != -1) {
          root_directory[slot].lba = TFS_DATA_START + slot;
          root_directory[slot].flags = 1;
          root_directory[slot].size = sizeof(potentials);
          /* Name: "auto_N" where N = pixel index */
          root_directory[slot].name[0] = 'a';
          root_directory[slot].name[1] = 'u';
          root_directory[slot].name[2] = 't';
          root_directory[slot].name[3] = 'o';
          root_directory[slot].name[4] = '_';
          root_directory[slot].name[5] = (char)('0' + p);
          root_directory[slot].name[6] = '\0';
          disk_write_sector(root_directory[slot].lba, (uint16_t *)potentials);
          disk_write_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
          /* Visual flash: show auto-save indicator on status bar */
          gprint(" [AUTO-SAVED]", 0x00FF00);
        }
      }
    }
    prev_phase[p] = os_memory_map[p].current_phase;
  }
}

/* ============================================================
 * Dashboard – Live Waveform (y: 110-300)
 * ============================================================ */
void draw_waveform() {
  const int WIN_Y0 = 110;                 /* top of waveform window      */
  const int WIN_Y1 = 300;                 /* bottom of waveform window   */
  const int WIN_H = WIN_Y1 - WIN_Y0;      /* 190 pixels        */
  const int BASE_Y = WIN_Y0 + WIN_H - 10; /* baseline        */
  int BAR_W = 4 * zoom_level;             /* wider bars when zoomed in   */
  if (BAR_W > 20)
    BAR_W = 20; /* cap for sanity              */

  /* Label + border */
  draw_hline(WIN_Y0 - 1, 0, 800, 0x334455);
  draw_hline(WIN_Y1, 0, 800, 0x334455);

  /* Title in the waveform band */
  cursor_x = 4;
  cursor_y = WIN_Y0;
  gprint("NEURAL WAVEFORM [SRNEM]", 0x3366AA);

  /* Zoom: show a subset of neurons when zoomed in */
  int neurons_visible = NEURON_COUNT / zoom_level;
  if (neurons_visible < 1)
    neurons_visible = 1;
  int start_n = zoom_offset;
  if (start_n + neurons_visible > NEURON_COUNT)
    start_n = NEURON_COUNT - neurons_visible;
  if (start_n < 0)
    start_n = 0;

  /* X spacing: spread visible neurons evenly across 800px */
  int spacing = 800 / (neurons_visible + 1);

  int prev_px = -1, prev_py = -1;

  for (int idx = 0; idx < neurons_visible; idx++) {
    int i = start_n + idx;
    int potential = potentials[i];
    if (potential < 0)
      potential = 0;
    if (potential > THRESHOLD)
      potential = THRESHOLD;

    /* Map potential to a bar height (0 – WIN_H-20 pixels) */
    int bar_h = (potential * (WIN_H - 20)) / THRESHOLD;

    int px = spacing + idx * spacing;
    int py = BASE_Y - bar_h;

    /* Choose colour: green->yellow->red by intensity */
    uint32_t color;
    if (potential < THRESHOLD / 3) {
      color = 0x00CC44; /* Dim green */
    } else if (potential < (THRESHOLD * 2) / 3) {
      color = 0xFFDD00; /* Yellow */
    } else {
      color = 0xFF3300; /* Red – near spike */
    }

    /* Draw vertical bar */
    for (int dy = 0; dy < bar_h && dy < WIN_H - 10; dy++) {
      for (int dx = 0; dx < BAR_W; dx++) {
        put_pixel(px - BAR_W / 2 + dx, BASE_Y - dy, color);
      }
    }

    /* Draw connecting line to previous point */
    if (prev_px >= 0) {
      /* Simple Bresenham for the waveform spine */
      int dx = px - prev_px;
      int ddx = dx > 0 ? 1 : -1;
      for (int xi = prev_px; xi != px; xi += ddx) {
        int t_y = prev_py + (py - prev_py) * (xi - prev_px) / (dx ? dx : 1);
        put_pixel(xi, t_y, 0x005588);
      }
    }
    prev_px = px;
    prev_py = py;
  }

  /* Spike indicator: small '*' dot above firing neurons */
  for (int i = 0; i < NEURON_COUNT; i++) {
    if (potentials[i] >= THRESHOLD - 50) {
      int px = spacing + i * spacing;
      for (int dx = -2; dx <= 2; dx++)
        for (int dy = -2; dy <= 2; dy++)
          put_pixel(px + dx, WIN_Y0 + 10 + dy, 0xFFFFFF);
    }
  }
}

/* ============================================================
 * shell_render – redraw the command-line input area
 *
 * Called from neuro_task_entry every frame so the graphical shell
 * stays in sync with keyboard input without the ISR touching the
 * backbuffer.
 * ============================================================ */
void shell_render() {
  extern int shell_cursor_x;
  extern int shell_cursor_y;
  
  /* Wipe the shell input line (one 8px-tall row at shell_cursor_y) */
  clear_region(0, shell_cursor_y, 800, shell_cursor_y + 10, 0x000033);

  /* Draw prompt "> " */
  int saved_cx = cursor_x;
  int saved_cy = cursor_y;
  cursor_x = 0;
  cursor_y = shell_cursor_y;
  gprint("> ", 0x00FFFF);

  /* Echo the current input_buffer contents */
  for (int i = 0; i < buffer_idx && i < COMMAND_MAX_LEN; i++) {
    char ch[2] = {input_buffer[i], '\0'};
    gprint(ch, 0xFFFFFF);
  }

  /* Update shell cursor X for the blinking cursor position */
  shell_cursor_x = 16 + (buffer_idx * 8); /* 16 = "> " width */

  /* Draw persistent command output BELOW input line (y=330+) */
  clear_region(0, 330, 800, 345, 0x000033);
  if (cmd_output_valid) {
    cursor_x = 0;
    cursor_y = 330;
    gprint(cmd_output, 0x44FF88);  /* green text */
  }

  /* Restore gprint cursor */
  cursor_x = saved_cx;
  cursor_y = saved_cy;

  shell_dirty = 0;
}

/* ============================================================
 * neuro_task_entry - safe dashboard + flip
 *
 * CRITICAL: This runs as a separate task with only a 4KB stack.
 * DO NOT CALL clear_screen() HERE - it loops 480,000 iterations!!!!
 * which overflows the tiny task stack via accumulated frame overhead.
 * Use clear_region() to wipe only what we repaint each frame.
 * ============================================================ */
void neuro_task_entry() {
  while (1) {
    render_frame++;
    pulse_neurons();

    /* Render the glass desktop shell and taskbar. */
    wm_render();

    /* SYS_FLIP: blit backbuffer -> VRAM (CLI+STI guarded in syscall.c) */
    asm volatile("mov $10, %%eax; int $0x80" : : : "eax");

    for (volatile int d = 0; d < 400000; d++)
      ;

    /* Yield to the main shell task */
    extern void schedule();
    schedule();
  }
}

/* TFS: List all active files from the root directory */
void list_files() {
  // Read the Root Directory from disk (LBA 150)
  if (ata_disk_available) {
    disk_read_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
  }

  kprint("NAME         LBA    SIZE", current_shell_row, 0, 0x0B);
  if (current_shell_row >= SHELL_MAX_ROW)
    scroll_shell();
  else
    current_shell_row++;
  kprint("------------------------", current_shell_row, 0, 0x07);
  if (current_shell_row >= SHELL_MAX_ROW)
    scroll_shell();
  else
    current_shell_row++;

  int found = 0;
  for (int i = 0; i < TFS_MAX_FILES; i++) {
    if (root_directory[i].flags == 1) {
      kprint(root_directory[i].name, current_shell_row, 0, 0x0F);

      // Print LBA number
      char lba_str[10];
      itoa((int)root_directory[i].lba, lba_str);
      kprint(lba_str, current_shell_row, 13, 0x0E);

      // Print size
      char size_str[10];
      itoa((int)root_directory[i].size, size_str);
      kprint(size_str, current_shell_row, 20, 0x08);

      if (current_shell_row >= SHELL_MAX_ROW)
        scroll_shell();
      else
        current_shell_row++;
      found = 1;
    }
  }

  if (!found) {
    kprint("NO FILES FOUND.", current_shell_row, 0, 0x0C);
  }
}

void process_command(char *cmd) {
  unsigned short *video = (unsigned short *)0xB8000;

  // ===== WRITE DIRECTLY TO LINE 3 =====
  int line3 = 80 * 3;

  // Clear line 3 completely first
  for (int i = 0; i < 80; i++) {
    video[line3 + i] = 0x4F20; // Red background, space
  }

  // Write "CMD:" in bright white on red
  video[line3 + 0] = 0x4F00 | 'C';
  video[line3 + 1] = 0x4F00 | 'M';
  video[line3 + 2] = 0x4F00 | 'D';
  video[line3 + 3] = 0x4F00 | ':';

  // Show entire command
  for (int i = 0; i < 10 && cmd[i] != '\0'; i++) {
    video[line3 + 4 + i] = 0x4F00 | cmd[i];
  }

  // ===== NOW DO ACTUAL COMMAND PROCESSING =====

  // Advance to next line
  if (current_shell_row >= SHELL_MAX_ROW) {
    scroll_shell();
  } else {
    current_shell_row++;
  }

  int output_row = current_shell_row;
  int output_offset = output_row * 80;

  // Command: "save X"
  if (cmd[0] == 's' && cmd[1] == 'a' && cmd[2] == 'v' && cmd[3] == 'e' &&
      cmd[4] == ' ') {
    int task_idx = cmd[5] - '0';

    // Show on line 3 that we matched
    video[line3 + 20] = 0x2F00 | 'S';
    video[line3 + 21] = 0x2F00 | 'A';
    video[line3 + 22] = 0x2F00 | 'V';
    video[line3 + 23] = 0x2F00 | 'E';

    if (task_idx >= 0 && task_idx < 2) {
      // Call save
      sys_save_task(task_idx, task_idx);

      // Write "SAVED!" directly to shell
      video[output_offset + 0] = 0x2F00 | 'S';
      video[output_offset + 1] = 0x2F00 | 'A';
      video[output_offset + 2] = 0x2F00 | 'V';
      video[output_offset + 3] = 0x2F00 | 'E';
      video[output_offset + 4] = 0x2F00 | 'D';
      video[output_offset + 5] = 0x2F00 | '!';

      video[line3 + 25] = 0x2F00 | 'O';
      video[line3 + 26] = 0x2F00 | 'K';
    } else {
      video[output_offset + 0] = 0x4F00 | 'E';
      video[output_offset + 1] = 0x4F00 | 'R';
      video[output_offset + 2] = 0x4F00 | 'R';

      video[line3 + 25] = 0x4F00 | 'E';
      video[line3 + 26] = 0x4F00 | 'R';
    }
  }
  // Command: "load X"
  else if (cmd[0] == 'l' && cmd[1] == 'o' && cmd[2] == 'a' && cmd[3] == 'd' &&
           cmd[4] == ' ') {
    int task_idx = cmd[5] - '0';

    video[line3 + 20] = 0x1F00 | 'L';
    video[line3 + 21] = 0x1F00 | 'O';
    video[line3 + 22] = 0x1F00 | 'A';
    video[line3 + 23] = 0x1F00 | 'D';

    if (task_idx >= 0 && task_idx < 2) {
      int ok = sys_load_task(task_idx, task_idx);

      if (ok) {
        video[output_offset + 0] = 0x2F00 | 'L';
        video[output_offset + 1] = 0x2F00 | 'O';
        video[output_offset + 2] = 0x2F00 | 'A';
        video[output_offset + 3] = 0x2F00 | 'D';
        video[output_offset + 4] = 0x2F00 | 'E';
        video[output_offset + 5] = 0x2F00 | 'D';
        video[output_offset + 6] = 0x2F00 | '!';
      } else {
        video[output_offset + 0] = 0x4F00 | 'S';
        video[output_offset + 1] = 0x4F00 | 'L';
        video[output_offset + 2] = 0x4F00 | 'O';
        video[output_offset + 3] = 0x4F00 | 'T';
        video[output_offset + 4] = 0x4F00 | ' ';
        video[output_offset + 5] = 0x4F00 | 'E';
        video[output_offset + 6] = 0x4F00 | 'M';
        video[output_offset + 7] = 0x4F00 | 'P';
        video[output_offset + 8] = 0x4F00 | 'T';
        video[output_offset + 9] = 0x4F00 | 'Y';
      }

      video[line3 + 25] = ok ? (0x2F00 | 'O') : (0x4F00 | 'E');
      video[line3 + 26] = ok ? (0x2F00 | 'K') : (0x4F00 | 'R');
    }
  }
  // Command: "ls"
  else if (cmd[0] == 'l' && cmd[1] == 's') {
    gprint("SCANNING NEURO-FILESYSTEM...\n", 0x00FFFF); // Cyan
    list_files_gfx(); // Update your list_files to use gprint
  }
  // Command: "help"
  else if (cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'l' && cmd[3] == 'p' &&
           cmd[4] == '\0') {
    video[line3 + 20] = 0x0700 | 'H';
    video[line3 + 21] = 0x0700 | 'E';
    video[line3 + 22] = 0x0700 | 'L';
    video[line3 + 23] = 0x0700 | 'P';

    kprint("save 0, load 0, stim, eval", current_shell_row, 0, 0x07);
    if (current_shell_row >= SHELL_MAX_ROW)
      scroll_shell();
    else
      current_shell_row++;
    kprint("tsave, tload, ls, pci, pci bar", current_shell_row, 0, 0x07);
    if (current_shell_row >= SHELL_MAX_ROW)
      scroll_shell();
    else
      current_shell_row++;
    kprint("zoom+ zoom- zpan+ zpan- clear", current_shell_row, 0, 0x07);
  }
  // Command: "clear"
  else if (cmd[0] == 'c' && cmd[1] == 'l' && cmd[2] == 'e' && cmd[3] == 'a' &&
           cmd[4] == 'r' && cmd[5] == '\0') {
    video[line3 + 20] = 0x0A00 | 'C';
    video[line3 + 21] = 0x0A00 | 'L';
    video[line3 + 22] = 0x0A00 | 'R';

    for (int row = SHELL_MIN_ROW; row <= SHELL_MAX_ROW; row++) {
      for (int col = 0; col < 80; col++) {
        video[(row * 80) + col] = 0x0F20;
      }
    }
    current_shell_row = SHELL_MIN_ROW;
    return;
  }
  // Command: "eval"
  else if (cmd[0] == 'e' && cmd[1] == 'v' && cmd[2] == 'a' && cmd[3] == 'l') {
    int active_task = 0; // Evaluate Task 0 (IO)
    TaskControlBlock *t = &task_list[active_task];
    int px = t->target_pixel; // Point at the LIVE pixel neurons

    int total_potential = 0;
    int total_resistance = 0;

    // Perform a LIVE "Neural Calculation" — reads directly from os_memory_map
    for (int n = 0; n < NEURONS_PER_PIXEL; n++) {
      // Potential = current voltage + synaptic weight (activity + learned
      // strength)
      total_potential += os_memory_map[px].neurons[n].voltage;
      total_potential += os_memory_map[px].neurons[n].synaptic_weight;
      // Higher thresholds = higher resistance to spiking (Fluid/Rigid phase)
      total_resistance += os_memory_map[px].neurons[n].dynamic_threshold;
    }

    kprint("NEURAL EVALUATION:", current_shell_row, 0, 0x0B);
    if (current_shell_row >= SHELL_MAX_ROW)
      scroll_shell();
    else
      current_shell_row++;

    // Logic: If potential > resistance, the brain is "Active/Fluid"
    if (total_potential > total_resistance) {
      kprint(">> FLUID/ACTIVE <<", current_shell_row, 0,
             0x2F); // Bright white on green
    } else {
      kprint(">> RIGID/STABLE <<", current_shell_row, 0, 0x4E); // Yellow on red
    }
  }
  // Command: "stim [id] [amount]" (e.g., stim 0 800)
  else if (cmd[0] == 's' && cmd[1] == 't' && cmd[2] == 'i' && cmd[3] == 'm') {
    int task_idx = cmd[5] - '0';

    // Simple manual parser for the stimulus amount
    // If the command is too short, we use a default of 500
    int amount = 500;
    if (cmd[7] != '\0') {
      // Simple conversion: assumes 3-digit number (e.g., 500, 999)
      amount = (cmd[7] - '0') * 100 + (cmd[8] - '0') * 10 + (cmd[9] - '0');
    }

    if (task_idx >= 0 && task_idx < 2) {
      // 1. Target the correct Task
      TaskControlBlock *t = &task_list[task_idx];
      int px = t->target_pixel; // Point at LIVE neurons

      // 2. Spread stimulus across the entire pixel cluster (5 neurons)
      // This triggers a collective reaction in the metamaterial phase logic
      for (int n = 0; n < NEURONS_PER_PIXEL; n++) {
        // Inject voltage into LIVE neurons
        os_memory_map[px].neurons[n].voltage += (amount / NEURONS_PER_PIXEL);

        // Permanent Synaptic Strengthening (+300 per stim)
        // Every stimulus makes the connection significantly "smarter"
        // We allow stim to push weight beyond the normal Hebbian cap of 800
        os_memory_map[px].neurons[n].synaptic_weight += 300;

        // Training Effect: Lower the firing threshold (priming, -800 per stim)
        // Makes neurons easier to fire — simulates neural plasticity
        // Must overpower the +200/spike adaptation in timer_handler
        // Floor at 200 to prevent negative/zero thresholds
        int new_thr = os_memory_map[px].neurons[n].dynamic_threshold - 800;
        if (new_thr < 200)
          new_thr = 200;
        os_memory_map[px].neurons[n].dynamic_threshold = new_thr;

        // Keep TCB snapshots in sync
        t->saved_voltages[n] = os_memory_map[px].neurons[n].voltage;
        t->saved_weights[n] = os_memory_map[px].neurons[n].synaptic_weight;
        t->saved_thresholds[n] = os_memory_map[px].neurons[n].dynamic_threshold;
      }

      // 3. UI Feedback
      kprint("STIMULUS INJECTED: ", current_shell_row, 0, 0x0E); // Yellow
      char val_str[10];
      itoa(amount, val_str);
      kprint(val_str, current_shell_row, 19, 0x0F);
    } else {
      kprint("ERR: INVALID TASK ID", current_shell_row, 0, 0x0C);
    }
  } else if (cmd[0] == 'm' && cmd[1] == 'a' && cmd[2] == 'l' && cmd[3] == 'l') {
    void *new_page = pmm_alloc_page();
    if (new_page) {
      char addr_str[11];
      hex_to_str((uint32_t)new_page, addr_str);
      kprint("ALLOCATED 4KB NEURAL PAGE AT: ", current_shell_row, 0, 0x0A);
      kprint(addr_str, current_shell_row, 30,
             0x0F); // White address after label
    } else {
      kprint("ERR: OUT OF PHYSICAL MEMORY", current_shell_row, 0, 0x0C);
    }
  } else if (cmd[0] == 'f' && cmd[1] == 'r' && cmd[2] == 'e' && cmd[3] == 'e') {
    // Extract the address starting at index 5: "free 0x100000"
    uint32_t addr_to_free = str_to_hex(&cmd[5]);

    if (addr_to_free >= 0x100000) {
      pmm_free_page(addr_to_free);
      kprint("PAGE RELEASED AT: ", current_shell_row, 0, 0x0E); // Yellow
      kprint(&cmd[5], current_shell_row, 18, 0x0F);
    } else {
      kprint("ERR: INVALID OR PROTECTED ADDR", current_shell_row, 0,
             0x0C); // Red
    }
  } else if (cmd[0] == 'm' && cmd[1] == 'a' && cmd[2] == 'p') {
    kprint("--- MEMORY MAP ---", current_shell_row, 0, 0x0B);
    current_shell_row++;
    if (current_shell_row >= SHELL_MAX_ROW)
      scroll_shell();
    pmm_print_map();
    // pmm_print_map uses 3 rows and advances current_shell_row itself
    if (current_shell_row >= SHELL_MAX_ROW)
      scroll_shell();
  }
  // Command: "tsave" - TFS named save of neural potentials
  else if (cmd[0] == 't' && cmd[1] == 's' && cmd[2] == 'a' && cmd[3] == 'v' &&
           cmd[4] == 'e') {
    // Load the existing directory from disk
    if (ata_disk_available) {
      disk_read_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
    }
    int slot = find_free_slot(root_directory);
    if (slot != -1) {
      kprint("SAVING SNAPSHOT...", current_shell_row, 0, 0x0E);
      if (current_shell_row >= SHELL_MAX_ROW)
        scroll_shell();
      else
        current_shell_row++;

      // Setup metadata
      root_directory[slot].lba = TFS_DATA_START + slot;
      root_directory[slot].flags = 1;
      root_directory[slot].size = sizeof(potentials);

      // Name: "brain0", "brain1", etc. or use user-supplied name
      // Check if user gave a name: "tsave myname"
      if (cmd[5] == ' ' && cmd[6] != '\0') {
        for (int i = 0; i < 11 && cmd[6 + i] != '\0'; i++)
          root_directory[slot].name[i] = cmd[6 + i];
        root_directory[slot].name[11] = '\0';
      } else {
        root_directory[slot].name[0] = 'b';
        root_directory[slot].name[1] = 'r';
        root_directory[slot].name[2] = 'a';
        root_directory[slot].name[3] = 'i';
        root_directory[slot].name[4] = 'n';
        root_directory[slot].name[5] = (char)('0' + slot);
        root_directory[slot].name[6] = '\0';
      }

      // Write neural data to the assigned LBA
      if (ata_disk_available) {
        disk_write_sector(root_directory[slot].lba, (uint16_t *)potentials);
        // Write updated directory back to LBA 150
        disk_write_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
      }

      kprint("SAVED AS: ", current_shell_row, 0, 0x0A);
      kprint(root_directory[slot].name, current_shell_row, 10, 0x0F);
    } else {
      kprint("ERR: DISK DIRECTORY FULL", current_shell_row, 0, 0x0C);
    }
  }
  // Command: "tload <name>" - TFS named load
  else if (cmd[0] == 't' && cmd[1] == 'l' && cmd[2] == 'o' && cmd[3] == 'a' &&
           cmd[4] == 'd') {
    // Require a name argument: "tload name"
    if (cmd[5] != ' ' || cmd[6] == '\0') {
      kprint("USAGE: tload <name>", current_shell_row, 0, 0x0C);
    } else {
      if (ata_disk_available) {
        disk_read_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
      }
      // Find the file by name (case-insensitive, starts at cmd[6])
      int found = -1;
      for (int i = 0; i < TFS_MAX_FILES; i++) {
        if (root_directory[i].flags != 1)
          continue;
        int match = 1;
        for (int j = 0; j < 11; j++) {
          char a = root_directory[i].name[j];
          char b = cmd[6 + j];
          // Normalize both to lowercase for comparison
          if (a >= 'A' && a <= 'Z')
            a += 32;
          if (b >= 'A' && b <= 'Z')
            b += 32;
          if (b == '\0') {
            // End of user input — match only if file name also ends here
            if (a != '\0')
              match = 0;
            break;
          }
          if (a != b) {
            match = 0;
            break;
          }
        }
        if (match) {
          found = i;
          break;
        }
      }
      if (found >= 0 && ata_disk_available) {
        disk_read_sector(root_directory[found].lba, (uint16_t *)potentials);
        kprint("LOADED: ", current_shell_row, 0, 0x0A);
        kprint(root_directory[found].name, current_shell_row, 8, 0x0F);
      } else {
        kprint("ERR: FILE NOT FOUND", current_shell_row, 0, 0x0C);
      }
    }
  } else if (cmd[0] == 'p' && cmd[1] == 'c' && cmd[2] == 'i') {
    if (cmd[3] == ' ' && cmd[4] == 'b' && cmd[5] == 'a' && cmd[6] == 'r') {
      /* "pci bar" – find storage controller, read BAR5, map MMIO */
      extern int pci_find_storage_controller(void);
      extern uint32_t pci_read_bar5(int device_index);
      extern void map_mmio_region(uint32_t phys, uint32_t size);
      int idx = pci_find_storage_controller();
      if (idx >= 0) {
        uint32_t bar5 = pci_read_bar5(idx);
        gprint("STORAGE CTRL FOUND  BAR5: ", 0x00FFCC);
        gprint_hex(bar5, 8, 0x00FF88);
        gprint("\n", 0);
        if (bar5 != 0) {
          map_mmio_region(bar5, 0x2000); /* Map 8KB AHCI HBA memory */
          gprint("MMIO MAPPED OK\n", 0x44FF44);
        } else {
          gprint("BAR5 IS ZERO (NOT CONFIGURED)\n", 0xFF8800);
        }
      } else {
        gprint("NO STORAGE CTRL (AHCI/NVMe) FOUND\n", 0xFF4444);
      }
    } else {
      /* plain "pci" command */
      extern void pci_print_results(void);
      pci_print_results();
    }
  }
  /* --- Waveform Zoom Commands --- */
  else if (cmd[0] == 'z' && cmd[1] == 'o' && cmd[2] == 'o' && cmd[3] == 'm') {
    if (cmd[4] == '+') {
      if (zoom_level < 4)
        zoom_level++;
      gprint("ZOOM: ", 0x00FFFF);
      gprint_dec(zoom_level, 0xFFFFFF);
      gprint("x\n", 0x00FFFF);
    } else if (cmd[4] == '-') {
      if (zoom_level > 1) {
        zoom_level--;
        zoom_offset = 0;
      }
      gprint("ZOOM: ", 0x00FFFF);
      gprint_dec(zoom_level, 0xFFFFFF);
      gprint("x\n", 0x00FFFF);
    } else {
      gprint("USE: zoom+ zoom-\n", 0xFFFF00);
    }
  } else if (cmd[0] == 'z' && cmd[1] == 'p' && cmd[2] == 'a' && cmd[3] == 'n') {
    int step = NEURON_COUNT / zoom_level;
    if (step < 1)
      step = 1;
    if (cmd[4] == '+') {
      zoom_offset += step;
      if (zoom_offset > NEURON_COUNT - step)
        zoom_offset = NEURON_COUNT - step;
    } else if (cmd[4] == '-') {
      zoom_offset -= step;
      if (zoom_offset < 0)
        zoom_offset = 0;
    }
    gprint("PAN OFFSET: ", 0x00FFFF);
    gprint_dec(zoom_offset, 0xFFFFFF);
    gprint("\n", 0);
  } else if (cmd[0] == 'w' && cmd[1] == 'i' && cmd[2] == 'p' && cmd[3] == 'e') {
    kprint("WIPING ALL NEURAL DATA...", current_shell_row, 0, 0x0E);
    if (current_shell_row >= SHELL_MAX_ROW)
      scroll_shell();
    else
      current_shell_row++;

    // Create a local zeroed-out buffer (256 uint16_t = 512 bytes)
    uint16_t zero_buffer[256];
    for (int i = 0; i < 256; i++) {
      zero_buffer[i] = 0;
    }

    // 1. Clear the TFS root directory (LBA 150)
    disk_write_sector(TFS_DIR_LBA, zero_buffer);
    // Also clear the in-RAM directory
    for (int i = 0; i < TFS_MAX_FILES; i++) {
      root_directory[i].flags = 0;
      root_directory[i].name[0] = '\0';
      root_directory[i].lba = 0;
      root_directory[i].size = 0;
    }

    // 2. Clear data sectors (LBA 200 through 200+TFS_MAX_FILES)
    for (int s = 0; s < TFS_MAX_FILES; s++) {
      disk_write_sector(TFS_DATA_START + s, zero_buffer);
    }

    // 3. Also reset the live potentials in RAM
    for (int i = 0; i < NEURON_COUNT; i++) {
      potentials[i] = 0;
    }

    kprint("DIR + DATA + RAM WIPED.", current_shell_row, 0, 0x0A);
  } else {
    video[line3 + 20] = 0x4F00 | 'N';
    video[line3 + 21] = 0x4F00 | 'O';
    video[line3 + 22] = 0x4F00 | 'N';
    video[line3 + 23] = 0x4F00 | 'E';

    video[output_offset + 0] = 0x4F00 | '?';
    video[output_offset + 1] = 0x4F00 | '?';
    video[output_offset + 2] = 0x4F00 | '?';
  }

  // Advance to next line
  if (current_shell_row >= SHELL_MAX_ROW) {
    scroll_shell();
  } else {
    current_shell_row++;
  }

  // Write prompt
  video[(current_shell_row * 80) + 0] = 0x0F00 | '>';
  video[(current_shell_row * 80) + 1] = 0x0F00 | ' ';
}

#endif

void kprint(const char *str, int row, int col, unsigned char color) {
  unsigned short *video = (unsigned short *)0xB8000;

  int offset = (row * 80) + col;
  for (int i = 0; str[i] != '\0' && (col + i) < 80; i++) {
    video[offset + i] = (color << 8) | str[i];
  }
}

extern void gprint_dec(int val, uint32_t color);

static void ahci_print_boot_report(const AhciProbeReport *report,
                                   int graphics_mode) {
  if (report == 0 || !report->controller_found) {
    if (graphics_mode) {
      gprint("AHCI: no controller found\n", 0xCCCCCC);
    } else {
      kprint("AHCI: no controller found", 2, 0, 0x07);
    }
    return;
  }

  if (graphics_mode) {
    gprint("AHCI HBA @ B", 0x66FFCC);
    gprint_hex(report->bus, 2, 0xFFFFFF);
    gprint(":D", 0x66FFCC);
    gprint_hex(report->slot, 2, 0xFFFFFF);
    gprint(":F", 0x66FFCC);
    gprint_hex(report->function, 2, 0xFFFFFF);
    gprint(" ABAR=", 0x66FFCC);
    gprint_hex(report->abar, 8, 0xFFFFFF);
    gprint("\n", 0x000000);

    gprint("AHCI CAP=", 0x55CCFF);
    gprint_hex(report->cap, 8, 0xFFFFFF);
    gprint(" PI=", 0x55CCFF);
    gprint_hex(report->pi, 8, 0xFFFFFF);
    gprint(" VS=", 0x55CCFF);
    gprint_hex(report->version, 8, 0xFFFFFF);
    gprint("\n", 0x000000);

    gprint("AHCI ports ready ", 0x88FFAA);
    gprint_dec(report->ready_ports, 0xFFFFFF);
    gprint("/", 0x88FFAA);
    gprint_dec(report->implemented_ports, 0xFFFFFF);
    gprint(report->ahci_enabled ? " (AE=ON)\n" : " (AE=OFF)\n", 0x88FFAA);

    for (int p = 0; p < AHCI_MAX_PORTS; p++) {
      const AhciPortReport *pr = &report->ports[p];
      if (!pr->implemented) {
        continue;
      }

      gprint("  P", 0xAAAAAA);
      gprint_hex(pr->port_no, 2, 0xFFFFFF);
      gprint(" DET=", 0xAAAAAA);
      gprint_hex(pr->det, 1, 0xFFFFFF);
      gprint(" IPM=", 0xAAAAAA);
      gprint_hex(pr->ipm, 1, 0xFFFFFF);
      gprint(" TFD=", 0xAAAAAA);
      gprint_hex(pr->tfd, 8, 0xFFFFFF);
      gprint(" SIG=", 0xAAAAAA);
      gprint_hex(pr->sig, 8, 0xFFFFFF);
      gprint(pr->ready ? " READY\n" : " NOT_READY\n",
             pr->ready ? 0x55FF88 : 0xFFCC55);
    }
  } else {
    kprint("AHCI: controller present (see graphics log for details)", 2, 0,
           0x0F);
  }
}

/* 6. Kernel Entry Point */
extern void pci_scan_all(); // Added for PCI Discovery
extern void flip_buffer();
extern void clear_screen(uint32_t color);
extern void put_pixel(int x, int y, uint32_t color);

__attribute__((section(".text.entry"))) void kernel_main(void) {

  /* Bootloader may leave IF enabled; block IRQs until IDT is ready. */
  __asm__ volatile("cli");

  /* Deterministic startup: clear BSS even if firmware/loader doesn't. */
  extern uint32_t __bss_start;
  extern uint32_t __bss_end;
  {
    uint8_t *bss = (uint8_t *)&__bss_start;
    uint32_t bss_size = (uint32_t)&__bss_end - (uint32_t)&__bss_start;
    for (uint32_t i = 0; i < bss_size; i++) {
      bss[i] = 0;
    }
  }

  /* Set segments - already established as safe */
  __asm__ volatile("mov $0x10, %%ax\n"
                   "mov %%ax, %%ds\n"
                   "mov %%ax, %%es\n"
                   "mov %%ax, %%fs\n"
                   "mov %%ax, %%gs\n"
                   "mov %%ax, %%ss\n"
                   :
                   :
                   : "ax");

  // Read VBE handoff values from low memory populated by bootloader:
  // 0x500: framebuffer address, 0x504: pitch(bytes per scanline),
  // 0x508: bits per pixel.
  vbe_framebuffer = *((uint32_t *)0x500);
  vbe_pitch = *((uint32_t *)0x504);
  vbe_bpp = *((uint32_t *)0x508);
  graphics_enabled =
      (vbe_framebuffer != 0) && (vbe_bpp == 24 || vbe_bpp == 32);

  // Load kernel-space GDT immediately so we don't depend on the bootloader's
  // GDT at 0x7C00 (which gets overwritten by BSS/page tables during
  // init_paging)
  load_kernel_gdt();

  // --- Hardware Discovery ---
  // NOTE: pci_scan_all is called AFTER init_idt (below) to ensure
  // exception handlers are in place. Moving it here (before IDT) caused
  // a triple fault because any exception had no handler.

  // --- Explicit .bss initialization (not zeroed by bootloader) ---
  buffer_idx = 0;
  for (int i = 0; i < COMMAND_MAX_LEN; i++)
    input_buffer[i] = 0;
  current_shell_row = SHELL_START_ROW;
  for (int i = 0; i < MAX_FILES; i++) {
    synapse_disk[i].is_used = 0;
    for (int j = 0; j < FILENAME_LEN; j++)
      synapse_disk[i].name[j] = 0;
    for (int j = 0; j < NEURONS_PER_PIXEL; j++) {
      synapse_disk[i].voltages[j] = 0;
      synapse_disk[i].weights[j] = 0;
      synapse_disk[i].thresholds[j] = 0;
    }
  }

  // --- Initialize the Pixelated Memory Map ---
  for (int p = 0; p < PIXELS_COUNT; p++) {
    os_memory_map[p].current_phase = 0; // Start in FLUID phase
    os_memory_map[p].pixel_recent_spikes = 0;

    for (int i = 0; i < NEURONS_PER_PIXEL; i++) {
      os_memory_map[p].neurons[i].voltage = 0;
      os_memory_map[p].neurons[i].spike_count = 0;
      os_memory_map[p].neurons[i].synaptic_weight = 100; // Base weight
      os_memory_map[p].neurons[i].refractory_timer = 0;
      // ID can be unique across the whole system: (pixel_index * size +
      // neuron_index)
      os_memory_map[p].neurons[i].id = (p * NEURONS_PER_PIXEL) + i;
    }
  }

  // --- Initialize the Task Control Blocks (TCB) ---

  // Task 0: High-Response I/O (Assigned to Pixel 0)
  task_list[0].task_id = 0;
  task_list[0].task_name = "IO  ";
  task_list[0].base_integration = 30; // Fast response for user input
  task_list[0].target_pixel = 0;

  // Task 1: Background Computation (Assigned to Pixel 1)
  task_list[1].task_id = 1;
  task_list[1].task_name = "COMP";
  task_list[1].base_integration = 60; // Slow, stable pulse for systems
  task_list[1].target_pixel = 1;

  // ---  Initialize TCB Saved States for Neural Persistence ---
  for (int t = 0; t < 2; t++) {
    // Set task metadata
    task_list[t].priority = (t == 0) ? 1 : 0;
    task_list[t].fire_threshold = THRESHOLD;
    // Initialize state snapshots for Neural Persistence
    for (int n = 0; n < NEURONS_PER_PIXEL; n++) {
      task_list[t].saved_voltages[n] = 0;
      task_list[t].saved_weights[n] = 100; // Match starting memory map weights
      // Ensure tasks start with the default baseline threshold
      // This is critical for the Adaptive Thresholding logic
      os_memory_map[t].neurons[n].dynamic_threshold = THRESHOLD;
      task_list[t].saved_thresholds[n] = THRESHOLD;
    }
  }

  /* Model parameters must be initialized before IRQ0 starts firing. */
  model_manager_init();

  /* Setup the NeuroCore pulse */
  init_idt();      /* Register timer_handler and enable interrupts FIRST */
  init_timer(100); /* 100Hz frequency - now IDT is ready for IRQ0 */
  init_input_stack();

  /* Detect ATA disk controller */
  ata_disk_available = ata_detect_disk();

  vfs_init();
  profile_init();
  storage_manifest_load("/session.manifest");

  if (graphics_enabled) {
    /* Hardware graphics init */
    init_graphics();
    clear_screen(0x000044); // NeuroSpark dark blue

    /* Reset cursor position for early boot logo */
    extern int cursor_x;
    extern int cursor_y;
    cursor_x = 0;
    cursor_y = 10;

    /* Print Status graphically */
    gprint("TIMER ACTIVE - NEURO CORE PULSING\n", 0x00FFFF);

    /* Flush the backbuffer to VRAM so the boot splash is visible immediately.
       At this point paging is NOT yet enabled, so direct VRAM access works. */
    flip_buffer();

    /* the WM desktop layer instead of the old dashboard renderer. */
    wm_init();
  } else {
    kprint("TEXT MODE BOOT (NO VBE)", 0, 0, 0x0F);
  }

  init_pmm();

  {
    uint32_t isr_stack = (uint32_t)pmm_alloc_page();
    if (isr_stack != 0) {
      tss_set_kernel_stack(isr_stack + 4096);
    }
  }

  init_paging(); // The "Nervous System" is now active — 8MB identity mapped

  os_tasks[0].page_dir = (uint32_t)page_directory;

  if (graphics_enabled) {
    /* Explicitly initialize graphics cursor (BSS may not be zeroed by bootloader)
     */
    extern int cursor_x;
    extern int cursor_y;
    cursor_x = 0;
    cursor_y = 120; /* Below status bar (top 100px) */
  }

  /* PCI scan now happens safely AFTER IDT is set up */
  pci_scan_all();

  {
    AhciProbeReport ahci_report;
    ahci_probe_and_enumerate(&ahci_report);
    ahci_print_boot_report(&ahci_report, graphics_enabled);
  }

  net_init();

  void init_syscalls();
  init_syscalls(); // Plug in 'int 0x80'

  // 2. Prepare Task 0: The Shell
  // Task 0 is 'born' already running because it's the current thread
  task_list[0].state = 0; // Running
  task_list[0].task_id = 0;

  // 3. Prepare Task 1: The NeuroCore (graphics mode only)
  if (graphics_enabled) {
    create_task(1, neuro_task_entry, (uint32_t)page_directory);
  }

  if (AUTO_LAUNCH_USER_PROCESS) {
    if (AUTO_EXEC_DISK_ON_BOOT && exec_user_program("/demo.bin")) {
      if (graphics_enabled)
        gprint("USER MODE EXEC /demo.bin\n", 0x44FF88);
      else
        kprint("USER MODE EXEC /demo.bin", 1, 0, 0x0A);
    } else if (launch_user_process_task()) {
      if (graphics_enabled)
        gprint("USER MODE PROCESS ONLINE\n", 0x44FF88);
      else
        kprint("USER MODE PROCESS ONLINE", 1, 0, 0x0A);
    } else {
      if (graphics_enabled)
        gprint("USER MODE PROCESS FAILED\n", 0xFF4444);
      else
        kprint("USER MODE PROCESS FAILED", 1, 0, 0x0C);
    }
  } else {
    if (graphics_enabled)
      gprint("USER MODE PROCESS DEFERRED\n", 0xFFCC44);
    else
      kprint("USER MODE PROCESS DEFERRED", 1, 0, 0x0E);
  }

  kprint("MULTITASKING KERNEL ACTIVE", 0, 0, 0x0E); // Yellow text

    if (graphics_enabled) {
     /* Flush backbuffer → VRAM once more so everything printed during
       post-paging init is visible before the task loop starts.       */
     flip_buffer();
    }

  /* Note: The shell task stays in this infinite loop, but yields CPU execution
   */
  /* to allow the background graphics task to render the UI each cycle. */
  extern void schedule();
  while (1) {
    net_rx_poll();
    schedule();
    __asm__ volatile("hlt");
  }
}

void pulse_neurons() {
  for (int i = 0; i < NEURON_COUNT; i++) {
    // 1. Leak: Natural decay of the membrane potential
    if (potentials[i] > 0) {
      potentials[i] -= LEAK_RATE;
    }

    // 2. Integration: Add a small charge to simulate background activity
    // In a real simulation, this would come from an input source
    potentials[i] += 5;

    // 3. Fire: If potential hits the threshold, the neuron 'spikes'
    if (potentials[i] >= THRESHOLD) {
      potentials[i] = 0; // Reset after firing

      // Visual feedback: Print a small indicator at the top right of the screen
      // 0x0A is Green, 0x0F is White
      kprint("*", 0, 75 + (i % 4), 0x0A);
    }
  }
}

/* VESA Graphics Globals */
// This should be set by bootloader ideally, or hardcoded for now
uint32_t vbe_framebuffer = 0xFD000000;
uint32_t vbe_pitch = 0;
uint32_t vbe_bpp = 32;
int screen_width = 800;
int screen_height = 600;