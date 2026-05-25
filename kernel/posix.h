#ifndef POSIX_H
#define POSIX_H

typedef unsigned int uint32_t;

#define POSIX_ACC_EXEC 0x1
#define POSIX_ACC_WRITE 0x2
#define POSIX_ACC_READ 0x4

#define POSIX_TTY_MAX 4
#define POSIX_NAME_MAX 16

void posix_init(void);
void posix_task_init(int task_id, int parent_task_id);
void posix_task_on_fork(int parent_task_id, int child_task_id);

uint32_t posix_get_uid(int task_id);
uint32_t posix_get_euid(int task_id);
uint32_t posix_get_gid(int task_id);
uint32_t posix_get_egid(int task_id);

int posix_set_uid(int task_id, uint32_t uid);
int posix_set_gid(int task_id, uint32_t gid);

int posix_check_path_permission(int task_id, const char *path, int access_mask);
int posix_path_chmod(int task_id, const char *path, int mode);
int posix_path_chown(int task_id, const char *path, int uid, int gid);
int posix_get_mode(const char *path, uint32_t *mode_out);
int posix_get_owner(const char *path, uint32_t *uid_out, uint32_t *gid_out);

void posix_track_fd_open(int task_id, int fd, const char *path, int flags);
void posix_track_fd_close(int task_id, int fd);
void posix_clone_fd_rights(int src_task_id, int dst_task_id);
int posix_check_fd_permission(int task_id, int fd, int access_mask);
void posix_clear_task_fds(int task_id);

int posix_apply_exec_credentials(int task_id, const char *path);

int posix_lookup_user_name(uint32_t uid, char *out, int out_cap);
int posix_lookup_group_name(uint32_t gid, char *out, int out_cap);

int posix_tty_tcsetpgrp(int task_id, int tty_id, int pgid);
int posix_tty_tcgetpgrp(int tty_id);
int posix_task_get_pgid(int task_id);
int posix_task_set_pgid(int task_id, int pgid);
int posix_task_signal_pgid(int sender_task_id, int pgid, int signal);
void posix_tty_signal_foreground(int tty_id, int signal);

#endif
