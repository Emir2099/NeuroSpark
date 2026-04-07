#ifndef PARTITION_H
#define PARTITION_H

#include "disk.h"

#define PARTITION_MAX_ENTRIES 4

typedef struct {
  int valid;
  uint8_t type;
  uint32_t lba_start;
  uint32_t sector_count;
} PartitionEntry;

int partition_scan_mbr(PartitionEntry *out_entries, int max_entries);
int partition_scan_gpt(PartitionEntry *out_entries, int max_entries);

#endif
