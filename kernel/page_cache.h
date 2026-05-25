#ifndef PAGE_CACHE_H
#define PAGE_CACHE_H

#include "disk.h"

typedef struct {
	uint32_t read_hits;
	uint32_t read_misses;
	uint32_t write_hits;
	uint32_t write_misses;
	uint32_t evictions;
	uint32_t buffered_writes;
	uint32_t flush_writes;
	uint32_t dirty_lines;
} PageCacheStats;

void page_cache_init(void);
int page_cache_read_sector(uint32_t lba, uint16_t *buffer);
int page_cache_write_sector(uint32_t lba, const uint16_t *buffer);
void page_cache_flush_all(void);
void page_cache_get_stats(PageCacheStats *out_stats);
void page_cache_reset_stats(void);

#endif
