#include "filesystem.h"

static uint16_t read_le16(const uint8_t *p) {
  return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t read_le32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

int ext2_probe(uint32_t partition_lba, Ext2Info *out_info) {
  uint16_t sector[256];
  uint8_t *bytes = (uint8_t *)sector;
  uint16_t magic;
  uint32_t log_block_size;

  if (out_info == 0) {
    return -1;
  }

  out_info->valid = 0;
  out_info->block_size = 0;
  out_info->inode_count = 0;
  out_info->block_count = 0;

  /* ext2 superblock starts at offset 1024 (LBA + 2 on 512-byte sectors). */
  if (disk_read_sector_uncached(partition_lba + 2, sector) != DISK_IO_OK) {
    return -1;
  }

  magic = read_le16(&bytes[56]);
  if (magic != 0xEF53) {
    return 0;
  }

  log_block_size = read_le32(&bytes[24]);
  out_info->valid = 1;
  out_info->inode_count = read_le32(&bytes[0]);
  out_info->block_count = read_le32(&bytes[4]);
  out_info->block_size = 1024u << log_block_size;
  return 1;
}
