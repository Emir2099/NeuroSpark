/* Typedefs */
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

/* Declare external functions */
extern void set_idt_gate(int n, uint32_t handler, uint16_t sel, uint8_t flags);
extern void syscall_wrapper(); 

void init_syscalls() {
    // 0x80 = 128 in decimal
    // 0x08 = Kernel Code Segment selector
    // 0x8E = Type: 32-bit Interrupt Gate
    // DPL (Privilege) must be 3 for tasks to call it
    set_idt_gate(0x80, (uint32_t)syscall_wrapper, 0x08, 0xEE); 
}