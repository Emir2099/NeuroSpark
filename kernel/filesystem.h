#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "disk.h"

typedef struct {
  int valid;
  uint32_t block_size;
  uint32_t inode_count;
  uint32_t block_count;
} Ext2Info;

int ext2_probe(uint32_t partition_lba, Ext2Info *out_info);

#endif
