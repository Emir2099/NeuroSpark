typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

#define PAGE_SIZE 4096
#define MEMORY_MAX 0x2000000 // 32MB
#define BITMAP_SIZE (MEMORY_MAX / PAGE_SIZE / 8)

uint8_t memory_bitmap[BITMAP_SIZE];
static uint32_t pmm_alloc_calls = 0;
static uint32_t pmm_free_calls = 0;

typedef struct SlabNode {
  struct SlabNode *next;
} SlabNode;

typedef struct {
  uint32_t obj_size;
  SlabNode *free_list;
  uint32_t pages;
  uint32_t allocs;
  uint32_t frees;
  uint32_t in_use;
} SlabCache;

typedef struct {
  uint32_t total_pages;
  uint32_t used_pages;
  uint32_t free_pages;
  uint32_t largest_free_run;
  uint32_t free_runs;
  uint32_t fragmentation_permille;
  uint32_t alloc_calls;
  uint32_t free_calls;
} PmmStats;

typedef struct {
  uint32_t obj_size;
  uint32_t pages;
  uint32_t allocs;
  uint32_t frees;
  uint32_t in_use;
  uint32_t free_objects;
} SlabClassStats;

#define SLAB_CACHE_COUNT 4
void *pmm_alloc_page(void);

static SlabCache slab_caches[SLAB_CACHE_COUNT] = {
    {32u, 0, 0, 0, 0, 0},
    {64u, 0, 0, 0, 0, 0},
    {128u, 0, 0, 0, 0, 0},
    {256u, 0, 0, 0, 0, 0},
};

static SlabCache *slab_pick_cache(uint32_t size) {
  for (int i = 0; i < SLAB_CACHE_COUNT; i++) {
    if (size <= slab_caches[i].obj_size) {
      return &slab_caches[i];
    }
  }
  return 0;
}

static int slab_grow_cache(SlabCache *cache) {
  uint8_t *page;
  uint32_t offset;

  if (cache == 0 || cache->obj_size < sizeof(SlabNode)) {
    return 0;
  }

  page = (uint8_t *)pmm_alloc_page();
  if (page == 0) {
    return 0;
  }

  cache->pages++;
  for (offset = 0; offset + cache->obj_size <= PAGE_SIZE; offset += cache->obj_size) {
    SlabNode *node = (SlabNode *)(page + offset);
    node->next = cache->free_list;
    cache->free_list = node;
  }
  return 1;
}

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

  pmm_free_calls++;

  memory_bitmap[page / 8] &= ~(1 << (page % 8));
}

// Logic to check status
int pmm_is_page_used(uint32_t page_addr) {
  uint32_t page = page_addr / PAGE_SIZE;
  return (memory_bitmap[page / 8] & (1 << (page % 8))) != 0;
}

// The allocator: finds and returns a pointer to a free 4KB page
void *pmm_alloc_page() {
  pmm_alloc_calls++;
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

int pmm_get_total_pages() {
  return (BITMAP_SIZE * 8);
}

void pmm_get_stats(void *out_stats) {
  PmmStats *stats = (PmmStats *)out_stats;
  uint32_t first_user_page = 0x100000 / PAGE_SIZE;
  uint32_t total_pages = (BITMAP_SIZE * 8) - first_user_page;
  uint32_t used_pages = 0;
  uint32_t largest_run = 0;
  uint32_t cur_run = 0;
  uint32_t free_runs = 0;

  if (stats == 0) {
    return;
  }

  for (uint32_t i = first_user_page; i < BITMAP_SIZE * 8; i++) {
    uint32_t addr = i * PAGE_SIZE;
    if (!pmm_is_page_used(addr)) {
      cur_run++;
      if (cur_run == 1) {
        free_runs++;
      }
      if (cur_run > largest_run) {
        largest_run = cur_run;
      }
    } else {
      used_pages++;
      cur_run = 0;
    }
  }

  stats->total_pages = total_pages;
  stats->used_pages = used_pages;
  stats->free_pages = total_pages - used_pages;
  stats->largest_free_run = largest_run;
  stats->free_runs = free_runs;
  if (stats->free_pages == 0 || largest_run == 0) {
    stats->fragmentation_permille = 0;
  } else {
    stats->fragmentation_permille =
        1000u - (largest_run * 1000u) / stats->free_pages;
  }
  stats->alloc_calls = pmm_alloc_calls;
  stats->free_calls = pmm_free_calls;
}

void *slab_alloc(uint32_t size) {
  SlabCache *cache = slab_pick_cache(size);
  SlabNode *node;

  if (cache == 0) {
    return pmm_alloc_page();
  }

  if (cache->free_list == 0 && !slab_grow_cache(cache)) {
    return 0;
  }

  node = cache->free_list;
  cache->free_list = node->next;
  cache->allocs++;
  cache->in_use++;
  return (void *)node;
}

int slab_free(void *ptr, uint32_t size) {
  SlabCache *cache = slab_pick_cache(size);
  SlabNode *node;

  if (ptr == 0) {
    return 0;
  }

  if (cache == 0) {
    pmm_free_page((uint32_t)ptr);
    return 1;
  }

  node = (SlabNode *)ptr;
  node->next = cache->free_list;
  cache->free_list = node;
  cache->frees++;
  if (cache->in_use > 0) {
    cache->in_use--;
  }
  return 1;
}

int slab_get_stats(int class_index, void *out_stats) {
  SlabClassStats *stats = (SlabClassStats *)out_stats;
  SlabCache *cache;
  SlabNode *n;
  uint32_t free_count = 0;

  if (class_index < 0 || class_index >= SLAB_CACHE_COUNT || stats == 0) {
    return 0;
  }

  cache = &slab_caches[class_index];
  for (n = cache->free_list; n != 0; n = n->next) {
    free_count++;
  }

  stats->obj_size = cache->obj_size;
  stats->pages = cache->pages;
  stats->allocs = cache->allocs;
  stats->frees = cache->frees;
  stats->in_use = cache->in_use;
  stats->free_objects = free_count;
  return 1;
}
