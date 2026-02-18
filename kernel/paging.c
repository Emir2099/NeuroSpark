typedef unsigned int uint32_t;

// Page Directory and one Page Table (mapping 4MB total)
// Aligned to 4096 bytes as required by x86 hardware
uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

/* Assembly function references */
extern void loadPageDirectory(unsigned int*);
extern void enablePaging();
void init_paging() {
    // 1. Clear Page Directory
    // Attribute 0x02 = Supervisor, Read/Write, Not Present
    for(int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002; 
    }

    // 2. Identity Map the first 4MB (0x0 to 0x400000)
    // This covers your Kernel, VGA, and PMM bitmap
    for(int i = 0; i < 1024; i++) {
        // Attribute 0x3 = Present, Read/Write
        first_page_table[i] = (i * 4096) | 3; 
    }

    // 3. Link the Table to the Directory
    page_directory[0] = ((unsigned int)first_page_table) | 3;

    // 4. Enable hardware paging
    loadPageDirectory(page_directory);
    enablePaging();
}