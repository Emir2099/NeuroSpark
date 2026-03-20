typedef unsigned int uint32_t;

#define PAGE_PRESENT 0x1
#define PAGE_RW 0x2
#define PAGE_USER 0x4
#define PAGE_MASK 0xFFFFF000

#define USER_VADDR_MIN 0x40000000U
#define USER_VADDR_MAX 0xBFFFFFFFU

// Page Directory and TWO Page Tables (mapping 8MB total)
// Aligned to 4096 bytes as required by x86 hardware
uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));
uint32_t second_page_table[1024] __attribute__((aligned(4096)));

/* Assembly function references */
extern void *pmm_alloc_page();
extern void loadPageDirectory(unsigned int *);
extern void enablePaging();

/* The VBE framebuffer physical address (set early in kernel_main
   from the bootloader's saved value BEFORE BSS is touched).       */
extern uint32_t vbe_framebuffer;

void map_page_flags(uint32_t phys_addr, uint32_t virt_addr, uint32_t flags);

// Helper to map a physical address to a virtual address
void map_page(uint32_t phys_addr, uint32_t virt_addr) {
  map_page_flags(phys_addr, virt_addr, PAGE_PRESENT | PAGE_RW);
}

void map_page_flags(uint32_t phys_addr, uint32_t virt_addr, uint32_t flags) {
  uint32_t pd_index = virt_addr >> 22;
  uint32_t pt_index = (virt_addr >> 12) & 0x03FF;

  uint32_t *page_table;

  if ((page_directory[pd_index] & 0x1) == 0) {
    uint32_t new_table_phys = (uint32_t)pmm_alloc_page();
    page_table = (uint32_t *)new_table_phys;
    for (int i = 0; i < 1024; i++)
      page_table[i] = 0;
    page_directory[pd_index] = (new_table_phys & PAGE_MASK) | PAGE_PRESENT | PAGE_RW;
  } else {
    page_table = (uint32_t *)(page_directory[pd_index] & PAGE_MASK);
  }

  page_table[pt_index] = (phys_addr & PAGE_MASK) | (flags & 0xFFF);
}

uint32_t create_user_page_directory(void) {
  uint32_t *new_pd = (uint32_t *)pmm_alloc_page();
  if (new_pd == (uint32_t *)0) {
    return 0;
  }

  for (int i = 0; i < 1024; i++) {
    uint32_t entry = page_directory[i];
    if (entry & PAGE_PRESENT) {
      // Keep kernel mappings supervisor-only when cloning into user PD.
      new_pd[i] = entry & (~PAGE_USER);
    } else {
      new_pd[i] = 0x00000002;
    }
  }

  return (uint32_t)new_pd;
}

int map_user_page(uint32_t pd_phys, uint32_t phys_addr, uint32_t virt_addr) {
  uint32_t *pd = (uint32_t *)(pd_phys & PAGE_MASK);
  if (pd == (uint32_t *)0) {
    return 0;
  }

  uint32_t pd_index = virt_addr >> 22;
  uint32_t pt_index = (virt_addr >> 12) & 0x03FF;

  uint32_t *page_table;
  if ((pd[pd_index] & PAGE_PRESENT) == 0) {
    uint32_t new_table_phys = (uint32_t)pmm_alloc_page();
    if (new_table_phys == 0) {
      return 0;
    }
    page_table = (uint32_t *)(new_table_phys & PAGE_MASK);
    for (int i = 0; i < 1024; i++) {
      page_table[i] = 0;
    }
    pd[pd_index] = (new_table_phys & PAGE_MASK) | PAGE_PRESENT | PAGE_RW | PAGE_USER;
  } else {
    page_table = (uint32_t *)(pd[pd_index] & PAGE_MASK);
  }

  page_table[pt_index] = (phys_addr & PAGE_MASK) | PAGE_PRESENT | PAGE_RW | PAGE_USER;
  return 1;
}

int is_user_range(const void *ptr, uint32_t size) {
  uint32_t start = (uint32_t)ptr;
  uint32_t end;

  if (ptr == (void *)0 || size == 0) {
    return 0;
  }

  end = start + size - 1;
  if (end < start) {
    return 0;
  }

  if (start < USER_VADDR_MIN || end > USER_VADDR_MAX) {
    return 0;
  }

  return 1;
}

void init_paging() {
  // 1. Clear Page Directory
  for (int i = 0; i < 1024; i++) {
    page_directory[i] = 0x00000002;
  }

  // 2. Identity Map the first 4MB (0x000000 – 0x3FFFFF)
  //    Covers kernel code/data, VGA text buffer, IDT, GDT, PMM bitmap
  for (int i = 0; i < 1024; i++) {
    first_page_table[i] = (i * 4096) | 3;
  }
  page_directory[0] = ((unsigned int)first_page_table) | 3;

  // 3. Identity Map the second 4MB (0x400000 – 0x7FFFFF)
  //    Critical: the graphics backbuffer[800*600] = 1.88MB of BSS
  //    sits here if the kernel + PMM bitmap exceed ~2MB.
  //    Without this mapping, any access to backbuffer[] above 0x400000
  //    causes a #PF which (with no double-fault handler) triple-faults.
  for (int i = 0; i < 1024; i++) {
    second_page_table[i] = (0x400000 + i * 4096) | 3;
  }
  page_directory[1] = ((unsigned int)second_page_table) | 3;

  // 4. Enable hardware paging with both tables in place
  loadPageDirectory(page_directory);
  enablePaging();

  // 5. Map the Framebuffer
  //    Use the global vbe_framebuffer (saved by kernel_main from 0x500
  //    before BSS was touched).  Map 2MB for 800x600x4.
  uint32_t fb_addr = vbe_framebuffer;
  if (fb_addr == 0)
    fb_addr = 0xFD000000; // fallback
  for (uint32_t i = 0; i < 512; i++) {
    uint32_t phys_addr = fb_addr + (i * 4096);
    map_page(phys_addr, phys_addr);
  }
}

void map_vbe_buffer(uint32_t physical_addr) {
  for (uint32_t i = 0; i < 512; i++) {
    uint32_t addr = physical_addr + (i * 4096);
    map_page(addr, addr);
  }
}

/* ============================================================
 * map_mmio_region – identity-map an arbitrary MMIO region
 * @phys:  physical base address (must be page-aligned)
 * @size:  region size in bytes (rounded up to 4KB pages)
 * ============================================================ */
void map_mmio_region(uint32_t phys, uint32_t size) {
  uint32_t pages = (size + 4095) / 4096;
  if (pages > 1024)
    pages = 1024; /* safety cap: 4MB max */
  for (uint32_t i = 0; i < pages; i++) {
    uint32_t addr = (phys & 0xFFFFF000) + (i * 4096);
    map_page(addr, addr);
  }
}