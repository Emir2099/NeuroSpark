#ifndef USERMODE_H
#define USERMODE_H

typedef unsigned int uint32_t;

int launch_user_process_task(void);
int exec_user_program(const char *path);
void tss_set_kernel_stack(uint32_t kernel_stack_top);

#endif
