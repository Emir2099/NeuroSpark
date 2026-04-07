#include "ext2fs.h"

#include "filesystem.h"
#include "partition.h"
#include "disk.h"

#define EXT2_SUPERBLOCK_OFFSET_LBA 2
#define EXT2_SUPERBLOCK_MAGIC 0xEF53
#define EXT2_BLOCK_SIZE 1024
#define EXT2_INODE_SIZE 128
#define EXT2_ROOT_INODE 2
#define EXT2_MAX_HANDLES 256
#define EXT2_NAME_MAX 255
#define EXT2_N_BLOCKS 15
#define EXT2_S_IFREG 0x8000
#define EXT2_S_IFDIR 0x4000

#define EXT2_FT_UNKNOWN 0
#define EXT2_FT_REG_FILE 1
#define EXT2_FT_DIR 2

#define EXT2_O_CREAT 0x4
#define EXT2_O_TRUNC 0x8

typedef struct __attribute__((packed)) {
  uint32_t inode_count;
  uint32_t block_count;
  uint32_t reserved_blocks_count;
  uint32_t free_blocks_count;
  uint32_t free_inodes_count;
  uint32_t first_data_block;
  uint32_t log_block_size;
  uint32_t log_frag_size;
  uint32_t blocks_per_group;
  uint32_t frags_per_group;
  uint32_t inodes_per_group;
  uint32_t mtime;
  uint32_t wtime;
  uint16_t mount_count;
  uint16_t max_mount_count;
  uint16_t magic;
  uint16_t state;
  uint16_t errors;
  uint16_t minor_rev_level;
  uint32_t lastcheck;
  uint32_t checkinterval;
  uint32_t creator_os;
  uint32_t rev_level;
  uint16_t def_resuid;
  uint16_t def_resgid;
  uint32_t first_ino;
  uint16_t inode_size;
  uint16_t block_group_nr;
  uint32_t feature_compat;
  uint32_t feature_incompat;
  uint32_t feature_ro_compat;
  uint8_t uuid[16];
  char volume_name[16];
  char last_mounted[64];
  uint32_t algo_bitmap;
  uint8_t padding[820];
} Ext2Superblock;

typedef struct __attribute__((packed)) {
  uint32_t block_bitmap;
  uint32_t inode_bitmap;
  uint32_t inode_table;
  uint16_t free_blocks_count;
  uint16_t free_inodes_count;
  uint16_t used_dirs_count;
  uint16_t pad;
  uint8_t reserved[12];
} Ext2GroupDesc;

typedef struct __attribute__((packed)) {
  uint16_t mode;
  uint16_t uid;
  uint32_t size;
  uint32_t atime;
  uint32_t ctime;
  uint32_t mtime;
  uint32_t dtime;
  uint16_t gid;
  uint16_t links_count;
  uint32_t blocks;
  uint32_t flags;
  uint32_t osd1;
  uint32_t block[15];
  uint32_t generation;
  uint32_t file_acl;
  uint32_t dir_acl;
  uint32_t faddr;
  uint8_t osd2[12];
} Ext2Inode;

typedef struct __attribute__((packed)) {
  uint32_t inode;
  uint16_t rec_len;
  uint8_t name_len;
  uint8_t file_type;
  char name[];
} Ext2DirEntry;

typedef struct {
  int used;
  int ref_count;
  uint32_t inode_num;
  uint32_t pos;
  int flags;
} Ext2Handle;

typedef struct {
  int mounted;
  uint32_t partition_lba;
  uint32_t block_size;
  uint32_t sectors_per_block;
  uint32_t inodes_per_group;
  uint32_t blocks_per_group;
  uint32_t inode_size;
  uint32_t inode_table_block;
  uint32_t block_bitmap_block;
  uint32_t inode_bitmap_block;
  uint32_t group_desc_block;
  uint32_t group_desc_count;
  uint32_t total_inodes;
  uint32_t total_blocks;
  Ext2Superblock sb;
} Ext2MountState;

static Ext2MountState ext2_state;
static Ext2Handle ext2_handles[EXT2_MAX_HANDLES];

static uint16_t read_le16(const uint8_t *p) {
  return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t read_le32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

static void write_le32(uint8_t *p, uint32_t value) {
  p[0] = (uint8_t)(value & 0xFF);
  p[1] = (uint8_t)((value >> 8) & 0xFF);
  p[2] = (uint8_t)((value >> 16) & 0xFF);
  p[3] = (uint8_t)((value >> 24) & 0xFF);
}

static int str_eq(const char *a, const char *b) {
  int i = 0;
  if (a == 0 || b == 0) {
    return 0;
  }
  while (a[i] && b[i]) {
    if (a[i] != b[i]) {
      return 0;
    }
    i++;
  }
  return a[i] == '\0' && b[i] == '\0';
}

static int parse_u32_dec(const char *s, uint32_t *out) {
  uint32_t value = 0;
  int i = 0;
  if (s == 0 || s[0] == '\0' || out == 0) {
    return 0;
  }
  while (s[i]) {
    if (s[i] < '0' || s[i] > '9') {
      return 0;
    }
    value = (value * 10u) + (uint32_t)(s[i] - '0');
    i++;
  }
  *out = value;
  return 1;
}

static uint32_t block_to_lba(uint32_t block_num) {
  return ext2_state.partition_lba + (block_num * ext2_state.sectors_per_block);
}

static int read_block(uint32_t block_num, uint8_t *buffer) {
  uint32_t lba = block_to_lba(block_num);
  uint32_t sectors = ext2_state.sectors_per_block;
  if (buffer == 0) {
    return 0;
  }
  for (uint32_t s = 0; s < sectors; s++) {
    if (disk_read_sector_uncached(lba + s, (uint16_t *)(buffer + (s * 512u))) != DISK_IO_OK) {
      return 0;
    }
  }
  return 1;
}

static int write_block(uint32_t block_num, const uint8_t *buffer) {
  uint32_t lba = block_to_lba(block_num);
  uint32_t sectors = ext2_state.sectors_per_block;
  if (buffer == 0) {
    return 0;
  }
  for (uint32_t s = 0; s < sectors; s++) {
    disk_write_sector_uncached(lba + s, (const uint16_t *)(buffer + (s * 512u)));
  }
  return 1;
}

static int read_superblock(void) {
  uint8_t raw[1024];
  if (disk_read_sector_uncached(ext2_state.partition_lba + EXT2_SUPERBLOCK_OFFSET_LBA,
                                (uint16_t *)raw) != DISK_IO_OK) {
    return 0;
  }
  if (disk_read_sector_uncached(ext2_state.partition_lba + EXT2_SUPERBLOCK_OFFSET_LBA + 1,
                                (uint16_t *)(raw + 512)) != DISK_IO_OK) {
    return 0;
  }
  ext2_state.sb.inode_count = read_le32(&raw[0]);
  ext2_state.sb.block_count = read_le32(&raw[4]);
  ext2_state.sb.first_data_block = read_le32(&raw[20]);
  ext2_state.sb.log_block_size = read_le32(&raw[24]);
  ext2_state.sb.blocks_per_group = read_le32(&raw[32]);
  ext2_state.sb.inodes_per_group = read_le32(&raw[40]);
  ext2_state.sb.magic = read_le16(&raw[56]);
  ext2_state.sb.rev_level = read_le32(&raw[76]);
  ext2_state.sb.first_ino = read_le32(&raw[84]);
  ext2_state.sb.inode_size = read_le16(&raw[88]);
  return ext2_state.sb.magic == EXT2_SUPERBLOCK_MAGIC;
}

static int read_group_desc(uint32_t group, Ext2GroupDesc *out_desc) {
  uint8_t raw[1024];
  uint32_t per_block = ext2_state.block_size / 32u;
  uint32_t block = ext2_state.group_desc_block;
  uint32_t block_index = block + (group / per_block);
  uint32_t offset = (group % per_block) * 32u;

  if (!read_block(block_index, raw)) {
    return 0;
  }
  out_desc->block_bitmap = read_le32(&raw[offset + 0]);
  out_desc->inode_bitmap = read_le32(&raw[offset + 4]);
  out_desc->inode_table = read_le32(&raw[offset + 8]);
  out_desc->free_blocks_count = read_le16(&raw[offset + 12]);
  out_desc->free_inodes_count = read_le16(&raw[offset + 14]);
  out_desc->used_dirs_count = read_le16(&raw[offset + 16]);
  return 1;
}

static int inode_group_and_offset(uint32_t inode_num, uint32_t *group_out, uint32_t *index_out) {
  if (inode_num == 0 || inode_num > ext2_state.total_inodes) {
    return 0;
  }
  *group_out = (inode_num - 1u) / ext2_state.inodes_per_group;
  *index_out = (inode_num - 1u) % ext2_state.inodes_per_group;
  return 1;
}

static int read_inode(uint32_t inode_num, Ext2Inode *out_inode) {
  uint32_t group;
  uint32_t index;
  Ext2GroupDesc gd;
  uint8_t raw[1024];
  uint32_t inode_offset;
  uint32_t block_index;
  uint32_t block_offset;

  if (out_inode == 0 || !inode_group_and_offset(inode_num, &group, &index)) {
    return 0;
  }
  if (!read_group_desc(group, &gd)) {
    return 0;
  }

  inode_offset = index * ext2_state.inode_size;
  block_index = gd.inode_table + (inode_offset / ext2_state.block_size);
  block_offset = inode_offset % ext2_state.block_size;
  if (!read_block(block_index, raw)) {
    return 0;
  }
  if (block_offset + sizeof(Ext2Inode) > ext2_state.block_size) {
    uint8_t temp[4096];
    if (ext2_state.block_size > sizeof(temp)) {
      return 0;
    }
    if (!read_block(block_index, temp)) {
      return 0;
    }
    for (uint32_t i = 0; i < sizeof(Ext2Inode); i++) {
      ((uint8_t *)out_inode)[i] = temp[block_offset + i];
    }
    return 1;
  }

  for (uint32_t i = 0; i < sizeof(Ext2Inode); i++) {
    ((uint8_t *)out_inode)[i] = raw[block_offset + i];
  }
  return 1;
}

static int write_inode(uint32_t inode_num, const Ext2Inode *inode) {
  uint32_t group;
  uint32_t index;
  Ext2GroupDesc gd;
  uint8_t raw[4096];
  uint32_t inode_offset;
  uint32_t block_index;
  uint32_t block_offset;

  if (inode == 0 || !inode_group_and_offset(inode_num, &group, &index)) {
    return 0;
  }
  if (!read_group_desc(group, &gd)) {
    return 0;
  }
  if (ext2_state.block_size > sizeof(raw)) {
    return 0;
  }

  inode_offset = index * ext2_state.inode_size;
  block_index = gd.inode_table + (inode_offset / ext2_state.block_size);
  block_offset = inode_offset % ext2_state.block_size;
  if (!read_block(block_index, raw)) {
    return 0;
  }
  for (uint32_t i = 0; i < sizeof(Ext2Inode); i++) {
    raw[block_offset + i] = ((const uint8_t *)inode)[i];
  }
  return write_block(block_index, raw);
}

static int bit_test(const uint8_t *bitmap, uint32_t bit) {
  return (bitmap[bit / 8u] & (uint8_t)(1u << (bit % 8u))) != 0;
}

static void bit_set(uint8_t *bitmap, uint32_t bit, int value) {
  uint8_t mask = (uint8_t)(1u << (bit % 8u));
  if (value) {
    bitmap[bit / 8u] |= mask;
  } else {
    bitmap[bit / 8u] &= (uint8_t)~mask;
  }
}

static int alloc_inode(void) {
  Ext2GroupDesc gd;
  uint8_t bitmap[1024];
  uint32_t group;

  for (group = 0; group < ext2_state.group_desc_count; group++) {
    if (!read_group_desc(group, &gd)) {
      return -1;
    }
    if (!read_block(gd.inode_bitmap, bitmap)) {
      return -1;
    }
    for (uint32_t bit = 0; bit < ext2_state.inodes_per_group; bit++) {
      uint32_t inode_num = group * ext2_state.inodes_per_group + bit + 1u;
      if (inode_num > ext2_state.total_inodes) {
        break;
      }
      if (!bit_test(bitmap, bit)) {
        bit_set(bitmap, bit, 1);
        if (!write_block(gd.inode_bitmap, bitmap)) {
          return -1;
        }
        return (int)inode_num;
      }
    }
  }
  return -1;
}

static void free_inode(uint32_t inode_num) {
  uint32_t group;
  uint32_t index;
  Ext2GroupDesc gd;
  uint8_t bitmap[1024];

  if (!inode_group_and_offset(inode_num, &group, &index)) {
    return;
  }
  if (!read_group_desc(group, &gd)) {
    return;
  }
  if (!read_block(gd.inode_bitmap, bitmap)) {
    return;
  }
  bit_set(bitmap, index, 0);
  (void)write_block(gd.inode_bitmap, bitmap);
}

static int alloc_block(void) {
  Ext2GroupDesc gd;
  uint8_t bitmap[1024];
  uint32_t start_block = ext2_state.sb.first_data_block;

  for (uint32_t group = 0; group < ext2_state.group_desc_count; group++) {
    if (!read_group_desc(group, &gd)) {
      return -1;
    }
    if (!read_block(gd.block_bitmap, bitmap)) {
      return -1;
    }
    for (uint32_t bit = 0; bit < ext2_state.blocks_per_group; bit++) {
      uint32_t block_num = group * ext2_state.blocks_per_group + bit + start_block;
      if (block_num >= ext2_state.total_blocks) {
        break;
      }
      if (!bit_test(bitmap, bit)) {
        bit_set(bitmap, bit, 1);
        if (!write_block(gd.block_bitmap, bitmap)) {
          return -1;
        }
        return (int)block_num;
      }
    }
  }
  return -1;
}

static void free_block(uint32_t block_num) {
  uint32_t group = (block_num - ext2_state.sb.first_data_block) / ext2_state.blocks_per_group;
  uint32_t index = (block_num - ext2_state.sb.first_data_block) % ext2_state.blocks_per_group;
  Ext2GroupDesc gd;
  uint8_t bitmap[1024];

  if (group >= ext2_state.group_desc_count) {
    return;
  }
  if (!read_group_desc(group, &gd) || !read_block(gd.block_bitmap, bitmap)) {
    return;
  }
  bit_set(bitmap, index, 0);
  (void)write_block(gd.block_bitmap, bitmap);
}

static uint32_t inode_ptrs_per_block(void) {
  return ext2_state.block_size / 4u;
}

static int inode_get_block(Ext2Inode *inode, uint32_t index, int create) {
  uint8_t indirect[4096];
  uint32_t ptrs = inode_ptrs_per_block();
  uint32_t block_num;

  if (index < 12u) {
    block_num = inode->block[index];
    if (block_num == 0 && create) {
      block_num = (uint32_t)alloc_block();
      if (block_num == 0 || (int)block_num < 0) {
        return 0;
      }
      inode->block[index] = block_num;
    }
    return (int)inode->block[index];
  }

  index -= 12u;
  if (index >= ptrs) {
    return 0;
  }

  if (inode->block[12] == 0) {
    if (!create) {
      return 0;
    }
    inode->block[12] = (uint32_t)alloc_block();
    if (inode->block[12] == 0 || (int)inode->block[12] < 0) {
      inode->block[12] = 0;
      return 0;
    }
    for (uint32_t i = 0; i < ext2_state.block_size; i++) {
      indirect[i] = 0;
    }
    if (!write_block(inode->block[12], indirect)) {
      return 0;
    }
  }

  if (!read_block(inode->block[12], indirect)) {
    return 0;
  }
  block_num = read_le32(&indirect[index * 4u]);
  if (block_num == 0 && create) {
    block_num = (uint32_t)alloc_block();
    if (block_num == 0 || (int)block_num < 0) {
      return 0;
    }
    write_le32(&indirect[index * 4u], block_num);
    if (!write_block(inode->block[12], indirect)) {
      return 0;
    }
  }
  return (int)read_le32(&indirect[index * 4u]);
}

static int inode_clear_blocks(Ext2Inode *inode) {
  uint8_t indirect[4096];
  uint32_t ptrs = inode_ptrs_per_block();

  for (int i = 0; i < 12; i++) {
    if (inode->block[i] != 0) {
      free_block(inode->block[i]);
      inode->block[i] = 0;
    }
  }

  if (inode->block[12] != 0) {
    if (read_block(inode->block[12], indirect)) {
      for (uint32_t i = 0; i < ptrs; i++) {
        uint32_t block_num = read_le32(&indirect[i * 4u]);
        if (block_num != 0) {
          free_block(block_num);
        }
      }
    }
    free_block(inode->block[12]);
    inode->block[12] = 0;
  }
  inode->size = 0;
  inode->blocks = 0;
  return 1;
}

static int parse_component(const char **path, char *component, int component_size) {
  int len = 0;
  const char *s;

  if (path == 0 || component == 0 || component_size <= 0) {
    return 0;
  }
  s = *path;
  while (*s == '/') {
    s++;
  }
  if (*s == '\0') {
    component[0] = '\0';
    *path = s;
    return 0;
  }
  while (*s && *s != '/' && len < component_size - 1) {
    component[len++] = *s++;
  }
  component[len] = '\0';
  while (*s == '/') {
    s++;
  }
  *path = s;
  return len > 0;
}

static int dir_lookup(uint32_t dir_inode_num, const char *name, uint32_t *found_inode,
                      uint32_t *dir_block_num, uint32_t *dir_entry_offset, uint16_t *dir_rec_len,
                      uint32_t *entry_block_index) {
  Ext2Inode dir_inode;
  uint8_t block[4096];
  uint32_t block_count;

  if (!read_inode(dir_inode_num, &dir_inode) || !(dir_inode.mode & EXT2_S_IFDIR)) {
    return 0;
  }

  block_count = (dir_inode.size + ext2_state.block_size - 1u) / ext2_state.block_size;
  for (uint32_t b = 0; b < block_count; b++) {
    int block_num = inode_get_block(&dir_inode, b, 0);
    if (block_num <= 0) {
      continue;
    }
    if (!read_block((uint32_t)block_num, block)) {
      continue;
    }
    uint32_t off = 0;
    while (off + 8u <= ext2_state.block_size) {
      Ext2DirEntry *entry = (Ext2DirEntry *)(block + off);
      if (entry->rec_len == 0) {
        break;
      }
      if (entry->inode != 0 && entry->name_len > 0) {
        char entry_name[EXT2_NAME_MAX + 1];
        for (uint32_t i = 0; i < entry->name_len; i++) {
          entry_name[i] = entry->name[i];
        }
        entry_name[entry->name_len] = '\0';
        if (str_eq(entry_name, name)) {
          if (found_inode) {
            *found_inode = entry->inode;
          }
          if (dir_block_num) {
            *dir_block_num = (uint32_t)block_num;
          }
          if (dir_entry_offset) {
            *dir_entry_offset = off;
          }
          if (dir_rec_len) {
            *dir_rec_len = entry->rec_len;
          }
          if (entry_block_index) {
            *entry_block_index = b;
          }
          return 1;
        }
      }
      off += entry->rec_len;
    }
  }
  return 0;
}

static int resolve_path(const char *path, uint32_t *inode_out, uint32_t *parent_inode_out,
                        char *leaf_name, int leaf_size) {
  uint32_t current = EXT2_ROOT_INODE;
  char component[EXT2_NAME_MAX + 1];
  const char *cursor = path;
  uint32_t next_inode = 0;

  if (path == 0 || path[0] != '/') {
    return 0;
  }
  if (path[1] == '\0') {
    if (inode_out) {
      *inode_out = EXT2_ROOT_INODE;
    }
    if (parent_inode_out) {
      *parent_inode_out = 0;
    }
    if (leaf_name && leaf_size > 0) {
      leaf_name[0] = '\0';
    }
    return 1;
  }

  while (parse_component(&cursor, component, sizeof(component))) {
    if (cursor[0] == '\0') {
      if (leaf_name && leaf_size > 0) {
        int i = 0;
        while (component[i] && i < leaf_size - 1) {
          leaf_name[i] = component[i];
          i++;
        }
        leaf_name[i] = '\0';
      }
      if (inode_out && dir_lookup(current, component, &next_inode, 0, 0, 0, 0)) {
        *inode_out = next_inode;
      } else if (inode_out) {
        *inode_out = 0;
      }
      if (parent_inode_out) {
        *parent_inode_out = current;
      }
      return 1;
    }
    if (!dir_lookup(current, component, &next_inode, 0, 0, 0, 0)) {
      if (inode_out) {
        *inode_out = 0;
      }
      if (parent_inode_out) {
        *parent_inode_out = current;
      }
      if (leaf_name && leaf_size > 0) {
        int i = 0;
        while (component[i] && i < leaf_size - 1) {
          leaf_name[i] = component[i];
          i++;
        }
        leaf_name[i] = '\0';
      }
      return 1;
    }
    current = next_inode;
  }
  return 0;
}

static int dir_add_entry(uint32_t dir_inode_num, const char *name, uint32_t inode_num,
                         uint8_t file_type) {
  Ext2Inode dir_inode;
  uint8_t block[4096];
  uint32_t block_count;
  uint32_t name_len = 0;

  if (!read_inode(dir_inode_num, &dir_inode) || !(dir_inode.mode & EXT2_S_IFDIR)) {
    return 0;
  }
  while (name[name_len] && name_len < EXT2_NAME_MAX) {
    name_len++;
  }
  block_count = (dir_inode.size + ext2_state.block_size - 1u) / ext2_state.block_size;
  for (uint32_t b = 0; b < block_count + 1u; b++) {
    int block_num = inode_get_block(&dir_inode, b, 1);
    if (block_num <= 0) {
      return 0;
    }
    if (!read_block((uint32_t)block_num, block)) {
      return 0;
    }

    uint32_t off = 0;
    while (off + 8u <= ext2_state.block_size) {
      Ext2DirEntry *entry = (Ext2DirEntry *)(block + off);
      uint16_t actual_len = (uint16_t)((8u + entry->name_len + 3u) & ~3u);
      uint16_t space_left;
      uint32_t end_off;

      if (entry->rec_len == 0) {
        break;
      }
      end_off = off + entry->rec_len;
      if (entry->inode != 0 && end_off <= ext2_state.block_size) {
        space_left = (uint16_t)(entry->rec_len - actual_len);
        if (space_left >= (uint16_t)((8u + name_len + 3u) & ~3u)) {
          uint16_t new_len = (uint16_t)((8u + name_len + 3u) & ~3u);
          Ext2DirEntry *new_entry = (Ext2DirEntry *)(block + off + actual_len);
          entry->rec_len = actual_len;
          new_entry->inode = inode_num;
          new_entry->rec_len = (uint16_t)(space_left);
          new_entry->name_len = (uint8_t)name_len;
          new_entry->file_type = file_type;
          for (uint32_t i = 0; i < name_len; i++) {
            new_entry->name[i] = name[i];
          }
          new_entry->name[name_len] = '\0';
          (void)new_len;
          if (!write_block((uint32_t)block_num, block)) {
            return 0;
          }
          if (dir_inode.size < (b + 1u) * ext2_state.block_size) {
            dir_inode.size = (b + 1u) * ext2_state.block_size;
            (void)write_inode(dir_inode_num, &dir_inode);
          }
          return 1;
        }
      }
      off += entry->rec_len;
    }

    if (b >= block_count) {
      for (uint32_t i = 0; i < ext2_state.block_size; i++) {
        block[i] = 0;
      }
      Ext2DirEntry *entry = (Ext2DirEntry *)block;
      entry->inode = inode_num;
      entry->rec_len = (uint16_t)ext2_state.block_size;
      entry->name_len = (uint8_t)name_len;
      entry->file_type = file_type;
      for (uint32_t i = 0; i < name_len; i++) {
        entry->name[i] = name[i];
      }
      entry->name[name_len] = '\0';
      if (!write_block((uint32_t)block_num, block)) {
        return 0;
      }
      dir_inode.size += ext2_state.block_size;
      return write_inode(dir_inode_num, &dir_inode);
    }
  }
  return 0;
}

static int dir_remove_entry(uint32_t dir_inode_num, const char *name) {
  Ext2Inode dir_inode;
  uint8_t block[4096];
  uint32_t block_count;

  if (!read_inode(dir_inode_num, &dir_inode) || !(dir_inode.mode & EXT2_S_IFDIR)) {
    return 0;
  }
  block_count = (dir_inode.size + ext2_state.block_size - 1u) / ext2_state.block_size;
  for (uint32_t b = 0; b < block_count; b++) {
    int block_num = inode_get_block(&dir_inode, b, 0);
    if (block_num <= 0 || !read_block((uint32_t)block_num, block)) {
      continue;
    }
    uint32_t off = 0;
    uint32_t prev_off = 0;
    Ext2DirEntry *prev = 0;
    while (off + 8u <= ext2_state.block_size) {
      Ext2DirEntry *entry = (Ext2DirEntry *)(block + off);
      char entry_name[EXT2_NAME_MAX + 1];
      if (entry->rec_len == 0) {
        break;
      }
      if (entry->inode != 0) {
        for (uint32_t i = 0; i < entry->name_len; i++) {
          entry_name[i] = entry->name[i];
        }
        entry_name[entry->name_len] = '\0';
        if (str_eq(entry_name, name)) {
          if (prev != 0) {
            prev->rec_len = (uint16_t)(prev->rec_len + entry->rec_len);
          } else {
            entry->inode = 0;
          }
          if (!write_block((uint32_t)block_num, block)) {
            return 0;
          }
          (void)prev_off;
          return 1;
        }
      }
      prev = entry;
      prev_off = off;
      off += entry->rec_len;
    }
  }
  return 0;
}

static int ext2_handle_alloc(void) {
  for (int i = 0; i < EXT2_MAX_HANDLES; i++) {
    if (!ext2_handles[i].used) {
      ext2_handles[i].used = 1;
      ext2_handles[i].ref_count = 1;
      ext2_handles[i].inode_num = 0;
      ext2_handles[i].pos = 0;
      ext2_handles[i].flags = 0;
      return i;
    }
  }
  return -1;
}

static int ext2_handle_close(int handle) {
  if (handle < 0 || handle >= EXT2_MAX_HANDLES || !ext2_handles[handle].used) {
    return VFS_ERR_INVALID_FD;
  }
  if (ext2_handles[handle].ref_count > 1) {
    ext2_handles[handle].ref_count--;
    return VFS_OK;
  }
  ext2_handles[handle].used = 0;
  ext2_handles[handle].ref_count = 0;
  return VFS_OK;
}

static int ext2_truncate_inode(uint32_t inode_num, uint32_t new_size) {
  Ext2Inode inode;
  uint32_t old_blocks;
  uint32_t new_blocks;

  if (!read_inode(inode_num, &inode)) {
    return 0;
  }
  old_blocks = (inode.size + ext2_state.block_size - 1u) / ext2_state.block_size;
  new_blocks = (new_size + ext2_state.block_size - 1u) / ext2_state.block_size;
  if (new_blocks < old_blocks) {
    for (uint32_t i = new_blocks; i < old_blocks; i++) {
      int block_num = inode_get_block(&inode, i, 0);
      if (block_num > 0) {
        free_block((uint32_t)block_num);
      }
    }
    if (new_blocks <= 12u) {
      inode_clear_blocks(&inode);
    }
  }
  if (new_size == 0) {
    inode_clear_blocks(&inode);
  }
  inode.size = new_size;
  return write_inode(inode_num, &inode);
}

static int ext2_open(const char *path, int flags) {
  uint32_t inode_num = 0;
  uint32_t parent_inode = 0;
  char leaf[EXT2_NAME_MAX + 1];
  Ext2Inode inode;
  int handle;

  if (!ext2_state.mounted) {
    return VFS_ERR_IO;
  }
  if (!resolve_path(path, &inode_num, &parent_inode, leaf, sizeof(leaf))) {
    return VFS_ERR_INVALID_ARG;
  }

  if (inode_num == 0) {
    if ((flags & EXT2_O_CREAT) == 0) {
      return VFS_ERR_NOT_FOUND;
    }
    inode_num = (uint32_t)alloc_inode();
    if (inode_num == 0 || (int)inode_num < 0) {
      return VFS_ERR_NO_SPACE;
    }
    inode.mode = EXT2_S_IFREG | 0644;
    inode.uid = 0;
    inode.size = 0;
    inode.atime = 0;
    inode.ctime = 0;
    inode.mtime = 0;
    inode.dtime = 0;
    inode.gid = 0;
    inode.links_count = 1;
    inode.blocks = 0;
    inode.flags = 0;
    inode.osd1 = 0;
    for (int i = 0; i < 15; i++) {
      inode.block[i] = 0;
    }
    inode.generation = 0;
    inode.file_acl = 0;
    inode.dir_acl = 0;
    inode.faddr = 0;
    for (int i = 0; i < 12; i++) {
      inode.osd2[i] = 0;
    }
    if (!write_inode(inode_num, &inode)) {
      free_inode(inode_num);
      return VFS_ERR_IO;
    }
    if (!dir_add_entry(parent_inode, leaf, inode_num, EXT2_FT_REG_FILE)) {
      free_inode(inode_num);
      return VFS_ERR_IO;
    }
  }

  if (!read_inode(inode_num, &inode)) {
    return VFS_ERR_IO;
  }
  if (flags & EXT2_O_TRUNC) {
    if (!ext2_truncate_inode(inode_num, 0)) {
      return VFS_ERR_IO;
    }
    if (!read_inode(inode_num, &inode)) {
      return VFS_ERR_IO;
    }
  }

  handle = ext2_handle_alloc();
  if (handle < 0) {
    return VFS_ERR_NO_SPACE;
  }
  ext2_handles[handle].inode_num = inode_num;
  ext2_handles[handle].pos = 0;
  ext2_handles[handle].flags = flags;
  return handle;
}

static int ext2_read(int handle, void *buf, uint32_t size) {
  Ext2Inode inode;
  uint8_t block[4096];
  uint8_t *out = (uint8_t *)buf;
  uint32_t total = 0;
  uint32_t remaining;

  if (handle < 0 || handle >= EXT2_MAX_HANDLES || !ext2_handles[handle].used || buf == 0) {
    return VFS_ERR_INVALID_FD;
  }
  if ((ext2_handles[handle].flags & VFS_O_RDONLY) == 0 &&
      (ext2_handles[handle].flags & VFS_O_RDWR) != VFS_O_RDWR) {
    return VFS_ERR_PERM;
  }
  if (!read_inode(ext2_handles[handle].inode_num, &inode)) {
    return VFS_ERR_IO;
  }
  if (ext2_handles[handle].pos >= inode.size) {
    return 0;
  }

  remaining = inode.size - ext2_handles[handle].pos;
  if (size > remaining) {
    size = remaining;
  }
  while (total < size) {
    uint32_t abs_pos = ext2_handles[handle].pos + total;
    uint32_t block_index = abs_pos / ext2_state.block_size;
    uint32_t block_off = abs_pos % ext2_state.block_size;
    uint32_t chunk = ext2_state.block_size - block_off;
    int block_num = inode_get_block(&inode, block_index, 0);
    if (block_num <= 0) {
      break;
    }
    if (chunk > (size - total)) {
      chunk = size - total;
    }
    if (!read_block((uint32_t)block_num, block)) {
      return VFS_ERR_IO;
    }
    for (uint32_t i = 0; i < chunk; i++) {
      out[total + i] = block[block_off + i];
    }
    total += chunk;
  }
  ext2_handles[handle].pos += total;
  return (int)total;
}

static int ext2_write(int handle, const void *buf, uint32_t size) {
  Ext2Inode inode;
  uint8_t block[4096];
  const uint8_t *src = (const uint8_t *)buf;
  uint32_t total = 0;

  if (handle < 0 || handle >= EXT2_MAX_HANDLES || !ext2_handles[handle].used || buf == 0) {
    return VFS_ERR_INVALID_FD;
  }
  if ((ext2_handles[handle].flags & VFS_O_WRONLY) == 0 &&
      (ext2_handles[handle].flags & VFS_O_RDWR) != VFS_O_RDWR) {
    return VFS_ERR_PERM;
  }
  if (!read_inode(ext2_handles[handle].inode_num, &inode)) {
    return VFS_ERR_IO;
  }

  while (total < size) {
    uint32_t abs_pos = ext2_handles[handle].pos + total;
    uint32_t block_index = abs_pos / ext2_state.block_size;
    uint32_t block_off = abs_pos % ext2_state.block_size;
    uint32_t chunk = ext2_state.block_size - block_off;
    int block_num = inode_get_block(&inode, block_index, 1);
    if (block_num <= 0) {
      return VFS_ERR_NO_SPACE;
    }
    if (chunk > (size - total)) {
      chunk = size - total;
    }
    if (!read_block((uint32_t)block_num, block)) {
      for (uint32_t i = 0; i < ext2_state.block_size; i++) {
        block[i] = 0;
      }
    }
    for (uint32_t i = 0; i < chunk; i++) {
      block[block_off + i] = src[total + i];
    }
    if (!write_block((uint32_t)block_num, block)) {
      return VFS_ERR_IO;
    }
    total += chunk;
  }

  ext2_handles[handle].pos += total;
  if (ext2_handles[handle].pos > inode.size) {
    inode.size = ext2_handles[handle].pos;
  }
  inode.blocks = ((inode.size + 511u) / 512u);
  if (!write_inode(ext2_handles[handle].inode_num, &inode)) {
    return VFS_ERR_IO;
  }
  return (int)total;
}

static int ext2_close(int handle) {
  return ext2_handle_close(handle);
}

static int ext2_delete(const char *path) {
  uint32_t inode_num = 0;
  uint32_t parent_inode = 0;
  char leaf[EXT2_NAME_MAX + 1];
  Ext2Inode inode;

  if (!resolve_path(path, &inode_num, &parent_inode, leaf, sizeof(leaf))) {
    return VFS_ERR_INVALID_ARG;
  }
  if (inode_num == 0 || parent_inode == 0) {
    return VFS_ERR_NOT_FOUND;
  }
  if (!read_inode(inode_num, &inode)) {
    return VFS_ERR_IO;
  }
  if (!inode_clear_blocks(&inode)) {
    return VFS_ERR_IO;
  }
  free_inode(inode_num);
  if (!dir_remove_entry(parent_inode, leaf)) {
    return VFS_ERR_IO;
  }
  inode.dtime = 0;
  return VFS_OK;
}

static int ext2_lseek(int handle, int offset, int whence) {
  Ext2Inode inode;
  int base;
  int target;

  if (handle < 0 || handle >= EXT2_MAX_HANDLES || !ext2_handles[handle].used) {
    return VFS_ERR_INVALID_FD;
  }
  if (!read_inode(ext2_handles[handle].inode_num, &inode)) {
    return VFS_ERR_IO;
  }
  if (whence == VFS_SEEK_SET) {
    base = 0;
  } else if (whence == VFS_SEEK_CUR) {
    base = (int)ext2_handles[handle].pos;
  } else if (whence == VFS_SEEK_END) {
    base = (int)inode.size;
  } else {
    return VFS_ERR_INVALID_ARG;
  }
  target = base + offset;
  if (target < 0) {
    return VFS_ERR_INVALID_ARG;
  }
  ext2_handles[handle].pos = (uint32_t)target;
  return target;
}

static int ext2_stat(const char *path, VfsFileStat *stat_out) {
  uint32_t inode_num = 0;
  if (stat_out == 0) {
    return VFS_ERR_INVALID_ARG;
  }
  if (!resolve_path(path, &inode_num, 0, 0, 0) || inode_num == 0) {
    return VFS_ERR_NOT_FOUND;
  }
  Ext2Inode inode;
  if (!read_inode(inode_num, &inode)) {
    return VFS_ERR_IO;
  }
  stat_out->size = inode.size;
  stat_out->flags = 1;
  return VFS_OK;
}

static int ext2_init_state_from_partition(uint32_t partition_lba) {
  uint32_t gd_block;
  uint32_t total_groups;

  ext2_state = (Ext2MountState){0};
  ext2_state.partition_lba = partition_lba;
  if (!read_superblock()) {
    return 0;
  }
  ext2_state.block_size = EXT2_BLOCK_SIZE;
  if ((1024u << ext2_state.sb.log_block_size) != EXT2_BLOCK_SIZE) {
    return 0;
  }
  ext2_state.sectors_per_block = EXT2_BLOCK_SIZE / 512u;
  ext2_state.inodes_per_group = ext2_state.sb.inodes_per_group;
  ext2_state.blocks_per_group = ext2_state.sb.blocks_per_group;
  ext2_state.inode_size = ext2_state.sb.inode_size ? ext2_state.sb.inode_size : EXT2_INODE_SIZE;
  ext2_state.total_inodes = ext2_state.sb.inode_count;
  ext2_state.total_blocks = ext2_state.sb.block_count;
  total_groups = (ext2_state.total_blocks + ext2_state.blocks_per_group - 1u) /
                 ext2_state.blocks_per_group;
  ext2_state.group_desc_count = total_groups;
  gd_block = (ext2_state.block_size == 1024u) ? 2u : 1u;
  ext2_state.group_desc_block = gd_block;
  ext2_state.block_bitmap_block = 0;
  ext2_state.inode_bitmap_block = 0;
  ext2_state.inode_table_block = 0;
  ext2_state.mounted = 1;

  /* Read group 0 descriptor for convenience; future groups are read on demand. */
  {
    Ext2GroupDesc gd;
    if (!read_group_desc(0, &gd)) {
      ext2_state.mounted = 0;
      return 0;
    }
    ext2_state.block_bitmap_block = gd.block_bitmap;
    ext2_state.inode_bitmap_block = gd.inode_bitmap;
    ext2_state.inode_table_block = gd.inode_table;
  }
  return 1;
}

static const VfsBackendOps ext2_backend_ops = {
    ext2_open,
    ext2_read,
    ext2_write,
    ext2_close,
    ext2_delete,
    ext2_lseek,
    ext2_stat,
};

int ext2_list_root_long(Ext2LsEntry *out, int max_entries) {
  Ext2Inode root;
  uint8_t block[4096];
  int out_count = 0;
  uint32_t block_count;

  if (!ext2_state.mounted || out == 0 || max_entries <= 0) {
    return -1;
  }
  if (!read_inode(EXT2_ROOT_INODE, &root) || !(root.mode & EXT2_S_IFDIR)) {
    return -1;
  }

  block_count = (root.size + ext2_state.block_size - 1u) / ext2_state.block_size;
  for (uint32_t b = 0; b < block_count && out_count < max_entries; b++) {
    int block_num = inode_get_block(&root, b, 0);
    if (block_num <= 0 || !read_block((uint32_t)block_num, block)) {
      continue;
    }

    uint32_t off = 0;
    while (off + 8u <= ext2_state.block_size && out_count < max_entries) {
      Ext2DirEntry *entry = (Ext2DirEntry *)(block + off);
      if (entry->rec_len == 0) {
        break;
      }
      if (entry->inode != 0 && entry->name_len > 0) {
        Ext2Inode inode;
        int copy_len = (entry->name_len < (uint8_t)(sizeof(out[out_count].name) - 1))
                           ? entry->name_len
                           : (int)(sizeof(out[out_count].name) - 1);
        if (read_inode(entry->inode, &inode)) {
          out[out_count].inode = entry->inode;
          out[out_count].size = inode.size;
          out[out_count].mtime = inode.mtime;
          for (int i = 0; i < copy_len; i++) {
            out[out_count].name[i] = entry->name[i];
          }
          out[out_count].name[copy_len] = '\0';
          out_count++;
        }
      }
      off += entry->rec_len;
    }
  }
  return out_count;
}

static int ext2_mount_from_lba(uint32_t partition_lba) {
  if (!ext2_init_state_from_partition(partition_lba)) {
    return 0;
  }
  for (int i = 0; i < EXT2_MAX_HANDLES; i++) {
    ext2_handles[i].used = 0;
    ext2_handles[i].ref_count = 0;
    ext2_handles[i].inode_num = 0;
    ext2_handles[i].pos = 0;
    ext2_handles[i].flags = 0;
  }
  return 1;
}

const VfsBackendOps *ext2_backend_from_spec(const char *spec) {
  uint32_t lba = 0;
  if (spec == 0) {
    return 0;
  }
  if (str_eq(spec, "ext2")) {
    PartitionEntry parts[PARTITION_MAX_ENTRIES];
    int count = partition_scan_gpt(parts, PARTITION_MAX_ENTRIES);
    if (count <= 0) {
      count = partition_scan_mbr(parts, PARTITION_MAX_ENTRIES);
    }
    for (int i = 0; i < count; i++) {
      if (ext2_mount_from_lba(parts[i].lba_start)) {
        return &ext2_backend_ops;
      }
    }
    return 0;
  }
  if (spec[0] == 'e' && spec[1] == 'x' && spec[2] == 't' && spec[3] == '2' &&
      (spec[4] == ':' || spec[4] == '@')) {
    if (!parse_u32_dec(spec + 5, &lba)) {
      return 0;
    }
    if (!ext2_mount_from_lba(lba)) {
      return 0;
    }
    return &ext2_backend_ops;
  }
  return 0;
}
