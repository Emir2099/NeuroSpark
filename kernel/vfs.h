#ifndef VFS_H
#define VFS_H

typedef unsigned int uint32_t;

/* ===== VFS Error Codes ===== */
#define VFS_OK              0
#define VFS_ERR_INVALID_FD  -1
#define VFS_ERR_NOT_FOUND   -2
#define VFS_ERR_NO_SPACE    -3
#define VFS_ERR_INVALID_ARG -4
#define VFS_ERR_IO          -5
#define VFS_ERR_PERM        -6

/* ===== VFS File Operations Flags ===== */
#define VFS_O_RDONLY 0x1
#define VFS_O_WRONLY 0x2
#define VFS_O_RDWR (VFS_O_RDONLY | VFS_O_WRONLY)
#define VFS_O_CREAT 0x4
#define VFS_O_TRUNC 0x8

typedef struct {
  int (*open)(const char *path, int flags);
  int (*read)(int handle, void *buf, uint32_t size);
  int (*write)(int handle, const void *buf, uint32_t size);
  int (*close)(int handle);
} VfsBackendOps;

/* Initialize VFS and mount default filesystems */
void vfs_init(void);

/* Mount a filesystem backend at a path */
int vfs_mount(const char *mount_path, const VfsBackendOps *ops);

/* Core VFS operations: open, read, write, close */
int vfs_open(const char *path, int flags);
int vfs_read(int fd, void *buf, uint32_t size);
int vfs_write(int fd, const void *buf, uint32_t size);
int vfs_close(int fd);

/* Convenience functions: single-call file I/O */
int vfs_read_file(const char *path, void *buf, uint32_t max_size);
int vfs_write_file(const char *path, const void *buf, uint32_t size);

/* File metadata query */
typedef struct {
  uint32_t size;   /* File size in bytes */
  uint32_t flags;  /* File flags (1=exists, 0=deleted) */
} VfsFileStat;

int vfs_stat(const char *path, VfsFileStat *stat_out);

#endif
