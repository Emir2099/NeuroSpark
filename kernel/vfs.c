#include "vfs.h"

#include "disk.h"
#include "task.h"
#include "ext2fs.h"

#define VFS_MAX_MOUNTS 8
#define VFS_MAX_GLOBAL_FDS 512
#define TFS_MAX_HANDLES 256
#define TFS_SECTOR_SIZE 512
#define RAMDISK_MAX_FILES 64
#define RAMDISK_MAX_HANDLES 128
#define RAMDISK_NAME_MAX 23
#define RAMDISK_FILE_CAPACITY 8192

typedef struct {
  int used;
  char mount_path[24];
  int mount_len;
  const VfsBackendOps *ops;
} VfsMount;

typedef struct {
  int used;
  const VfsBackendOps *ops;
  int backend_handle;
  int flags;
  int ref_count;
} VfsFd;

typedef struct {
  int used;
  int ref_count;
  int entry_idx;
  uint32_t pos;
  int flags;
} TfsHandle;

typedef struct {
  int used;
  char name[RAMDISK_NAME_MAX + 1];
  uint32_t size;
  uint32_t inode;
  uint32_t mtime;
  unsigned char data[RAMDISK_FILE_CAPACITY];
} RamdiskFile;

typedef struct {
  int used;
  int entry_idx;
  uint32_t pos;
  int flags;
} RamdiskHandle;

extern FileEntry root_directory[TFS_MAX_FILES];
extern void disk_write_sector(uint32_t lba, uint16_t *buffer);
extern void disk_read_sector(uint32_t lba, uint16_t *buffer);

static VfsMount mounts[VFS_MAX_MOUNTS];
static VfsFd fd_table[VFS_MAX_GLOBAL_FDS];
static int task_fd_map[MAX_TASKS][TASK_MAX_FDS];
static TfsHandle tfs_handles[TFS_MAX_HANDLES];
static RamdiskFile ramdisk_files[RAMDISK_MAX_FILES];
static RamdiskHandle ramdisk_handles[RAMDISK_MAX_HANDLES];
static uint32_t ramdisk_next_inode = 1;
static uint32_t ramdisk_mtime_tick = 1;

extern int os_current_task;

static int current_task_id(void) {
  if (os_current_task < 0 || os_current_task >= MAX_TASKS) {
    return 0;
  }
  return os_current_task;
}

static int alloc_task_fd(int task_id) {
  for (int i = 0; i < TASK_MAX_FDS; i++) {
    if (task_fd_map[task_id][i] < 0) {
      return i;
    }
  }
  return -1;
}

static int alloc_global_fd(void) {
  for (int i = 0; i < VFS_MAX_GLOBAL_FDS; i++) {
    if (!fd_table[i].used) {
      return i;
    }
  }
  return -1;
}

static int task_fd_to_global(int local_fd) {
  int task_id = current_task_id();
  if (local_fd < 0 || local_fd >= TASK_MAX_FDS) {
    return -1;
  }
  return task_fd_map[task_id][local_fd];
}

static void clear_file_entry(FileEntry *entry) {
  int i;
  if (entry == 0) {
    return;
  }
  for (i = 0; i < 12; i++) {
    entry->name[i] = 0;
  }
  entry->lba = 0;
  entry->size = 0;
  entry->flags = 0;
}

static int str_eq(const char *a, const char *b) {
  int i = 0;
  while (a[i] && b[i]) {
    if (a[i] != b[i]) {
      return 0;
    }
    i++;
  }
  return a[i] == '\0' && b[i] == '\0';
}

static int str_prefix(const char *s, const char *prefix) {
  int i = 0;
  while (prefix[i]) {
    if (s[i] != prefix[i]) {
      return 0;
    }
    i++;
  }
  return 1;
}

static int copy_text(char *dst, int dst_sz, const char *src) {
  int i = 0;
  if (!dst || dst_sz <= 0 || !src) {
    return 0;
  }
  while (src[i] && i < dst_sz - 1) {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';
  return i;
}

static int resolve_mount(const char *path, const VfsBackendOps **ops_out,
                         const char **rel_path_out) {
  int best = -1;
  if (!path || !ops_out || !rel_path_out) {
    return 0;
  }

  for (int i = 0; i < VFS_MAX_MOUNTS; i++) {
    if (!mounts[i].used) {
      continue;
    }
    if (str_eq(mounts[i].mount_path, "/")) {
      if (best < 0) {
        best = i;
      }
      continue;
    }
    if (str_prefix(path, mounts[i].mount_path)) {
      int ml = mounts[i].mount_len;
      if ((path[ml] == '\0' || path[ml] == '/') &&
          (best < 0 || mounts[i].mount_len > mounts[best].mount_len)) {
        best = i;
      }
    }
  }

  if (best < 0) {
    return 0;
  }

  *ops_out = mounts[best].ops;
  if (str_eq(mounts[best].mount_path, "/")) {
    *rel_path_out = path;
  } else {
    int ml = mounts[best].mount_len;
    *rel_path_out = (path[ml] == '\0') ? "/" : (path + ml);
  }
  return 1;
}

static int str_case_eq(const char *a, const char *b) {
  int i = 0;
  while (a[i] && b[i]) {
    char ca = a[i];
    char cb = b[i];
    if (ca >= 'A' && ca <= 'Z') {
      ca = (char)(ca + 32);
    }
    if (cb >= 'A' && cb <= 'Z') {
      cb = (char)(cb + 32);
    }
    if (ca != cb) {
      return 0;
    }
    i++;
  }
  return a[i] == '\0' && b[i] == '\0';
}

static int normalize_tfs_name(const char *path, char out[12]) {
  int src = 0;
  int dst = 0;

  if (path == 0 || path[0] == '\0') {
    return 0;
  }

  while (path[src] == '/') {
    src++;
  }

  if (path[src] == '\0') {
    return 0;
  }

  while (path[src] && path[src] != '/' && dst < 11) {
    out[dst++] = path[src++];
  }
  out[dst] = '\0';

  return dst > 0;
}

static int tfs_find_entry_by_name(const char *name) {
  for (int i = 0; i < TFS_MAX_FILES; i++) {
    if (root_directory[i].flags == 1 && str_case_eq(root_directory[i].name, name)) {
      return i;
    }
  }
  return -1;
}

static int tfs_alloc_handle(void) {
  for (int i = 0; i < TFS_MAX_HANDLES; i++) {
    if (!tfs_handles[i].used) {
      tfs_handles[i].used = 1;
      tfs_handles[i].ref_count = 1;
      tfs_handles[i].entry_idx = -1;
      tfs_handles[i].pos = 0;
      tfs_handles[i].flags = 0;
      return i;
    }
  }
  return -1;
}

static int tfs_open(const char *path, int flags) {
  char name[12];
  int entry_idx;
  int handle;

  if (!ata_disk_available) {
    return VFS_ERR_IO;
  }

  if (!normalize_tfs_name(path, name)) {
    return VFS_ERR_INVALID_ARG;
  }

  disk_read_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
  entry_idx = tfs_find_entry_by_name(name);

  if (entry_idx < 0) {
    if ((flags & VFS_O_CREAT) == 0) {
      return VFS_ERR_NOT_FOUND;
    }
    entry_idx = find_free_slot(root_directory);
    if (entry_idx < 0) {
      return VFS_ERR_NO_SPACE;
    }

    for (int i = 0; i < 12; i++) {
      root_directory[entry_idx].name[i] = name[i];
      if (name[i] == '\0') {
        break;
      }
    }
    root_directory[entry_idx].lba = TFS_DATA_START + entry_idx;
    root_directory[entry_idx].size = 0;
    root_directory[entry_idx].flags = 1;
    disk_write_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
  }

  if (flags & VFS_O_TRUNC) {
    root_directory[entry_idx].size = 0;
    disk_write_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
  }

  handle = tfs_alloc_handle();
  if (handle < 0) {
    return VFS_ERR_NO_SPACE;
  }

  tfs_handles[handle].entry_idx = entry_idx;
  tfs_handles[handle].pos = 0;
  tfs_handles[handle].flags = flags;

  return handle;
}

static int tfs_read(int handle, void *buf, uint32_t size) {
  TfsHandle *h;
  FileEntry *entry;
  uint8_t *dst = (uint8_t *)buf;
  uint16_t sector[TFS_SECTOR_SIZE / 2];
  uint32_t remaining;
  uint32_t total = 0;

  if (handle < 0 || handle >= TFS_MAX_HANDLES || !tfs_handles[handle].used) {
    return VFS_ERR_INVALID_FD;
  }
  if (buf == 0 || size == 0) {
    return VFS_ERR_INVALID_ARG;
  }

  h = &tfs_handles[handle];
  if ((h->flags & VFS_O_RDONLY) == 0 && (h->flags & VFS_O_RDWR) != VFS_O_RDWR) {
    return VFS_ERR_PERM;
  }
  entry = &root_directory[h->entry_idx];
  if (h->pos >= entry->size) {
    return 0;
  }

  remaining = entry->size - h->pos;
  if (size > remaining) {
    size = remaining;
  }

  while (total < size) {
    uint32_t abs_pos = h->pos + total;
    uint32_t sector_off = abs_pos % TFS_SECTOR_SIZE;
    uint32_t chunk = TFS_SECTOR_SIZE - sector_off;
    uint32_t lba = entry->lba + (abs_pos / TFS_SECTOR_SIZE);

    if (chunk > (size - total)) {
      chunk = size - total;
    }

    disk_read_sector(lba, sector);
    for (uint32_t i = 0; i < chunk; i++) {
      dst[total + i] = ((uint8_t *)sector)[sector_off + i];
    }
    total += chunk;
  }

  h->pos += total;
  return (int)total;
}

static int tfs_write(int handle, const void *buf, uint32_t size) {
  TfsHandle *h;
  FileEntry *entry;
  const uint8_t *src = (const uint8_t *)buf;
  uint16_t sector[TFS_SECTOR_SIZE / 2];
  uint32_t total = 0;

  if (handle < 0 || handle >= TFS_MAX_HANDLES || !tfs_handles[handle].used) {
    return VFS_ERR_INVALID_FD;
  }
  if (buf == 0 || size == 0) {
    return VFS_ERR_INVALID_ARG;
  }

  h = &tfs_handles[handle];
  entry = &root_directory[h->entry_idx];

  if (!(h->flags & VFS_O_WRONLY) && !(h->flags & VFS_O_RDWR)) {
    return VFS_ERR_PERM;
  }

  while (total < size) {
    uint32_t abs_pos = h->pos + total;
    uint32_t sector_off = abs_pos % TFS_SECTOR_SIZE;
    uint32_t chunk = TFS_SECTOR_SIZE - sector_off;
    uint32_t lba = entry->lba + (abs_pos / TFS_SECTOR_SIZE);

    if (lba >= (TFS_DATA_START + TFS_MAX_FILES)) {
      break;
    }

    if (chunk > (size - total)) {
      chunk = size - total;
    }

    disk_read_sector(lba, sector);
    for (uint32_t i = 0; i < chunk; i++) {
      ((uint8_t *)sector)[sector_off + i] = src[total + i];
    }
    disk_write_sector(lba, sector);
    total += chunk;
  }

  h->pos += total;
  if (h->pos > entry->size) {
    entry->size = h->pos;
    disk_write_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
  }

  return (int)total;
}

static int tfs_close(int handle) {
  if (handle < 0 || handle >= TFS_MAX_HANDLES || !tfs_handles[handle].used) {
    return VFS_ERR_INVALID_FD;
  }
  if (tfs_handles[handle].ref_count > 1) {
    tfs_handles[handle].ref_count--;
    return VFS_OK;
  }
  tfs_handles[handle].used = 0;
  tfs_handles[handle].ref_count = 0;
  return VFS_OK;
}

static int tfs_lseek(int handle, int offset, int whence) {
  TfsHandle *h;
  FileEntry *entry;
  int base = 0;
  int target;

  if (handle < 0 || handle >= TFS_MAX_HANDLES || !tfs_handles[handle].used) {
    return VFS_ERR_INVALID_FD;
  }

  h = &tfs_handles[handle];
  entry = &root_directory[h->entry_idx];

  if (whence == VFS_SEEK_SET) {
    base = 0;
  } else if (whence == VFS_SEEK_CUR) {
    base = (int)h->pos;
  } else if (whence == VFS_SEEK_END) {
    base = (int)entry->size;
  } else {
    return VFS_ERR_INVALID_ARG;
  }

  target = base + offset;
  if (target < 0) {
    return VFS_ERR_INVALID_ARG;
  }
  h->pos = (uint32_t)target;
  return target;
}

static int tfs_stat(const char *path, VfsFileStat *stat_out) {
  char name[12];
  int entry_idx;

  if (path == 0 || stat_out == 0) {
    return VFS_ERR_INVALID_ARG;
  }
  if (!ata_disk_available) {
    return VFS_ERR_IO;
  }
  if (!normalize_tfs_name(path, name)) {
    return VFS_ERR_INVALID_ARG;
  }

  disk_read_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
  entry_idx = tfs_find_entry_by_name(name);
  if (entry_idx < 0) {
    return VFS_ERR_NOT_FOUND;
  }

  stat_out->size = root_directory[entry_idx].size;
  stat_out->flags = root_directory[entry_idx].flags;
  return VFS_OK;
}

static int tfs_delete(const char *path) {
  char name[12];
  int entry_idx;

  if (!ata_disk_available) {
    return VFS_ERR_IO;
  }
  if (!normalize_tfs_name(path, name)) {
    return VFS_ERR_INVALID_ARG;
  }

  disk_read_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
  entry_idx = tfs_find_entry_by_name(name);
  if (entry_idx < 0) {
    return VFS_ERR_NOT_FOUND;
  }

  clear_file_entry(&root_directory[entry_idx]);
  disk_write_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
  return VFS_OK;
}

static const VfsBackendOps tfs_backend_ops = {
    tfs_open,
    tfs_read,
    tfs_write,
    tfs_close,
    tfs_delete,
  tfs_lseek,
  tfs_stat,
};

static int ramdisk_extract_name(const char *path, char *out_name) {
  int i = 0;
  int j = 0;

  if (path == 0 || out_name == 0) {
    return 0;
  }

  if (path[0] == '/') {
    i = 1;
  }

  while (path[i] != '\0' && path[i] != '/' && j < RAMDISK_NAME_MAX) {
    out_name[j++] = path[i++];
  }
  out_name[j] = '\0';

  if (j == 0 || path[i] != '\0') {
    return 0;
  }
  return 1;
}

static int ramdisk_find_entry(const char *name) {
  for (int i = 0; i < RAMDISK_MAX_FILES; i++) {
    if (ramdisk_files[i].used && str_eq(ramdisk_files[i].name, name)) {
      return i;
    }
  }
  return -1;
}

static int ramdisk_alloc_entry(void) {
  for (int i = 0; i < RAMDISK_MAX_FILES; i++) {
    if (!ramdisk_files[i].used) {
      return i;
    }
  }
  return -1;
}

static int ramdisk_alloc_handle(void) {
  for (int i = 0; i < RAMDISK_MAX_HANDLES; i++) {
    if (!ramdisk_handles[i].used) {
      return i;
    }
  }
  return -1;
}

static int ramdisk_open(const char *path, int flags) {
  char name[RAMDISK_NAME_MAX + 1];
  int entry_idx;
  int handle;

  if (!ramdisk_extract_name(path, name)) {
    return VFS_ERR_INVALID_ARG;
  }

  entry_idx = ramdisk_find_entry(name);
  if (entry_idx < 0) {
    if ((flags & VFS_O_CREAT) == 0) {
      return VFS_ERR_NOT_FOUND;
    }
    entry_idx = ramdisk_alloc_entry();
    if (entry_idx < 0) {
      return VFS_ERR_NO_SPACE;
    }
    ramdisk_files[entry_idx].used = 1;
    copy_text(ramdisk_files[entry_idx].name, RAMDISK_NAME_MAX + 1, name);
    ramdisk_files[entry_idx].size = 0;
    ramdisk_files[entry_idx].inode = ramdisk_next_inode++;
    ramdisk_files[entry_idx].mtime = ramdisk_mtime_tick++;
  } else if (flags & VFS_O_TRUNC) {
    ramdisk_files[entry_idx].size = 0;
    ramdisk_files[entry_idx].mtime = ramdisk_mtime_tick++;
  }

  handle = ramdisk_alloc_handle();
  if (handle < 0) {
    return VFS_ERR_NO_SPACE;
  }
  ramdisk_handles[handle].used = 1;
  ramdisk_handles[handle].entry_idx = entry_idx;
  ramdisk_handles[handle].pos = 0;
  ramdisk_handles[handle].flags = flags;
  return handle;
}

static int ramdisk_read(int handle, void *buf, uint32_t size) {
  RamdiskFile *file;
  uint32_t remaining;

  if (handle < 0 || handle >= RAMDISK_MAX_HANDLES || !ramdisk_handles[handle].used || buf == 0) {
    return VFS_ERR_INVALID_ARG;
  }
  if ((ramdisk_handles[handle].flags & VFS_O_RDONLY) == 0 &&
      (ramdisk_handles[handle].flags & VFS_O_RDWR) != VFS_O_RDWR) {
    return VFS_ERR_PERM;
  }

  file = &ramdisk_files[ramdisk_handles[handle].entry_idx];
  if (ramdisk_handles[handle].pos >= file->size) {
    return 0;
  }

  remaining = file->size - ramdisk_handles[handle].pos;
  if (size > remaining) {
    size = remaining;
  }

  for (uint32_t i = 0; i < size; i++) {
    ((unsigned char *)buf)[i] = file->data[ramdisk_handles[handle].pos + i];
  }
  ramdisk_handles[handle].pos += size;
  return (int)size;
}

static int ramdisk_write(int handle, const void *buf, uint32_t size) {
  RamdiskFile *file;
  uint32_t free_bytes;

  if (handle < 0 || handle >= RAMDISK_MAX_HANDLES || !ramdisk_handles[handle].used || buf == 0) {
    return VFS_ERR_INVALID_ARG;
  }
  if ((ramdisk_handles[handle].flags & VFS_O_WRONLY) == 0 &&
      (ramdisk_handles[handle].flags & VFS_O_RDWR) != VFS_O_RDWR) {
    return VFS_ERR_PERM;
  }

  file = &ramdisk_files[ramdisk_handles[handle].entry_idx];
  if (ramdisk_handles[handle].pos >= RAMDISK_FILE_CAPACITY) {
    return VFS_ERR_NO_SPACE;
  }

  free_bytes = RAMDISK_FILE_CAPACITY - ramdisk_handles[handle].pos;
  if (size > free_bytes) {
    size = free_bytes;
  }

  for (uint32_t i = 0; i < size; i++) {
    file->data[ramdisk_handles[handle].pos + i] = ((const unsigned char *)buf)[i];
  }

  ramdisk_handles[handle].pos += size;
  if (ramdisk_handles[handle].pos > file->size) {
    file->size = ramdisk_handles[handle].pos;
  }
  file->mtime = ramdisk_mtime_tick++;
  return (int)size;
}

static int ramdisk_close(int handle) {
  if (handle < 0 || handle >= RAMDISK_MAX_HANDLES || !ramdisk_handles[handle].used) {
    return VFS_ERR_INVALID_ARG;
  }
  ramdisk_handles[handle].used = 0;
  ramdisk_handles[handle].entry_idx = -1;
  ramdisk_handles[handle].pos = 0;
  ramdisk_handles[handle].flags = 0;
  return VFS_OK;
}

static int ramdisk_delete(const char *path) {
  char name[RAMDISK_NAME_MAX + 1];
  int entry_idx;

  if (!ramdisk_extract_name(path, name)) {
    return VFS_ERR_INVALID_ARG;
  }

  entry_idx = ramdisk_find_entry(name);
  if (entry_idx < 0) {
    return VFS_ERR_NOT_FOUND;
  }

  for (int i = 0; i < RAMDISK_MAX_HANDLES; i++) {
    if (ramdisk_handles[i].used && ramdisk_handles[i].entry_idx == entry_idx) {
      return VFS_ERR_PERM;
    }
  }

  ramdisk_files[entry_idx].used = 0;
  ramdisk_files[entry_idx].name[0] = '\0';
  ramdisk_files[entry_idx].size = 0;
  ramdisk_files[entry_idx].inode = 0;
  ramdisk_files[entry_idx].mtime = 0;
  return VFS_OK;
}

static int ramdisk_lseek(int handle, int offset, int whence) {
  RamdiskFile *file;
  int new_pos;

  if (handle < 0 || handle >= RAMDISK_MAX_HANDLES || !ramdisk_handles[handle].used) {
    return VFS_ERR_INVALID_ARG;
  }

  file = &ramdisk_files[ramdisk_handles[handle].entry_idx];
  if (whence == VFS_SEEK_SET) {
    new_pos = offset;
  } else if (whence == VFS_SEEK_CUR) {
    new_pos = (int)ramdisk_handles[handle].pos + offset;
  } else if (whence == VFS_SEEK_END) {
    new_pos = (int)file->size + offset;
  } else {
    return VFS_ERR_INVALID_ARG;
  }

  if (new_pos < 0) {
    new_pos = 0;
  }
  if (new_pos > (int)file->size) {
    new_pos = (int)file->size;
  }

  ramdisk_handles[handle].pos = (uint32_t)new_pos;
  return new_pos;
}

static int ramdisk_stat(const char *path, VfsFileStat *stat_out) {
  char name[RAMDISK_NAME_MAX + 1];
  int entry_idx;

  if (stat_out == 0 || !ramdisk_extract_name(path, name)) {
    return VFS_ERR_INVALID_ARG;
  }

  entry_idx = ramdisk_find_entry(name);
  if (entry_idx < 0) {
    return VFS_ERR_NOT_FOUND;
  }

  stat_out->size = ramdisk_files[entry_idx].size;
  stat_out->flags = 1;
  return VFS_OK;
}

static const VfsBackendOps ramdisk_backend_ops = {
    ramdisk_open,
    ramdisk_read,
    ramdisk_write,
    ramdisk_close,
    ramdisk_delete,
    ramdisk_lseek,
    ramdisk_stat,
};

static const VfsBackendOps *lookup_backend(const char *name) {
  if (name == 0) {
    return 0;
  }
  if (str_case_eq(name, "tfs")) {
    return &tfs_backend_ops;
  }
  if (str_case_eq(name, "ramdisk")) {
    return &ramdisk_backend_ops;
  }
  if (name[0] == 'e' && name[1] == 'x' && name[2] == 't' && name[3] == '2') {
    return ext2_backend_from_spec(name);
  }
  return 0;
}

static int close_global_fd(int gfd) {
  int rc = VFS_OK;
  if (gfd < 0 || gfd >= VFS_MAX_GLOBAL_FDS || !fd_table[gfd].used || fd_table[gfd].ops == 0) {
    return VFS_ERR_INVALID_FD;
  }

  if (fd_table[gfd].ref_count > 1) {
    fd_table[gfd].ref_count--;
    return VFS_OK;
  }

  if (fd_table[gfd].ops->close) {
    rc = fd_table[gfd].ops->close(fd_table[gfd].backend_handle);
  }
  fd_table[gfd].used = 0;
  fd_table[gfd].ops = 0;
  fd_table[gfd].backend_handle = -1;
  fd_table[gfd].flags = 0;
  fd_table[gfd].ref_count = 0;
  return rc;
}

void vfs_init(void) {
  for (int i = 0; i < VFS_MAX_MOUNTS; i++) {
    mounts[i].used = 0;
    mounts[i].mount_path[0] = '\0';
    mounts[i].mount_len = 0;
    mounts[i].ops = 0;
  }
  for (int i = 0; i < VFS_MAX_GLOBAL_FDS; i++) {
    fd_table[i].used = 0;
    fd_table[i].ops = 0;
    fd_table[i].backend_handle = -1;
    fd_table[i].flags = 0;
    fd_table[i].ref_count = 0;
  }
  for (int t = 0; t < MAX_TASKS; t++) {
    for (int i = 0; i < TASK_MAX_FDS; i++) {
      task_fd_map[t][i] = -1;
    }
  }
  for (int i = 0; i < TFS_MAX_HANDLES; i++) {
    tfs_handles[i].used = 0;
    tfs_handles[i].ref_count = 0;
  }
  for (int i = 0; i < RAMDISK_MAX_FILES; i++) {
    ramdisk_files[i].used = 0;
    ramdisk_files[i].name[0] = '\0';
    ramdisk_files[i].size = 0;
    ramdisk_files[i].inode = 0;
    ramdisk_files[i].mtime = 0;
  }
  for (int i = 0; i < RAMDISK_MAX_HANDLES; i++) {
    ramdisk_handles[i].used = 0;
    ramdisk_handles[i].entry_idx = -1;
    ramdisk_handles[i].pos = 0;
    ramdisk_handles[i].flags = 0;
  }
  ramdisk_next_inode = 1;
  ramdisk_mtime_tick = 1;

  vfs_mount("/", &tfs_backend_ops);
}

int vfs_mount(const char *mount_path, const VfsBackendOps *ops) {
  if (mount_path == 0 || ops == 0) {
    return VFS_ERR_INVALID_ARG;
  }
  if (mount_path[0] != '/') {
    return VFS_ERR_INVALID_ARG;
  }
  for (int i = 0; i < VFS_MAX_MOUNTS; i++) {
    if (mounts[i].used && str_eq(mounts[i].mount_path, mount_path)) {
      mounts[i].ops = ops;
      return VFS_OK;
    }
  }
  for (int i = 0; i < VFS_MAX_MOUNTS; i++) {
    if (!mounts[i].used) {
      mounts[i].used = 1;
      mounts[i].mount_len = copy_text(mounts[i].mount_path, sizeof(mounts[i].mount_path), mount_path);
      mounts[i].ops = ops;
      return VFS_OK;
    }
  }
  return VFS_ERR_NO_SPACE;
}

int vfs_umount(const char *mount_path) {
  if (mount_path == 0 || mount_path[0] != '/') {
    return VFS_ERR_INVALID_ARG;
  }

  if (mount_path && str_eq(mount_path, "/")) {
    return VFS_ERR_PERM;
  }

  for (int i = 0; i < VFS_MAX_MOUNTS; i++) {
    if (mounts[i].used && str_eq(mounts[i].mount_path, mount_path)) {
      mounts[i].used = 0;
      mounts[i].mount_path[0] = '\0';
      mounts[i].mount_len = 0;
      mounts[i].ops = 0;
      return VFS_OK;
    }
  }
  return VFS_ERR_NOT_FOUND;
}

int vfs_mount_backend(const char *mount_path, const char *backend_name) {
  const VfsBackendOps *ops = lookup_backend(backend_name);
  if (ops == 0) {
    return VFS_ERR_NOT_FOUND;
  }
  return vfs_mount(mount_path, ops);
}

int vfs_list_mounts(VfsMountInfo *out, int max_entries) {
  int count = 0;
  if (out == 0 || max_entries <= 0) {
    return VFS_ERR_INVALID_ARG;
  }
  for (int i = 0; i < VFS_MAX_MOUNTS && count < max_entries; i++) {
    if (mounts[i].used) {
      copy_text(out[count].mount_path, (int)sizeof(out[count].mount_path), mounts[i].mount_path);
      out[count].used = 1;
      count++;
    }
  }
  return count;
}

int vfs_open(const char *path, int flags) {
  const VfsBackendOps *ops;
  const char *rel_path;
  int backend_handle;
  int gfd;
  int lfd;
  int task_id = current_task_id();

  if (path == 0 || path[0] == '\0') {
    return VFS_ERR_INVALID_ARG;
  }

  if (!resolve_mount(path, &ops, &rel_path) || ops == 0 || ops->open == 0) {
    return VFS_ERR_NOT_FOUND;
  }

  backend_handle = ops->open(rel_path, flags);
  if (backend_handle < 0) {
    return backend_handle;
  }

  gfd = alloc_global_fd();
  lfd = alloc_task_fd(task_id);
  if (gfd < 0 || lfd < 0) {
    if (ops->close) {
      ops->close(backend_handle);
    }
    return VFS_ERR_NO_SPACE;
  }

  fd_table[gfd].used = 1;
  fd_table[gfd].ops = ops;
  fd_table[gfd].backend_handle = backend_handle;
  fd_table[gfd].flags = flags;
  fd_table[gfd].ref_count = 1;
  task_fd_map[task_id][lfd] = gfd;
  return lfd;
}

int vfs_read(int fd, void *buf, uint32_t size) {
  int gfd = task_fd_to_global(fd);
  if (gfd < 0 || gfd >= VFS_MAX_GLOBAL_FDS || !fd_table[gfd].used || fd_table[gfd].ops == 0 ||
      fd_table[gfd].ops->read == 0) {
    return VFS_ERR_INVALID_FD;
  }
  return fd_table[gfd].ops->read(fd_table[gfd].backend_handle, buf, size);
}

int vfs_write(int fd, const void *buf, uint32_t size) {
  int gfd = task_fd_to_global(fd);
  if (gfd < 0 || gfd >= VFS_MAX_GLOBAL_FDS || !fd_table[gfd].used || fd_table[gfd].ops == 0 ||
      fd_table[gfd].ops->write == 0) {
    return VFS_ERR_INVALID_FD;
  }
  return fd_table[gfd].ops->write(fd_table[gfd].backend_handle, buf, size);
}

int vfs_close(int fd) {
  int task_id = current_task_id();
  int gfd;
  if (fd < 0 || fd >= TASK_MAX_FDS) {
    return VFS_ERR_INVALID_FD;
  }
  gfd = task_fd_map[task_id][fd];
  if (gfd < 0) {
    return VFS_ERR_INVALID_FD;
  }
  task_fd_map[task_id][fd] = -1;
  return close_global_fd(gfd);
}

int vfs_lseek(int fd, int offset, int whence) {
  int gfd = task_fd_to_global(fd);
  if (gfd < 0 || gfd >= VFS_MAX_GLOBAL_FDS || !fd_table[gfd].used || fd_table[gfd].ops == 0 ||
      fd_table[gfd].ops->lseek == 0) {
    return VFS_ERR_INVALID_FD;
  }
  return fd_table[gfd].ops->lseek(fd_table[gfd].backend_handle, offset, whence);
}

int vfs_dup(int fd) {
  int task_id = current_task_id();
  int gfd = task_fd_to_global(fd);
  int newfd;

  if (gfd < 0 || gfd >= VFS_MAX_GLOBAL_FDS || !fd_table[gfd].used || fd_table[gfd].ops == 0) {
    return VFS_ERR_INVALID_FD;
  }

  newfd = alloc_task_fd(task_id);
  if (newfd < 0) {
    return VFS_ERR_NO_SPACE;
  }

  task_fd_map[task_id][newfd] = gfd;
  fd_table[gfd].ref_count++;
  return newfd;
}

int vfs_dup2(int fd, int newfd) {
  int task_id = current_task_id();
  int src_gfd = task_fd_to_global(fd);

  if (src_gfd < 0 || src_gfd >= VFS_MAX_GLOBAL_FDS || !fd_table[src_gfd].used ||
      fd_table[src_gfd].ops == 0) {
    return VFS_ERR_INVALID_FD;
  }
  if (newfd < 0 || newfd >= TASK_MAX_FDS) {
    return VFS_ERR_INVALID_ARG;
  }
  if (fd == newfd) {
    return newfd;
  }

  if (task_fd_map[task_id][newfd] >= 0) {
    vfs_close(newfd);
  }

  task_fd_map[task_id][newfd] = src_gfd;
  fd_table[src_gfd].ref_count++;
  return newfd;
}

int vfs_fcntl(int fd, int cmd, int arg) {
  int gfd = task_fd_to_global(fd);
  if (gfd < 0 || gfd >= VFS_MAX_GLOBAL_FDS || !fd_table[gfd].used) {
    return VFS_ERR_INVALID_FD;
  }
  if (cmd == VFS_F_GETFL) {
    return fd_table[gfd].flags;
  }
  if (cmd == VFS_F_SETFL) {
    fd_table[gfd].flags = arg;
    return VFS_OK;
  }
  return VFS_ERR_INVALID_ARG;
}

void vfs_close_all_for_task(int task_id) {
  if (task_id < 0 || task_id >= MAX_TASKS) {
    return;
  }
  for (int i = 0; i < TASK_MAX_FDS; i++) {
    int gfd = task_fd_map[task_id][i];
    if (gfd >= 0) {
      task_fd_map[task_id][i] = -1;
      close_global_fd(gfd);
    }
  }
}

int vfs_clone_task_fds(int src_task_id, int dst_task_id) {
  if (src_task_id < 0 || src_task_id >= MAX_TASKS || dst_task_id < 0 || dst_task_id >= MAX_TASKS) {
    return VFS_ERR_INVALID_ARG;
  }

  for (int i = 0; i < TASK_MAX_FDS; i++) {
    task_fd_map[dst_task_id][i] = -1;
  }

  for (int i = 0; i < TASK_MAX_FDS; i++) {
    int gfd = task_fd_map[src_task_id][i];
    if (gfd >= 0 && gfd < VFS_MAX_GLOBAL_FDS && fd_table[gfd].used) {
      task_fd_map[dst_task_id][i] = gfd;
      fd_table[gfd].ref_count++;
    }
  }

  return VFS_OK;
}

int vfs_read_file(const char *path, void *buf, uint32_t max_size) {
  int fd = vfs_open(path, VFS_O_RDONLY);
  int total = 0;

  if (fd < 0) {
    return fd;
  }

  while ((uint32_t)total < max_size) {
    int n = vfs_read(fd, (uint8_t *)buf + total, max_size - (uint32_t)total);
    if (n < 0) {
      vfs_close(fd);
      return n;
    }
    if (n == 0) {
      break;
    }
    total += n;
  }

  vfs_close(fd);
  return total;
}

int vfs_write_file(const char *path, const void *buf, uint32_t size) {
  int fd = vfs_open(path, VFS_O_WRONLY | VFS_O_CREAT | VFS_O_TRUNC);
  int total = 0;

  if (fd < 0) {
    return fd;
  }

  while ((uint32_t)total < size) {
    int n = vfs_write(fd, (const uint8_t *)buf + total, size - (uint32_t)total);
    if (n <= 0) {
      vfs_close(fd);
      return VFS_ERR_IO;
    }
    total += n;
  }

  vfs_close(fd);
  return total;
}

int vfs_delete(const char *path) {
  const VfsBackendOps *ops;
  const char *rel_path;

  if (path == 0 || path[0] == '\0') {
    return VFS_ERR_INVALID_ARG;
  }

  if (!resolve_mount(path, &ops, &rel_path) || ops == 0 || ops->delete == 0) {
    return VFS_ERR_NOT_FOUND;
  }

  return ops->delete(rel_path);
}

int vfs_stat(const char *path, VfsFileStat *stat_out) {
  const VfsBackendOps *ops;
  const char *rel_path;

  if (path == 0 || stat_out == 0) {
    return VFS_ERR_INVALID_ARG;
  }

  if (!resolve_mount(path, &ops, &rel_path) || ops == 0 || ops->stat == 0) {
    return VFS_ERR_NOT_FOUND;
  }

  return ops->stat(rel_path, stat_out);
}
