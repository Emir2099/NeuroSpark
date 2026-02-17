/* 1. Global Definitions and Types */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

/* External assembly wrapper for the timer */
extern void timer_wrapper(void);
extern void keyboard_wrapper(void);

// External PMM functions
extern void init_pmm();
extern void* pmm_alloc_page();
extern void pmm_free_page(uint32_t page_addr);

#define THRESHOLD 1000  // Membrane potential required to spike
#define DECAY 5         // Voltage lost per clock tick (leaky behavior)
#define GRID_SIZE 10    // Number of neurons in our initial grid
#define REFRACTORY_TICKS 10 // 10 ticks = 100ms of "silence" after a spike
#define CRITICAL_THRESHOLD 15 // Spikes per second to trigger phase change
#define PIXELS_COUNT 2
#define NEURONS_PER_PIXEL 5
#define TASK_IO 0
#define TASK_COMPUTE 1
#define GLOBAL_MAX_SPIKES 5000 // If total spikes exceed this, initiate decay
#define SYSTEM_DECAY_RATE 10   // Amount of synaptic weight lost during global decay

#define MAX_FILES 4
#define FILENAME_LEN 8

#define COMMAND_MAX_LEN 32

#define SHELL_START_ROW 15
#define SHELL_MIN_ROW 15  
#define SHELL_MAX_ROW 24

#include "disk.h"

volatile int current_shell_row = SHELL_START_ROW;

int recent_spikes = 0;
uint8_t current_bg_color = 0x1F; // Default Blue

// Forward declarations
void update_monitor();
void process_command(char *cmd);
void kprint(const char *str, int row, int col, unsigned char color);
void scroll_shell();
void sys_save_task(int task_id, int slot);
int sys_load_task(int task_id, int slot);

typedef struct {
    int voltage;        /* Current membrane potential */
    int spike_count;    /* Total spikes fired by this neuron */
    int id;             /* Unique identifier */
    int synaptic_weight; /* Strength of connection to the next neuron */
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
    int priority;        /* 0 = Low, 1 = High */
    int target_pixel;    /* Which cluster this task is currently assigned to */
    
    // Process-specific "Biological" parameters
    int base_integration; 
    int fire_threshold;
    
    const char* task_name;

    // Stores the 'learned' weights and current potential for 5 neurons
    int saved_voltages[NEURONS_PER_PIXEL];
    int saved_weights[NEURONS_PER_PIXEL];

    int saved_thresholds[NEURONS_PER_PIXEL]; // For Adaptive Thresholding

    int last_spike_count;    /* Spike count from the previous second */
    int spikes_per_second;   /* Calculated real-time throughput */
} TaskControlBlock;

// For now, let's define two tasks
TaskControlBlock task_list[2];



typedef struct {
    char name[FILENAME_LEN];
    int is_used;
    // Data payload: The saved state of 5 neurons
    int voltages[NEURONS_PER_PIXEL];
    int weights[NEURONS_PER_PIXEL];
    int thresholds[NEURONS_PER_PIXEL];
} VirtualFile;

volatile VirtualFile synapse_disk[MAX_FILES];


volatile char input_buffer[COMMAND_MAX_LEN];
volatile int buffer_idx = 0;
int shell_line = 15; // Start the shell on line 16 (80 * 15) to avoid the neural grid


/* 2. Low-level Hardware Helpers */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

// Additional I/O functions for disk operations
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

/* 3. The IDT Entry Structure */
struct idt_entry {
    uint16_t base_lo;
    uint16_t sel;        /* Kernel segment selector (0x08) */
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_hi;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct idt_entry idt[256];
struct idt_ptr idtp;

/* 4. Interrupt Logic */
void set_idt_gate(int n, uint32_t handler) {
    idt[n].base_lo = handler & 0xFFFF;
    idt[n].sel = 0x08;     /* Code segment from your GDT */
    idt[n].always0 = 0;
    idt[n].flags = 0x8E;   /* 32-bit Interrupt Gate, Present, Ring 0 */
    idt[n].base_hi = (handler >> 16) & 0xFFFF;
}

void init_idt() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;

    /* Remap the PIC (Programmable Interrupt Controller) */
    /* Hardware interrupts IRQ 0-7 normally conflict with CPU exceptions. */
    /* We remap them to start at IDT index 32. */
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    
    /* Mask (disable) all IRQs except what we need:
     * Master PIC (0x21): bit0=IRQ0(timer), bit1=IRQ1(keyboard)
     *   0xFC = 11111100 → only IRQ 0 and IRQ 1 unmasked
     * Slave PIC (0xA1): mask everything (0xFF)
     *   Especially IRQ 14 (IDE) which has no handler and would triple-fault
     */
    outb(0x21, 0xFC);  outb(0xA1, 0xFF);

    /* Register our timer wrapper (IRQ0 -> Index 32) */
    set_idt_gate(32, (uint32_t)timer_wrapper);

    set_idt_gate(33, (uint32_t)keyboard_wrapper); // Keyboard

    /* Load the IDT into the CPU and enable interrupts */
    __asm__ volatile("lidt (%0)" : : "r" (&idtp));
    __asm__ volatile("sti");
}

/* 5. Timer Implementation */
uint32_t tick = 0;
void timer_handler(void) {
    tick++;
    unsigned short *video = (unsigned short *)0xB8000;

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
            // If the pixel is RIGID (Phase 1), we still use a hardcoded dampener (10) to ensure stability.
            // If it is FLUID (Phase 0), we use the dynamic 'base_integration' from the TCB.
            int base_gain = (os_memory_map[p].current_phase == 1) ? 10 : current_task->base_integration;
            
            // --- CONSOLIDATED INTEGRATION LOGIC ---
            // We apply task-specific rhythms based on the TASK assigned to this pixel
            if (current_task->task_id == TASK_IO) {
                // I/O Pixel: Fires based on high-frequency external stimulus
                if (tick % (i + 2) == 0) n->voltage += base_gain; 
            } else {
                // Compute Pixel: Fires in a slow, stable rhythmic pulse
                // if (tick % 20 == 0) n->voltage += (base_gain / 2);
                if (tick % 4 == 0) n->voltage += 25;
            }

            // 2. Leakage (Bio-decay)
            if (n->voltage > 0) n->voltage -= DECAY;

            // 2.5 Weight Decay (Homeostasis)
            if (tick % 100 == 0 && n->synaptic_weight > 100) {
                n->synaptic_weight -= 5; 
            }

            // 3. Fire & Synaptic Propagation
            // Use n->dynamic_threshold instead of the THRESHOLD constant
            if (n->voltage >= n->dynamic_threshold) {
                n->voltage = 0;
                n->spike_count++;
                n->refractory_timer = REFRACTORY_TICKS; // Enter recovery
                
                // ADAPTATION: Increase threshold on every spike (Heavier to fire again)
                if (n->dynamic_threshold < 5000) {
                    n->dynamic_threshold += 200; 
                }


                // Propagate within the same pixel cluster
                int next = (i + 1) % NEURONS_PER_PIXEL;
                os_memory_map[p].neurons[next].voltage += n->synaptic_weight;
        
                // Hebbian Learning: Strengthen the synapse (Plasticity)
                if (n->synaptic_weight < 800) {
                    n->synaptic_weight += 10; 
                }

                unsigned char color = 0x1A; // Default Green on Blue
                if (n->synaptic_weight > 500) color = 0x1C; // Red on Blue
                else if (n->synaptic_weight > 300) color = 0x1E; // Yellow on Blue

                video[screen_pos] = (color << 8) | '!';
            } else {
                // --- HOMEOSTASIS ---
                // Slowly lower the threshold back to base THRESHOLD over time
                if (tick % 50 == 0 && n->dynamic_threshold > THRESHOLD) {
                    n->dynamic_threshold -= 50;
                }
        
                // Visualize potential levels
                if (n->voltage > 700) video[screen_pos] = 0x1700 | '^';
                else if (n->voltage > 300) video[screen_pos] = 0x1700 | '.';
                else video[screen_pos] = 0x1F20;  // Clear 
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
        // Visual indicator: Flash the background or a corner character to show throttling
        video[78] = 0x4C21; // Red '!' in the corner
    } else {
        video[78] = 0x1F20; // Clear indicator
    }

    if (tick % 10 == 0) { // Update monitor every 10 ticks to save cycles
        update_monitor();
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
    if (n == 0) { str[i++] = '0'; str[i] = '\0'; return; }
    if (n < 0) { is_negative = 1; n = -n; }
    while (n != 0) {
        str[i++] = (n % 10) + '0';
        n = n / 10;
    }
    if (is_negative) str[i++] = '-';
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
            attr = 0x4E; // Yellow text on RED
            pixel_msg = "RIGID ";

        } else {
            os_memory_map[p].current_phase = 0; // FLUID
            attr = 0x1F; // White on BLUE
            pixel_msg = "FLUID ";
        }

        // LOCAL STIFFNESS: Suppress only this pixel's voltage
        if (os_memory_map[p].current_phase == 1) {
            for(int i = 0; i < NEURONS_PER_PIXEL; i++) {
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

        // Clean up: Clear a few trailing characters to ensure old text doesn't ghost
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
        // We show the threshold of the first neuron in the pixel cluster as a representative
        itoa(os_memory_map[p].neurons[0].dynamic_threshold, thr_str);
        const char *thr_label = "THR: ";
        for (int i = 0; thr_label[i] != '\0'; i++) video[thr_offset + i] = 0x0700 | thr_label[i];
        for (int i = 0; thr_str[i] != '\0'; i++) video[thr_offset + 5 + i] = 0x0B00 | thr_str[i]; // Cyan
        video[thr_offset + 9] = 0x0700 | ' '; // Clear trailing digits
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

void switch_tasks() {
    // 1. SAVE current state - find which task owns which pixel first
    for (int p = 0; p < PIXELS_COUNT; p++) {
        // Find the task assigned to this pixel
        for (int t = 0; t < 2; t++) {
            if (task_list[t].target_pixel == p) {
                // Save this pixel's state to the correct task
                for (int n = 0; n < NEURONS_PER_PIXEL; n++) {
                    task_list[t].saved_voltages[n] = os_memory_map[p].neurons[n].voltage;
                    task_list[t].saved_weights[n] = os_memory_map[p].neurons[n].synaptic_weight;
                    // Save the current adaptation level
                    task_list[t].saved_thresholds[n] = os_memory_map[p].neurons[n].dynamic_threshold;
                }
                break;
            }
        }
    }

    // 2. SWAP the Task Control Blocks in the list
    TaskControlBlock temp = task_list[0];
    task_list[0] = task_list[1];
    task_list[1] = temp;

    // 3. Reset the target pixels to match the new array positions
    task_list[0].target_pixel = 0;
    task_list[1].target_pixel = 1;

    // 4. LOAD the new state into the Pixels (Neural Persistence)
    // Instead of clearing to 0, we restore the saved state of the incoming task.
     for (int p = 0; p < PIXELS_COUNT; p++) {
        // Find which task is now assigned to this pixel (after the swap)
        for (int t = 0; t < 2; t++) {
            if (task_list[t].target_pixel == p) {
                // Restore this task's saved state to the pixel
                for (int n = 0; n < NEURONS_PER_PIXEL; n++) {
                    os_memory_map[p].neurons[n].voltage = task_list[t].saved_voltages[n];
                    os_memory_map[p].neurons[n].synaptic_weight = task_list[t].saved_weights[n];
                    // Restore the adaptation level for the incoming task
                    os_memory_map[p].neurons[n].dynamic_threshold = task_list[t].saved_thresholds[n];
                }
                break;
            }
        }
    }

    // Clear voltages for a 'cold' context switch
    // This prevents residual potential from the previous task from 'polluting' the new one.
    // Commented out to enable Neural Persistence (State Saving).
    /* 
    for (int p = 0; p < PIXELS_COUNT; p++) {
        for (int n = 0; n < NEURONS_PER_PIXEL; n++) {
            os_memory_map[p].neurons[n].voltage = 0;
        }
    }
    */

}


char get_ascii(uint8_t scancode) {
    static char map[0x80] = {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
        '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
    };
    return map[scancode];
}


// ============================================================================
// NEURAL PERSISTENCE FUNCTIONS (Save/Load with Disk)
// ============================================================================

// SAVE: Copy a task's LIVE neural state into a "File"
void sys_save_task(int task_id, int slot) {
    if (slot >= MAX_FILES) return;
    
    volatile VirtualFile *f = &synapse_disk[slot];
    TaskControlBlock *t = &task_list[task_id];
    
    // Copy Metadata
    for(int i = 0; i < FILENAME_LEN; i++) f->name[i] = t->task_name[i];
    f->is_used = 1;
    
    // Copy LIVE Neural State from the pixel this task is assigned to
    int px = t->target_pixel;
    for(int n = 0; n < NEURONS_PER_PIXEL; n++) {
        f->voltages[n] = os_memory_map[px].neurons[n].voltage;
        f->weights[n] = os_memory_map[px].neurons[n].synaptic_weight;
        f->thresholds[n] = os_memory_map[px].neurons[n].dynamic_threshold;
    }
    
    // Force memory write before any further operations
    __asm__ volatile("" ::: "memory");
    
    // ===== WRITE TO DISK (only if controller is present) =====
    if (ata_disk_available) {
        disk_write_sector(DISK_DATA_OFFSET + slot, (uint16_t*)f);
    }
    
    // Visual feedback
    unsigned short *video = (unsigned short *)0xB8000;
    video[79] = ata_disk_available ? 0x2F44 : 0x2F4D; // 'D' for disk, 'M' for memory-only
}

// LOAD: Restore a task's persistence data from a "File"
// Returns 1 on success, 0 if slot is empty
int sys_load_task(int task_id, int slot) {
    if (slot >= MAX_FILES) return 0;
    
    // ===== READ FROM DISK (only if controller is present) =====
    if (ata_disk_available) {
        disk_read_sector(DISK_DATA_OFFSET + slot, (uint16_t*)&synapse_disk[slot]);
    }
    
    // Force memory read
    __asm__ volatile("" ::: "memory");
    
    // Check if the file exists
    int used_val = synapse_disk[slot].is_used;
    
    // Debug: show is_used value on line 12
    unsigned short *video_dbg = (unsigned short *)0xB8000;
    video_dbg[80*11 + 5] = 0x0C00 | 'L'; // Load
    video_dbg[80*11 + 6] = 0x0C00 | ('0' + slot);
    video_dbg[80*11 + 7] = 0x0C00 | ':';
    video_dbg[80*11 + 8] = 0x0C00 | ('0' + used_val);
    
    if (!used_val) return 0;
    
    volatile VirtualFile *f = &synapse_disk[slot];
    TaskControlBlock *t = &task_list[task_id];
    
    // Restore Neural State to LIVE pixel neurons
    int px = t->target_pixel;
    for(int n = 0; n < NEURONS_PER_PIXEL; n++) {
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
    // Read the scancode from the keyboard data port
    uint8_t scancode = 0;
    unsigned short *video = (unsigned short *)0xB8000;
    __asm__ volatile("inb $0x60, %0" : "=a"(scancode));
    
    if (scancode < 0x80) {
        char c = get_ascii(scancode);
        
        // --- 1. Control Keys (High Priority) ---
        if (scancode == 0x1C) { // ENTER Key
            input_buffer[buffer_idx] = '\0';
            
            // Copy buffer to local stack to avoid any volatile issues
            char cmd_copy[COMMAND_MAX_LEN];
            for (int ci = 0; ci <= buffer_idx && ci < COMMAND_MAX_LEN; ci++) {
                cmd_copy[ci] = input_buffer[ci];
            }
            cmd_copy[COMMAND_MAX_LEN - 1] = '\0'; // safety null
            
            process_command(cmd_copy);
            buffer_idx = 0;
            
            // Dont clear the line where we typed, let it stay visible!
            // The old text will scroll up naturally
            
            // Return early so ENTER doesn't get typed as a character
            video[80 * 5] = 0x0F00 | 'K'; 
            return;
        } 
        else if (scancode == 0x0E && buffer_idx > 0) { // BACKSPACE
            buffer_idx--;
            video[(current_shell_row * 80) + buffer_idx] = 0x0F20;
            video[80 * 5] = 0x0F00 | 'K';
            return;
        }
        
        // --- 2. Neural Probe & System Keys (Function Keys) ---
        // Use function keys to avoid conflicts with command typing
        if (scancode == 0x3B) { // F1 key for testing (was T)
            for(int n = 0; n < NEURONS_PER_PIXEL; n++) {
                os_memory_map[0].neurons[n].voltage += 300; 
            }
            video[80 * 5] = 0x0F00 | 'K';
            return;
        }
        if (scancode == 0x3C) { // F2 key for Switch (was S)
            switch_tasks();
            video[80 * 5] = 0x0F00 | 'K';
            return;
        }
        if (scancode == 0x3D) { // F3 key for zap (was Z)
            for(int n = 0; n < NEURONS_PER_PIXEL; n++) {
                os_memory_map[1].neurons[n].voltage += 1000; 
            }
            video[80 * 5] = 0x0F00 | 'K';
            return;
        }
        if (scancode == 0x3E) { // F4 key for quick save (was W)
            sys_save_task(0, 0);
            video[77] = 0x2F57; // Green 'W'
            video[80 * 5] = 0x0F00 | 'K';
            return;
        }
        if (scancode == 0x3F) { // F5 key for quick load (was L)
            sys_load_task(0, 0);
            video[77] = 0x1F4C; // Blue 'L'
            video[80 * 5] = 0x0F00 | 'K';
            return;
        }
        
        // --- 3. Standard Character Input ---
        // Every valid character (including 'l', 'w', 's') now reaches this check
        if (c && buffer_idx < COMMAND_MAX_LEN - 1) {
            input_buffer[buffer_idx++] = c;
            // Bounds check: only write if within shell zone
            if (current_shell_row >= SHELL_MIN_ROW && current_shell_row <= SHELL_MAX_ROW) {
                video[(current_shell_row * 80) + buffer_idx - 1] = 0x0F00 | c;
            }
        }
        // --- 4. Fallback (Default Action) ---
        else if (!c) {
            // Normal Operation: Stimulate the FIRST neuron of the FIRST pixel
            os_memory_map[0].neurons[0].voltage += 500;
        }
        
        // Visual feedback on the monitor line
        video[80 * 5] = 0x0F00 | 'K'; // Show 'K' for Keyboard event
    }
}

// Helper: Convert hex address to string for display
void hex_to_str(uint32_t n, char *str) {
    char *hex = "0123456789ABCDEF";
    str[0] = '0'; str[1] = 'x';
    for(int i = 7; i >= 0; i--) {
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
        if (byte >= '0' && byte <= '9') byte = byte - '0';
        else if (byte >= 'a' && byte <= 'f') byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <= 'F') byte = byte - 'A' + 10;
        else break;

        val = (val << 4) | (byte & 0xF);
        i++;
    }
    return val;
}

void process_command(char *cmd) {
    unsigned short *video = (unsigned short *)0xB8000;
    
    // ===== WRITE DIRECTLY TO LINE 3 =====
    int line3 = 80 * 3;
    
    // Clear line 3 completely first
    for(int i = 0; i < 80; i++) {
        video[line3 + i] = 0x4F20; // Red background, space
    }
    
    // Write "CMD:" in bright white on red
    video[line3 + 0] = 0x4F00 | 'C';
    video[line3 + 1] = 0x4F00 | 'M';
    video[line3 + 2] = 0x4F00 | 'D';
    video[line3 + 3] = 0x4F00 | ':';
    
    // Show entire command
    for(int i = 0; i < 10 && cmd[i] != '\0'; i++) {
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
    if (cmd[0] == 's' && cmd[1] == 'a' && cmd[2] == 'v' && cmd[3] == 'e' && cmd[4] == ' ') {
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
    else if (cmd[0] == 'l' && cmd[1] == 'o' && cmd[2] == 'a' && cmd[3] == 'd' && cmd[4] == ' ') {
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
    else if (cmd[0] == 'l' && cmd[1] == 's' && cmd[2] == '\0') {
        video[line3 + 20] = 0x0A00 | 'L';
        video[line3 + 21] = 0x0A00 | 'S';
        
        const char *msg = "FILES: 0:IO 1:COMP";
        for(int i = 0; msg[i] != '\0'; i++) {
            video[output_offset + i] = 0x0A00 | msg[i];
        }
    }
    // Command: "help"
    else if (cmd[0] == 'h' && cmd[1] == 'e' && cmd[2] == 'l' && cmd[3] == 'p' && cmd[4] == '\0') {
        video[line3 + 20] = 0x0700 | 'H';
        video[line3 + 21] = 0x0700 | 'E';
        video[line3 + 22] = 0x0700 | 'L';
        video[line3 + 23] = 0x0700 | 'P';
        
        const char *msg = "save 0, load 0, ls, help";
        for(int i = 0; msg[i] != '\0'; i++) {
            video[output_offset + i] = 0x0700 | msg[i];
        }
    }
    // Command: "clear"
    else if (cmd[0] == 'c' && cmd[1] == 'l' && cmd[2] == 'e' && cmd[3] == 'a' && cmd[4] == 'r' && cmd[5] == '\0') {
        video[line3 + 20] = 0x0A00 | 'C';
        video[line3 + 21] = 0x0A00 | 'L';
        video[line3 + 22] = 0x0A00 | 'R';
        
        for(int row = SHELL_MIN_ROW; row <= SHELL_MAX_ROW; row++) {
            for(int col = 0; col < 80; col++) {
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
        for(int n = 0; n < NEURONS_PER_PIXEL; n++) {
            // Potential = current voltage + synaptic weight (activity + learned strength)
            total_potential += os_memory_map[px].neurons[n].voltage;
            total_potential += os_memory_map[px].neurons[n].synaptic_weight;
            // Higher thresholds = higher resistance to spiking (Fluid/Rigid phase)
            total_resistance += os_memory_map[px].neurons[n].dynamic_threshold;
        }

        kprint("NEURAL EVALUATION:", current_shell_row, 0, 0x0B);
        if (current_shell_row >= SHELL_MAX_ROW) scroll_shell();
        else current_shell_row++;

        // Logic: If potential > resistance, the brain is "Active/Fluid"
        if (total_potential > total_resistance) {
            kprint(">> FLUID/ACTIVE <<", current_shell_row, 0, 0x2F);  // Bright white on green
        } else {
            kprint(">> RIGID/STABLE <<", current_shell_row, 0, 0x4E);  // Yellow on red
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
            for(int n = 0; n < NEURONS_PER_PIXEL; n++) {
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
                if (new_thr < 200) new_thr = 200;
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
    }
    else if (cmd[0] == 'm' && cmd[1] == 'a' && cmd[2] == 'l' && cmd[3] == 'l') {
        void* new_page = pmm_alloc_page();
        if(new_page) {
            char addr_str[11];
            hex_to_str((uint32_t)new_page, addr_str);
            kprint("ALLOCATED 4KB NEURAL PAGE AT: ", current_shell_row, 0, 0x0A);
            kprint(addr_str, current_shell_row, 30, 0x0F); // White address after label
        } else {
            kprint("ERR: OUT OF PHYSICAL MEMORY", current_shell_row, 0, 0x0C);
        }
    }
    else if (cmd[0] == 'f' && cmd[1] == 'r' && cmd[2] == 'e' && cmd[3] == 'e') {
    // Extract the address starting at index 5: "free 0x100000"
        uint32_t addr_to_free = str_to_hex(&cmd[5]);
    
        if (addr_to_free >= 0x100000) {
            pmm_free_page(addr_to_free);
            kprint("PAGE RELEASED AT: ", current_shell_row, 0, 0x0E); // Yellow
            kprint(&cmd[5], current_shell_row, 18, 0x0F);
        } else {
            kprint("ERR: INVALID OR PROTECTED ADDR", current_shell_row, 0, 0x0C); // Red
        }
    }
    else {
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



void kprint(const char *str, int row, int col, unsigned char color) {
    unsigned short *video = (unsigned short *)0xB8000;
        
    int offset = (row * 80) + col;
    for (int i = 0; str[i] != '\0' && (col + i) < 80; i++) {
        video[offset + i] = (color << 8) | str[i];
    }
}


/* 6. Kernel Entry Point */
__attribute__((section(".text.entry")))
void kernel_main(void) {
    /* Set segments - already established as safe */
    __asm__ volatile (
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        : : : "ax"
    );

    // --- Explicit .bss initialization (not zeroed by bootloader) ---
    buffer_idx = 0;
    for (int i = 0; i < COMMAND_MAX_LEN; i++) input_buffer[i] = 0;
    current_shell_row = SHELL_START_ROW;
    for (int i = 0; i < MAX_FILES; i++) {
        synapse_disk[i].is_used = 0;
        for (int j = 0; j < FILENAME_LEN; j++) synapse_disk[i].name[j] = 0;
        for (int j = 0; j < NEURONS_PER_PIXEL; j++) {
            synapse_disk[i].voltages[j] = 0;
            synapse_disk[i].weights[j] = 0;
            synapse_disk[i].thresholds[j] = 0;
        }
    }

    // --- Initialize the Pixelated Memory Map ---
    for (int p = 0; p < PIXELS_COUNT; p++) {
        os_memory_map[p].current_phase = 0;     // Start in FLUID phase
        os_memory_map[p].pixel_recent_spikes = 0;
        
        for (int i = 0; i < NEURONS_PER_PIXEL; i++) {
            os_memory_map[p].neurons[i].voltage = 0;
            os_memory_map[p].neurons[i].spike_count = 0;
            os_memory_map[p].neurons[i].synaptic_weight = 100; // Base weight
            os_memory_map[p].neurons[i].refractory_timer = 0;
            // ID can be unique across the whole system: (pixel_index * size + neuron_index)
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

    /* Setup the NeuroCore pulse */
    init_timer(100);  /* 100Hz frequency */
    init_idt();      /* Register timer_handler and start interrupts */

    /* Detect ATA disk controller */
    ata_disk_available = ata_detect_disk();

    /* Clear screen to blue */
    unsigned short *video_memory = (unsigned short *)0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        video_memory[i] = 0x1F20;
    }

    /* Print Status */
    const char *message = "TIMER ACTIVE - NEURO CORE PULSING";
    for (int i = 0; message[i] != '\0'; i++) {
        video_memory[i] = 0x1F00 | message[i];
    }
    init_pmm();
    while (1) {
        __asm__ volatile ("hlt");
    }
}