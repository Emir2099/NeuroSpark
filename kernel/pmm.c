typedef unsigned char  uint8_t;
typedef unsigned int   uint32_t;

#define PAGE_SIZE 4096
#define MEMORY_MAX 0x2000000 // 32MB
#define BITMAP_SIZE (MEMORY_MAX / PAGE_SIZE / 8)

uint8_t memory_bitmap[BITMAP_SIZE];

// Logic to mark a page as used
void pmm_set_page(uint32_t page_addr) {
    uint32_t page = page_addr / PAGE_SIZE;
    memory_bitmap[page / 8] |= (1 << (page % 8));
}

// Logic to mark a page as free
void pmm_free_page(uint32_t page_addr) {
    uint32_t page = page_addr / PAGE_SIZE;
    // Safety check: Don't allow freeing the first 1MB (Kernel Space)
    if (page_addr < 0x100000) return; 
    
    memory_bitmap[page / 8] &= ~(1 << (page % 8));
}

// Logic to check status
int pmm_is_page_used(uint32_t page_addr) {
    uint32_t page = page_addr / PAGE_SIZE;
    return (memory_bitmap[page / 8] & (1 << (page % 8))) != 0;
}

// The allocator: finds and returns a pointer to a free 4KB page
void* pmm_alloc_page() {
    for (uint32_t i = 0; i < BITMAP_SIZE * 8; i++) {
        uint32_t addr = i * PAGE_SIZE;
        if (!pmm_is_page_used(addr)) {
            pmm_set_page(addr);
            return (void*)addr;
        }
    }
    return (void*)0; 
}

// Initializer to clear RAM and reserve the first 1MB for the kernel
void init_pmm() {
    for(int i = 0; i < BITMAP_SIZE; i++) memory_bitmap[i] = 0;
    for(uint32_t addr = 0; addr < 0x100000; addr += PAGE_SIZE) {
        pmm_set_page(addr);
    }
}