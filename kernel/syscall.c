#include "disk.h" // For FileEntry and types

/* Define syscall numbers */
#define SYS_PRINT 1

extern void kprint(const char *str, int row, int col, unsigned char color);
extern volatile int current_shell_row;

void syscall_handler(uint32_t *regs) {
    uint32_t syscall_num = regs[7]; // EAX is at the bottom of the pushad stack

    if (syscall_num == SYS_PRINT) {
        char *msg = (char *)regs[6]; // We'll pass the string address in EBX
        // Use your existing kprint logic
        // This ensures kprint only runs when the Kernel allows it
        kprint(msg, current_shell_row++, 0, 0x07); 
    }
}