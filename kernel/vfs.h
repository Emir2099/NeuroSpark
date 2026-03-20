#ifndef VFS_H
#define VFS_H

typedef unsigned int uint32_t;

typedef struct {
  int (*open)(const char *path, int flags);
  int (*read)(int handle, void *buf, uint32_t size);
  int (*write)(int handle, const void *buf, uint32_t size);
  int (*close)(int handle);
} VfsBackendOps;

#define VFS_O_RDONLY 0x1
#define VFS_O_WRONLY 0x2
#define VFS_O_RDWR (VFS_O_RDONLY | VFS_O_WRONLY)
#define VFS_O_CREAT 0x4
#define VFS_O_TRUNC 0x8

void vfs_init(void);
int vfs_mount(const char *mount_path, const VfsBackendOps *ops);

int vfs_open(const char *path, int flags);
int vfs_read(int fd, void *buf, uint32_t size);
int vfs_write(int fd, const void *buf, uint32_t size);
int vfs_close(int fd);

int vfs_read_file(const char *path, void *buf, uint32_t max_size);
int vfs_write_file(const char *path, const void *buf, uint32_t size);

#endif
