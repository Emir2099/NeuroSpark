#include "vfs.h"

#include "disk.h"

#define VFS_MAX_MOUNTS 4
#define VFS_MAX_FDS 16
#define TFS_MAX_HANDLES 16
#define TFS_SECTOR_SIZE 512

typedef struct {
  int used;
  const char *mount_path;
  const VfsBackendOps *ops;
} VfsMount;

typedef struct {
  int used;
  const VfsBackendOps *ops;
  int backend_handle;
} VfsFd;

typedef struct {
  int used;
  int entry_idx;
  uint32_t pos;
  int flags;
} TfsHandle;

extern FileEntry root_directory[TFS_MAX_FILES];
extern void disk_write_sector(uint32_t lba, uint16_t *buffer);
extern void disk_read_sector(uint32_t lba, uint16_t *buffer);

static VfsMount mounts[VFS_MAX_MOUNTS];
static VfsFd fd_table[VFS_MAX_FDS];
static TfsHandle tfs_handles[TFS_MAX_HANDLES];

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
    return -1;
  }

  if (!normalize_tfs_name(path, name)) {
    return -1;
  }

  disk_read_sector(TFS_DIR_LBA, (uint16_t *)root_directory);
  entry_idx = tfs_find_entry_by_name(name);

  if (entry_idx < 0) {
    if ((flags & VFS_O_CREAT) == 0) {
      return -1;
    }
    entry_idx = find_free_slot(root_directory);
    if (entry_idx < 0) {
      return -1;
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
    return -1;
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

  if (handle < 0 || handle >= TFS_MAX_HANDLES || !tfs_handles[handle].used ||
      buf == 0 || size == 0) {
    return -1;
  }

  h = &tfs_handles[handle];
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

  if (handle < 0 || handle >= TFS_MAX_HANDLES || !tfs_handles[handle].used ||
      buf == 0 || size == 0) {
    return -1;
  }

  h = &tfs_handles[handle];
  entry = &root_directory[h->entry_idx];

  if (!(h->flags & VFS_O_WRONLY) && !(h->flags & VFS_O_RDWR)) {
    return -1;
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
    return -1;
  }
  tfs_handles[handle].used = 0;
  return 0;
}

static const VfsBackendOps tfs_backend_ops = {
    tfs_open,
    tfs_read,
    tfs_write,
    tfs_close,
};

void vfs_init(void) {
  for (int i = 0; i < VFS_MAX_MOUNTS; i++) {
    mounts[i].used = 0;
    mounts[i].mount_path = 0;
    mounts[i].ops = 0;
  }
  for (int i = 0; i < VFS_MAX_FDS; i++) {
    fd_table[i].used = 0;
    fd_table[i].ops = 0;
    fd_table[i].backend_handle = -1;
  }
  for (int i = 0; i < TFS_MAX_HANDLES; i++) {
    tfs_handles[i].used = 0;
  }

  vfs_mount("/", &tfs_backend_ops);
}

int vfs_mount(const char *mount_path, const VfsBackendOps *ops) {
  if (mount_path == 0 || ops == 0) {
    return -1;
  }
  for (int i = 0; i < VFS_MAX_MOUNTS; i++) {
    if (!mounts[i].used) {
      mounts[i].used = 1;
      mounts[i].mount_path = mount_path;
      mounts[i].ops = ops;
      return 0;
    }
  }
  return -1;
}

static const VfsBackendOps *vfs_find_backend(const char *path) {
  int best_len = -1;
  const VfsBackendOps *best = 0;

  for (int i = 0; i < VFS_MAX_MOUNTS; i++) {
    if (!mounts[i].used) {
      continue;
    }

    const char *mount = mounts[i].mount_path;
    int mount_len = 0;
    while (mount[mount_len]) {
      mount_len++;
    }

    if (str_eq(mount, "/") || str_prefix(path, mount)) {
      if (mount_len > best_len) {
        best_len = mount_len;
        best = mounts[i].ops;
      }
    }
  }

  return best;
}

int vfs_open(const char *path, int flags) {
  const VfsBackendOps *ops;
  int backend_handle;

  if (path == 0 || path[0] == '\0') {
    return -1;
  }

  ops = vfs_find_backend(path);
  if (ops == 0 || ops->open == 0) {
    return -1;
  }

  backend_handle = ops->open(path, flags);
  if (backend_handle < 0) {
    return -1;
  }

  for (int i = 0; i < VFS_MAX_FDS; i++) {
    if (!fd_table[i].used) {
      fd_table[i].used = 1;
      fd_table[i].ops = ops;
      fd_table[i].backend_handle = backend_handle;
      return i;
    }
  }

  ops->close(backend_handle);
  return -1;
}

int vfs_read(int fd, void *buf, uint32_t size) {
  if (fd < 0 || fd >= VFS_MAX_FDS || !fd_table[fd].used || fd_table[fd].ops == 0 ||
      fd_table[fd].ops->read == 0) {
    return -1;
  }
  return fd_table[fd].ops->read(fd_table[fd].backend_handle, buf, size);
}

int vfs_write(int fd, const void *buf, uint32_t size) {
  if (fd < 0 || fd >= VFS_MAX_FDS || !fd_table[fd].used || fd_table[fd].ops == 0 ||
      fd_table[fd].ops->write == 0) {
    return -1;
  }
  return fd_table[fd].ops->write(fd_table[fd].backend_handle, buf, size);
}

int vfs_close(int fd) {
  int rc;
  if (fd < 0 || fd >= VFS_MAX_FDS || !fd_table[fd].used || fd_table[fd].ops == 0 ||
      fd_table[fd].ops->close == 0) {
    return -1;
  }

  rc = fd_table[fd].ops->close(fd_table[fd].backend_handle);
  fd_table[fd].used = 0;
  fd_table[fd].ops = 0;
  fd_table[fd].backend_handle = -1;
  return rc;
}

int vfs_read_file(const char *path, void *buf, uint32_t max_size) {
  int fd = vfs_open(path, VFS_O_RDONLY);
  int total = 0;

  if (fd < 0) {
    return -1;
  }

  while ((uint32_t)total < max_size) {
    int n = vfs_read(fd, (uint8_t *)buf + total, max_size - (uint32_t)total);
    if (n < 0) {
      vfs_close(fd);
      return -1;
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
    return VFS_ERR_NOT_FOUND;
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

int vfs_stat(const char *path, VfsFileStat *stat_out) {
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
