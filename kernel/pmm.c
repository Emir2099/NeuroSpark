typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

#define PAGE_SIZE 4096
#define MEMORY_MAX 0x2000000 // 32MB
#define BITMAP_SIZE (MEMORY_MAX / PAGE_SIZE / 8)

uint8_t memory_bitmap[BITMAP_SIZE];

/* External UI references from kernel.c */
extern int current_shell_row;
extern void kprint(char *str, int row, int col, int color);
extern void scroll_shell();
#define SHELL_MAX_ROW 24

// Logic to mark a page as used
void pmm_set_page(uint32_t page_addr) {
  uint32_t page = page_addr / PAGE_SIZE;
  memory_bitmap[page / 8] |= (1 << (page % 8));
}

// Logic to mark a page as free
void pmm_free_page(uint32_t page_addr) {
  uint32_t page = page_addr / PAGE_SIZE;
  // Safety check: Don't allow freeing the first 1MB (Kernel Space)
  if (page_addr < 0x100000)
    return;

  memory_bitmap[page / 8] &= ~(1 << (page % 8));
}

// Logic to check status
int pmm_is_page_used(uint32_t page_addr) {
  uint32_t page = page_addr / PAGE_SIZE;
  return (memory_bitmap[page / 8] & (1 << (page % 8))) != 0;
}

// The allocator: finds and returns a pointer to a free 4KB page
void *pmm_alloc_page() {
  for (uint32_t i = 0; i < BITMAP_SIZE * 8; i++) {
    uint32_t addr = i * PAGE_SIZE;
    if (!pmm_is_page_used(addr)) {
      pmm_set_page(addr);
      return (void *)addr;
    }
  }
  return (void *)0;
}

// Linker-provided symbol marking the end of the kernel BSS (page-aligned).
// PMM must not allocate anything below this address.
extern char _kernel_end;

// Initializer to clear RAM and reserve everything up to kernel BSS end
void init_pmm() {
  for (int i = 0; i < BITMAP_SIZE; i++)
    memory_bitmap[i] = 0;

  // Reserve all pages from 0 up to and including the kernel image + BSS.
  // _kernel_end is page-aligned by the linker script.
  uint32_t kernel_top = (uint32_t)&_kernel_end;
  for (uint32_t addr = 0; addr < kernel_top; addr += PAGE_SIZE) {
    pmm_set_page(addr);
  }
}

static void itoa_pmm(int value, char *buf) {
  if (value == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    return;
  }
  char tmp[12];
  int i = 0;
  while (value > 0) {
    tmp[i++] = '0' + (value % 10);
    value /= 10;
  }
  int j = 0;
  while (i > 0)
    buf[j++] = tmp[--i];
  buf[j] = '\0';
}

void pmm_print_map() {
  extern void gprint(char *str, unsigned int color);
  extern void gprint_dec(int val, unsigned int color);

  /* Row 1: Kernel zone overview (0-1MB, 16 blocks of 64KB each) */
  gprint("KERN 0-1M: ", 0xFFFF00);
  for (uint32_t i = 0; i < 16; i++) {
    uint32_t start_addr = i * 64 * 1024;
    int is_used = 0;
    for (uint32_t p = 0; p < 16; p++) {
      if (pmm_is_page_used(start_addr + (p * 4096))) {
        is_used = 1;
        break;
      }
    }
    if (is_used)
      gprint("#", 0xFF0000);
    else
      gprint(".", 0xAAAAAA);
  }
  gprint("\n", 0);

  /* Row 2: Allocation zone detail (1MB+, each char = one 4KB page) */
  gprint("USER 1M+ : ", 0xFFFF00);
  int alloc_used = 0, alloc_free = 0;
  for (uint32_t i = 0; i < 60; i++) {
    uint32_t addr = 0x100000 + (i * PAGE_SIZE);
    if (pmm_is_page_used(addr)) {
      gprint("#", 0x00FF00);
      alloc_used++;
    } else {
      gprint(".", 0x005500);
      alloc_free++;
    }
  }
  gprint("\n", 0);

  /* Row 3: Summary counts */
  gprint("USED:", 0x00FF00);
  char num[12];
  itoa_pmm(alloc_used, num);
  gprint(num, 0xFFFFFF);
  gprint(" FREE:", 0xAAAAAA);
  itoa_pmm(alloc_free, num);
  gprint(num, 0xFFFFFF);
  gprint(" (each char=4KB)\n", 0x555555);
}
/* Count free 4KB pages above the 1MB kernel reserve */
int pmm_get_free_pages() {
  int free_count = 0;
  for (uint32_t i = 0x100000 / PAGE_SIZE; i < BITMAP_SIZE * 8; i++) {
    uint32_t addr = i * PAGE_SIZE;
    if (!pmm_is_page_used(addr))
      free_count++;
  }
  return free_count;
}
