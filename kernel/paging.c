typedef unsigned int uint32_t;

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

// Helper to map a physical address to a virtual address
void map_page(uint32_t phys_addr, uint32_t virt_addr) {
  uint32_t pd_index = virt_addr >> 22;
  uint32_t pt_index = (virt_addr >> 12) & 0x03FF;

  uint32_t *page_table;

  if ((page_directory[pd_index] & 0x1) == 0) {
    uint32_t new_table_phys = (uint32_t)pmm_alloc_page();
    page_table = (uint32_t *)new_table_phys;
    for (int i = 0; i < 1024; i++)
      page_table[i] = 0;
    page_directory[pd_index] = new_table_phys | 0x3;
  } else {
    page_table = (uint32_t *)(page_directory[pd_index] & 0xFFFFF000);
  }

  page_table[pt_index] = (phys_addr & 0xFFFFF000) | 0x3;
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