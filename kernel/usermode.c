#include "usermode.h"

#include "paging.h"
#include "scheduler.h"
#include "task.h"

typedef unsigned char uint8_t;

enum {
  USER_VA_CODE = 0x40000000,
  USER_VA_STACK_TOP = 0x40002000,
};

extern void *pmm_alloc_page(void);
extern void create_task(int index, void (*func_ptr)(), uint32_t page_dir);
extern void map_page(uint32_t phys_addr, uint32_t virt_addr);
extern void enter_user_mode(uint32_t entry_eip, uint32_t user_stack, uint32_t page_dir);
extern uint8_t user_program_start;
extern uint8_t user_program_end;

static uint32_t user_page_dir = 0;
static uint32_t user_entry = USER_VA_CODE;
static uint32_t user_stack_top = USER_VA_STACK_TOP - 16;

static void mem_zero(uint8_t *dst, uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    dst[i] = 0;
  }
}

static void mem_copy(uint8_t *dst, const uint8_t *src, uint32_t size) {
  for (uint32_t i = 0; i < size; i++) {
    dst[i] = src[i];
  }
}

static int setup_user_address_space(void) {
  uint8_t *code_start = &user_program_start;
  uint8_t *code_end = &user_program_end;
  uint32_t code_size = (uint32_t)(code_end - code_start);

  if (code_size == 0 || code_size > 4096) {
    return 0;
  }

  user_page_dir = create_user_page_directory();
  if (user_page_dir == 0) {
    return 0;
  }

  uint32_t code_phys = (uint32_t)pmm_alloc_page();
  uint32_t stack_phys = (uint32_t)pmm_alloc_page();
  if (code_phys == 0 || stack_phys == 0) {
    return 0;
  }

  // Ensure newly allocated physical pages are visible in the active kernel map.
  map_page(code_phys, code_phys);
  map_page(stack_phys, stack_phys);

  mem_zero((uint8_t *)code_phys, 4096);
  mem_zero((uint8_t *)stack_phys, 4096);
  mem_copy((uint8_t *)code_phys, code_start, code_size);

  if (!map_user_page(user_page_dir, code_phys, USER_VA_CODE)) {
    return 0;
  }
  if (!map_user_page(user_page_dir, stack_phys, USER_VA_STACK_TOP - 4096)) {
    return 0;
  }

  return 1;
}

static void user_task_entry(void) {
  enter_user_mode(user_entry, user_stack_top, user_page_dir);
  task_terminate_current();
}

int launch_user_process_task(void) {
  if (!setup_user_address_space()) {
    return 0;
  }

  create_task(2, user_task_entry, user_page_dir);
  return 1;
}
