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

int recent_spikes = 0;
uint8_t current_bg_color = 0x1F; // Default Blue

typedef struct {
    int voltage;        /* Current membrane potential */
    int spike_count;    /* Total spikes fired by this neuron */
    int id;             /* Unique identifier */
    int synaptic_weight; /* Strength of connection to the next neuron */
    int refractory_timer;  /* Ticks remaining in recovery */
} Neuron;

Neuron neural_grid[GRID_SIZE];


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

    for (int i = 0; i < GRID_SIZE; i++) {
        // --- Refractory Logic ---
        if (neural_grid[i].refractory_timer > 0) {
            neural_grid[i].refractory_timer--;
            neural_grid[i].voltage = 0; // Lock voltage at 0 during recovery
            
            // Visual: Show a '#' to indicate the neuron is "recovering"
            video[80 + i] = 0x1800 | '#'; 
            continue; // Skip integration/firing for this neuron
        }

        // 1. Integration (Background Noise)
        if (tick % (i + 5) == 0) neural_grid[i].voltage += 30;

        // 2. Leakage (Bio-decay)
        if (neural_grid[i].voltage > 0) neural_grid[i].voltage -= DECAY;

        // 2.5 Weight Decay (Homeostasis)
        // Every 100 ticks (~1 second), slightly weaken the synapses
        if (tick % 100 == 0 && neural_grid[i].synaptic_weight > 100) {
            neural_grid[i].synaptic_weight -= 5; 
        }

        // 3. Fire & Synaptic Propagation
        if (neural_grid[i].voltage >= THRESHOLD) {
            neural_grid[i].voltage = 0;
            neural_grid[i].spike_count++;
            neural_grid[i].refractory_timer = REFRACTORY_TICKS; // Enter recovery
            int next = (i + 1) % GRID_SIZE;
    
            // 1. Apply weighted stimulus to the next neuron
            neural_grid[next].voltage += neural_grid[i].synaptic_weight;
    
            // 2. Hebbian Learning: Strengthen the synapse (Plasticity)
            // We cap it at 800 so it doesn't become an infinite feedback loop
            if (neural_grid[i].synaptic_weight < 800) {
                neural_grid[i].synaptic_weight += 10; 
            }
            unsigned char color = 0x1A; // Default Green on Blue
            if (neural_grid[i].synaptic_weight > 500) color = 0x1C; // Red on Blue
            else if (neural_grid[i].synaptic_weight > 300) color = 0x1E; // Yellow on Blue

            video[80 + i] = (color << 8) | '!';
        } else {
            // Visualize potential levels with different characters
            if (neural_grid[i].voltage > 700) video[80 + i] = 0x1700 | '^';
            else if (neural_grid[i].voltage > 300) video[80 + i] = 0x1700 | '.';
            else video[80 + i] = 0x1F20;  // Clear 
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
    for (int i = 0; i < GRID_SIZE; i++) {
        total_spikes += neural_grid[i].spike_count;
    }

    // Calculate spikes in the last interval
    static int last_total = 0;
    recent_spikes = total_spikes - last_total;
    last_total = total_spikes;

    unsigned short *video = (unsigned short *)0xB8000;
    
    // --- UPDATED: Obvious Phase Change Logic with Global Stiffness ---
    const char *status_msg;
    unsigned char phase_attr;

    // Lowered to 5 for manual testing; if recent spikes > 5, we enter Critical Phase
    if (recent_spikes > 5) { 
        phase_attr = 0x4E; // Bright Yellow text on RED background
        status_msg = "PHASE: CRITICAL (RIGID)  ";

        // GLOBAL STIFFNESS: Actively suppress voltage to stabilize the system
        // This is the "Phase-Change Metamaterial" logic in action.
        for(int i = 0; i < GRID_SIZE; i++) {
            if (neural_grid[i].voltage > 200) {
                neural_grid[i].voltage -= 200; 
            }
        }
    } else {
        phase_attr = 0x1F; // White text on BLUE background
        status_msg = "PHASE: STABLE (FLUID)    ";
    }

    // Print the status message on line 7
    int phase_offset = 80 * 6; 
    for (int i = 0; status_msg[i] != '\0'; i++) {
        video[phase_offset + i] = (phase_attr << 8) | status_msg[i];
    }

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

void keyboard_handler(void) {
    // Read the scancode from the keyboard data port
    uint8_t scancode = 0;
    __asm__ volatile("inb $0x60, %0" : "=a"(scancode));

    // Scancodes below 0x80 are "key down" events
if (scancode < 0x80) {
        // Stimulate Neuron 0 with a massive 500mV jolt
            neural_grid[0].voltage += 500;

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

    for (int i = 0; i < GRID_SIZE; i++) {
    neural_grid[i].voltage = 0;
    neural_grid[i].spike_count = 0;
    neural_grid[i].id = i;
    neural_grid[i].synaptic_weight = 100; // Start with a base weight
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