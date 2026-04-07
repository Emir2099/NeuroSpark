#include "partition.h"

typedef unsigned long long uint64_t;

static uint32_t read_le32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

static uint64_t read_le64(const uint8_t *p) {
  return (uint64_t)p[0] | ((uint64_t)p[1] << 8) | ((uint64_t)p[2] << 16) |
         ((uint64_t)p[3] << 24) | ((uint64_t)p[4] << 32) | ((uint64_t)p[5] << 40) |
         ((uint64_t)p[6] << 48) | ((uint64_t)p[7] << 56);
}

static int guid_is_zero(const uint8_t *guid) {
  for (int i = 0; i < 16; i++) {
    if (guid[i] != 0) {
      return 0;
    }
  }
  return 1;
}

int partition_scan_mbr(PartitionEntry *out_entries, int max_entries) {
  uint16_t sector[256];
  uint8_t *bytes = (uint8_t *)sector;
  int count = 0;

  if (out_entries == 0 || max_entries <= 0) {
    return -1;
  }

  for (int i = 0; i < max_entries; i++) {
    out_entries[i].valid = 0;
    out_entries[i].type = 0;
    out_entries[i].lba_start = 0;
    out_entries[i].sector_count = 0;
  }

  if (disk_read_sector_uncached(0, sector) != DISK_IO_OK) {
    return -1;
  }

  if (bytes[510] != 0x55 || bytes[511] != 0xAA) {
    return 0;
  }

  for (int i = 0; i < PARTITION_MAX_ENTRIES && i < max_entries; i++) {
    int off = 446 + (i * 16);
    uint8_t type = bytes[off + 4];
    uint32_t lba_start = read_le32(&bytes[off + 8]);
    uint32_t sectors = read_le32(&bytes[off + 12]);

    if (type == 0 || sectors == 0) {
      continue;
    }

    out_entries[count].valid = 1;
    out_entries[count].type = type;
    out_entries[count].lba_start = lba_start;
    out_entries[count].sector_count = sectors;
    count++;
  }

  return count;
}

int partition_scan_gpt(PartitionEntry *out_entries, int max_entries) {
  uint16_t sector[256];
  uint8_t *bytes = (uint8_t *)sector;
  uint32_t header_lba;
  uint32_t entry_lba;
  uint32_t entry_count;
  uint32_t entry_size;
  uint32_t entries_per_sector;
  int count = 0;

  if (out_entries == 0 || max_entries <= 0) {
    return -1;
  }

  for (int i = 0; i < max_entries; i++) {
    out_entries[i].valid = 0;
    out_entries[i].type = 0;
    out_entries[i].lba_start = 0;
    out_entries[i].sector_count = 0;
  }

  if (disk_read_sector_uncached(0, sector) != DISK_IO_OK) {
    return -1;
  }
  if (bytes[510] != 0x55 || bytes[511] != 0xAA) {
    return 0;
  }

  header_lba = 1;
  if (disk_read_sector_uncached(header_lba, sector) != DISK_IO_OK) {
    return -1;
  }
  if (bytes[0] != 'E' || bytes[1] != 'F' || bytes[2] != 'I' || bytes[3] != ' ' ||
      bytes[4] != 'P' || bytes[5] != 'A' || bytes[6] != 'R' || bytes[7] != 'T') {
    return 0;
  }

  entry_lba = read_le32(&bytes[72]);
  entry_count = read_le32(&bytes[80]);
  entry_size = read_le32(&bytes[84]);
  if (entry_size == 0) {
    return 0;
  }

  entries_per_sector = 512u / entry_size;
  if (entries_per_sector == 0) {
    return 0;
  }

  for (uint32_t i = 0; i < entry_count && count < max_entries; i++) {
    uint32_t sector_index = i / entries_per_sector;
    uint32_t entry_offset = (i % entries_per_sector) * entry_size;
    uint16_t entry_sector[256];
    uint8_t *entry_bytes;
    uint64_t first_lba;
    uint64_t last_lba;

    if (disk_read_sector_uncached(entry_lba + sector_index, entry_sector) != DISK_IO_OK) {
      return -1;
    }
    entry_bytes = (uint8_t *)entry_sector;
    if (guid_is_zero(&entry_bytes[entry_offset + 16])) {
      continue;
    }

    first_lba = read_le64(&entry_bytes[entry_offset + 32]);
    last_lba = read_le64(&entry_bytes[entry_offset + 40]);
    if (last_lba < first_lba) {
      continue;
    }

    out_entries[count].valid = 1;
    out_entries[count].type = 0xEE;
    out_entries[count].lba_start = (uint32_t)first_lba;
    out_entries[count].sector_count = (uint32_t)(last_lba - first_lba + 1u);
    count++;
  }

  return count;
}
