#include "page_cache.h"

#define CACHE_LINES 32

typedef struct {
  int valid;
  int dirty;
  int ref;
  uint32_t lba;
  uint16_t data[256];
} CacheLine;

static CacheLine cache_lines[CACHE_LINES];
static int clock_hand = 0;
static int cache_initialized = 0;
static PageCacheStats cache_stats = {0, 0, 0, 0, 0, 0, 0, 0};

static void copy_sector(uint16_t *dst, const uint16_t *src) {
  for (int i = 0; i < 256; i++) {
    dst[i] = src[i];
  }
}

static int find_line(uint32_t lba) {
  for (int i = 0; i < CACHE_LINES; i++) {
    if (cache_lines[i].valid && cache_lines[i].lba == lba) {
      return i;
    }
  }
  return -1;
}

static void flush_line(int idx) {
  if (idx < 0 || idx >= CACHE_LINES) {
    return;
  }
  if (cache_lines[idx].valid && cache_lines[idx].dirty) {
    disk_write_sector_uncached(cache_lines[idx].lba, cache_lines[idx].data);
    cache_lines[idx].dirty = 0;
    cache_stats.flush_writes++;
  }
}

static int pick_victim_line(void) {
  for (int i = 0; i < CACHE_LINES * 2; i++) {
    int idx = clock_hand;
    clock_hand = (clock_hand + 1) % CACHE_LINES;

    if (!cache_lines[idx].valid) {
      return idx;
    }
    if (cache_lines[idx].ref) {
      cache_lines[idx].ref = 0;
      continue;
    }
    flush_line(idx);
    cache_lines[idx].valid = 0;
    cache_stats.evictions++;
    return idx;
  }

  flush_line(clock_hand);
  cache_lines[clock_hand].valid = 0;
  cache_stats.evictions++;
  return clock_hand;
}

void page_cache_init(void) {
  if (cache_initialized) {
    return;
  }
  for (int i = 0; i < CACHE_LINES; i++) {
    cache_lines[i].valid = 0;
    cache_lines[i].dirty = 0;
    cache_lines[i].ref = 0;
    cache_lines[i].lba = 0;
  }
  clock_hand = 0;
  cache_initialized = 1;
}

int page_cache_read_sector(uint32_t lba, uint16_t *buffer) {
  int idx;

  if (buffer == 0) {
    return DISK_IO_ERR_NO_DEVICE;
  }
  page_cache_init();

  idx = find_line(lba);
  if (idx >= 0) {
    copy_sector(buffer, cache_lines[idx].data);
    cache_lines[idx].ref = 1;
    cache_stats.read_hits++;
    return DISK_IO_OK;
  }

  cache_stats.read_misses++;

  idx = pick_victim_line();
  if (disk_read_sector_uncached(lba, cache_lines[idx].data) != DISK_IO_OK) {
    cache_lines[idx].valid = 0;
    cache_lines[idx].dirty = 0;
    cache_lines[idx].ref = 0;
    return DISK_IO_ERR_NO_DEVICE;
  }

  cache_lines[idx].valid = 1;
  cache_lines[idx].dirty = 0;
  cache_lines[idx].ref = 1;
  cache_lines[idx].lba = lba;

  copy_sector(buffer, cache_lines[idx].data);
  return DISK_IO_OK;
}

int page_cache_write_sector(uint32_t lba, const uint16_t *buffer) {
  int idx;

  if (buffer == 0) {
    return DISK_IO_ERR_NO_DEVICE;
  }
  page_cache_init();

  idx = find_line(lba);
  if (idx < 0) {
    cache_stats.write_misses++;
    idx = pick_victim_line();
    cache_lines[idx].valid = 1;
    cache_lines[idx].lba = lba;
  } else {
    cache_stats.write_hits++;
  }

  copy_sector(cache_lines[idx].data, buffer);
  cache_lines[idx].dirty = 1;
  cache_lines[idx].ref = 1;
  cache_stats.buffered_writes++;
  return DISK_IO_OK;
}

void page_cache_flush_all(void) {
  page_cache_init();
  for (int i = 0; i < CACHE_LINES; i++) {
    flush_line(i);
  }
}

void page_cache_get_stats(PageCacheStats *out_stats) {
  if (out_stats == 0) {
    return;
  }

  cache_stats.dirty_lines = 0;
  for (int i = 0; i < CACHE_LINES; i++) {
    if (cache_lines[i].valid && cache_lines[i].dirty) {
      cache_stats.dirty_lines++;
    }
  }
  *out_stats = cache_stats;
}

void page_cache_reset_stats(void) {
  cache_stats.read_hits = 0;
  cache_stats.read_misses = 0;
  cache_stats.write_hits = 0;
  cache_stats.write_misses = 0;
  cache_stats.evictions = 0;
  cache_stats.buffered_writes = 0;
  cache_stats.flush_writes = 0;
  cache_stats.dirty_lines = 0;
}
