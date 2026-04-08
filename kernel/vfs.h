#ifndef VFS_H
#define VFS_H

typedef unsigned int uint32_t;

typedef struct VfsFileStat {
  uint32_t size;   /* File size in bytes */
  uint32_t flags;  /* File flags (1=exists, 0=deleted) */
} VfsFileStat;

/* ===== VFS Error Codes ===== */
#define VFS_OK              0
#define VFS_ERR_INVALID_FD  -1
#define VFS_ERR_NOT_FOUND   -2
#define VFS_ERR_NO_SPACE    -3
#define VFS_ERR_INVALID_ARG -4
#define VFS_ERR_IO          -5
#define VFS_ERR_PERM        -6
#define VFS_ERR_WOULD_BLOCK -7

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
  int (*delete)(const char *path);
  int (*lseek)(int handle, int offset, int whence);
  int (*stat)(const char *path, struct VfsFileStat *stat_out);
  int (*rename)(const char *old_path, const char *new_path);
  int (*mkdir)(const char *path, int mode);
  int (*rmdir)(const char *path);
  int (*truncate)(const char *path, uint32_t size);
  int (*ftruncate)(int handle, uint32_t size);
  int (*chmod)(const char *path, int mode);
  int (*chown)(const char *path, int uid, int gid);
  int (*link)(const char *old_path, const char *new_path);
  int (*symlink)(const char *target, const char *link_path);
  int (*readlink)(const char *link_path, char *target_buf, uint32_t max_len);
} VfsBackendOps;

/* Initialize VFS and mount default filesystems */
void vfs_init(void);

/* Mount a filesystem backend at a path */
int vfs_mount(const char *mount_path, const VfsBackendOps *ops);
int vfs_umount(const char *mount_path);
int vfs_mount_backend(const char *mount_path, const char *backend_name);

/* Core VFS operations: open, read, write, close */
int vfs_open(const char *path, int flags);
int vfs_read(int fd, void *buf, uint32_t size);
int vfs_write(int fd, const void *buf, uint32_t size);
int vfs_close(int fd);
int vfs_lseek(int fd, int offset, int whence);
int vfs_dup(int fd);
int vfs_dup2(int fd, int newfd);
int vfs_fcntl(int fd, int cmd, int arg);
void vfs_close_all_for_task(int task_id);
int vfs_clone_task_fds(int src_task_id, int dst_task_id);

typedef struct {
  char mount_path[24];
  int used;
} VfsMountInfo;

int vfs_list_mounts(VfsMountInfo *out, int max_entries);

/* Convenience functions: single-call file I/O */
int vfs_read_file(const char *path, void *buf, uint32_t max_size);
int vfs_write_file(const char *path, const void *buf, uint32_t size);
int vfs_delete(const char *path);

int vfs_stat(const char *path, VfsFileStat *stat_out);
int vfs_rename(const char *old_path, const char *new_path);
int vfs_mkdir(const char *path, int mode);
int vfs_rmdir(const char *path);
int vfs_truncate(const char *path, uint32_t size);
int vfs_ftruncate(int fd, uint32_t size);
int vfs_chmod(const char *path, int mode);
int vfs_chown(const char *path, int uid, int gid);
int vfs_link(const char *old_path, const char *new_path);
int vfs_symlink(const char *target, const char *link_path);
int vfs_readlink(const char *link_path, char *target_buf, uint32_t max_len);
int vfs_pipe(int *read_fd_out, int *write_fd_out);

/* Seek semantics */
#define VFS_SEEK_SET 0
#define VFS_SEEK_CUR 1
#define VFS_SEEK_END 2

/* fcntl commands (minimal subset) */
#define VFS_F_GETFL 1
#define VFS_F_SETFL 2

#endif
