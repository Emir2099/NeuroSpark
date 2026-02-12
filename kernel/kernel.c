/* 1. Global Definitions and Types */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

/* External assembly wrapper for the timer */
extern void timer_wrapper(void);
extern void keyboard_wrapper(void);

#define THRESHOLD 1000  // Membrane potential required to spike
#define DECAY 5         // Voltage lost per clock tick (leaky behavior)
#define GRID_SIZE 10    // Number of neurons in our initial grid
#define REFRACTORY_TICKS 10 // 10 ticks = 100ms of "silence" after a spike
#define CRITICAL_THRESHOLD 15 // Spikes per second to trigger phase change
#define PIXELS_COUNT 2
#define NEURONS_PER_PIXEL 5
#define TASK_IO 0
#define TASK_COMPUTE 1

int recent_spikes = 0;
uint8_t current_bg_color = 0x1F; // Default Blue

typedef struct {
    int voltage;        /* Current membrane potential */
    int spike_count;    /* Total spikes fired by this neuron */
    int id;             /* Unique identifier */
    int synaptic_weight; /* Strength of connection to the next neuron */
    int refractory_timer;  /* Ticks remaining in recovery */
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
} TaskControlBlock;

// For now, let's define two tasks
TaskControlBlock task_list[2];

/* 2. Low-level Hardware Helpers */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
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
    outb(0x21, 0x0);  outb(0xA1, 0x0);

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
        TaskControlBlock *current_task = &task_list[p];
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
            // We apply task-specific rhythms based on the TCB assignment
            if (p == TASK_IO) {
                // I/O Pixel: Fires based on high-frequency external stimulus
                if (tick % (i + 2) == 0) n->voltage += base_gain; 
            } else {
                // Compute Pixel: Fires in a slow, stable rhythmic pulse
                if (tick % 20 == 0) n->voltage += (base_gain / 2);
            }

            // 2. Leakage (Bio-decay)
            if (n->voltage > 0) n->voltage -= DECAY;

            // 2.5 Weight Decay (Homeostasis)
            if (tick % 100 == 0 && n->synaptic_weight > 100) {
                n->synaptic_weight -= 5; 
            }

            // 3. Fire & Synaptic Propagation
            if (n->voltage >= THRESHOLD) {
                n->voltage = 0;
                n->spike_count++;
                n->refractory_timer = REFRACTORY_TICKS; // Enter recovery
                
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
                // Visualize potential levels
                if (n->voltage > 700) video[screen_pos] = 0x1700 | '^';
                else if (n->voltage > 300) video[screen_pos] = 0x1700 | '.';
                else video[screen_pos] = 0x1F20;  // Clear 
            }

            

            if (p == TASK_IO) {
            // I/O Pixel: Fires based on high-frequency external stimulus
                if (tick % (i + 2) == 0) n->voltage += base_gain; 
                } else {
                // Compute Pixel: Fires in a slow, stable rhythmic pulse
                if (tick % 20 == 0) n->voltage += (base_gain / 2);
            }
        }
    }

    // Keep the master heartbeat blinking in the corner
    video[79] = (tick % 20 < 10) ? 0x1F2A : 0x1F20; 
    
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
        // We use a wider spacing (30 chars) to prevent overlap on Line 7
        int offset = (80 * 6) + (p * 25);
        const char *t_name = task_list[p].task_name;

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
}

void switch_tasks() {
    // 1. SAVE current state of Pixel 0 to Task 0 and Pixel 1 to Task 1
    for (int p = 0; p < PIXELS_COUNT; p++) {
        for (int n = 0; n < NEURONS_PER_PIXEL; n++) {
            task_list[p].saved_voltages[n] = os_memory_map[p].neurons[n].voltage;
            task_list[p].saved_weights[n] = os_memory_map[p].neurons[n].synaptic_weight;
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
        for (int n = 0; n < NEURONS_PER_PIXEL; n++) {
            os_memory_map[p].neurons[n].voltage = task_list[p].saved_voltages[n];
            os_memory_map[p].neurons[n].synaptic_weight = task_list[p].saved_weights[n];
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

void keyboard_handler(void) {
    // Read the scancode from the keyboard data port
    uint8_t scancode = 0;
    __asm__ volatile("inb $0x60, %0" : "=a"(scancode));

    // Scancodes below 0x80 are "key down" events
    if (scancode < 0x80) {
        // --- Neural Probe (The 'T' key for testing) ---
        if (scancode == 0x14) { // 0x14 is the scancode for 'T'
            // Artificially stimulate ALL neurons in Pixel 0 to test Critical Phase
            // This allows testing without "destroying" biological constants.
            for(int n = 0; n < NEURONS_PER_PIXEL; n++) {
                os_memory_map[0].neurons[n].spike_count += 5; 
            }
        }
        else if (scancode == 0x1F) { // 'S' key for Switch
            switch_tasks();
        } 
        else {
            // Normal Operation: Stimulate the FIRST neuron of the FIRST pixel
            // This targets the specific cluster instead of a flat grid.
            os_memory_map[0].neurons[0].voltage += 500;
        }

        // Visual feedback on the monitor line
        unsigned short *video = (unsigned short *)0xB8000;
        video[80 * 5] = 0x0F00 | 'K'; // Show 'K' for Keyboard event
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
    task_list[1].base_integration = 15; // Slow, stable pulse for systems
    task_list[1].target_pixel = 1;

    // ---  Initialize TCB Saved States for Neural Persistence ---
    for (int t = 0; t < 2; t++) {
        // Set task metadata
        task_list[t].priority = (t == 0) ? 1 : 0; 
        task_list[t].fire_threshold = THRESHOLD;
        
        for (int n = 0; n < NEURONS_PER_PIXEL; n++) {
            // Ensure tasks start with 0 voltage and base synaptic weights
            task_list[t].saved_voltages[n] = 0;
            task_list[t].saved_weights[n] = 100; // Match starting memory map weights
        }
    }

    /* Setup the NeuroCore pulse */
    init_timer(100);  /* 100Hz frequency */
    init_idt();      /* Register timer_handler and start interrupts */

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

    while (1) {
        __asm__ volatile ("hlt");
    }
}