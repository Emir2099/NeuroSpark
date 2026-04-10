#include "posix.h"

#include "task.h"
#include "vfs.h"

typedef unsigned char uint8_t;

#define POSIX_PATH_META_MAX 128
#define POSIX_PATH_MAX 64
#define POSIX_DB_USERS_MAX 8
#define POSIX_DB_GROUPS_MAX 8

#define POSIX_MODE_XUSR 0100u
#define POSIX_MODE_WUSR 0200u
#define POSIX_MODE_RUSR 0400u
#define POSIX_MODE_XGRP 0010u
#define POSIX_MODE_WGRP 0020u
#define POSIX_MODE_RGRP 0040u
#define POSIX_MODE_XOTH 0001u
#define POSIX_MODE_WOTH 0002u
#define POSIX_MODE_ROTH 0004u
#define POSIX_MODE_SETUID 04000u
#define POSIX_MODE_SETGID 02000u

typedef struct {
  int used;
  char path[POSIX_PATH_MAX];
  uint32_t uid;
  uint32_t gid;
  uint32_t mode;
} PosixPathMeta;

typedef struct {
  uint32_t uid;
  uint32_t gid;
  uint32_t euid;
  uint32_t egid;
  int sid;
  int pgid;
  int tty;
} PosixIdentity;

typedef struct {
  int used;
  uint32_t uid;
  uint32_t gid;
  char name[POSIX_NAME_MAX];
} PosixUser;

typedef struct {
  int used;
  uint32_t gid;
  char name[POSIX_NAME_MAX];
} PosixGroup;

static PosixPathMeta g_path_meta[POSIX_PATH_META_MAX];
static PosixIdentity g_id[MAX_TASKS];
static uint8_t g_fd_rights[MAX_TASKS][TASK_MAX_FDS];
static int g_tty_fg_pgrp[POSIX_TTY_MAX];
static int g_tty_sid[POSIX_TTY_MAX];
static PosixUser g_users[POSIX_DB_USERS_MAX];
static PosixGroup g_groups[POSIX_DB_GROUPS_MAX];

static void clear_name(char *dst, int cap) {
  if (dst == 0 || cap <= 0) {
    return;
  }
  for (int i = 0; i < cap; i++) {
    dst[i] = '\0';
  }
}

static void name_copy(char *dst, int cap, const char *src) {
  int i = 0;
  if (dst == 0 || src == 0 || cap <= 0) {
    return;
  }
  while (src[i] && i < cap - 1) {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';
}

static void db_add_user(uint32_t uid, uint32_t gid, const char *name) {
  for (int i = 0; i < POSIX_DB_USERS_MAX; i++) {
    if (!g_users[i].used) {
      g_users[i].used = 1;
      g_users[i].uid = uid;
      g_users[i].gid = gid;
      name_copy(g_users[i].name, POSIX_NAME_MAX, name);
      return;
    }
  }
}

static void db_add_group(uint32_t gid, const char *name) {
  for (int i = 0; i < POSIX_DB_GROUPS_MAX; i++) {
    if (!g_groups[i].used) {
      g_groups[i].used = 1;
      g_groups[i].gid = gid;
      name_copy(g_groups[i].name, POSIX_NAME_MAX, name);
      return;
    }
  }
}

static int str_eq_local(const char *a, const char *b) {
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

static int path_has_suffix(const char *path, const char *suffix) {
  int p = 0;
  int s = 0;
  if (path == 0 || suffix == 0) {
    return 0;
  }
  while (path[p]) {
    p++;
  }
  while (suffix[s]) {
    s++;
  }
  if (s > p) {
    return 0;
  }
  for (int i = 0; i < s; i++) {
    if (path[p - s + i] != suffix[i]) {
      return 0;
    }
  }
  return 1;
}

static void path_copy(char *dst, const char *src) {
  int i = 0;
  if (dst == 0 || src == 0) {
    return;
  }
  while (src[i] && i < POSIX_PATH_MAX - 1) {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';
}

static PosixPathMeta *meta_find(const char *path) {
  if (path == 0 || path[0] == '\0') {
    return 0;
  }
  for (int i = 0; i < POSIX_PATH_META_MAX; i++) {
    if (g_path_meta[i].used && str_eq_local(g_path_meta[i].path, path)) {
      return &g_path_meta[i];
    }
  }
  return 0;
}

static PosixPathMeta *meta_get_or_create(const char *path) {
  PosixPathMeta *m = meta_find(path);
  if (m != 0) {
    return m;
  }

  for (int i = 0; i < POSIX_PATH_META_MAX; i++) {
    if (!g_path_meta[i].used) {
      g_path_meta[i].used = 1;
      path_copy(g_path_meta[i].path, path);
      g_path_meta[i].uid = 0;
      g_path_meta[i].gid = 0;
      if (path_has_suffix(path, ".bin") || path_has_suffix(path, ".elf")) {
        g_path_meta[i].mode = 0755u;
      } else {
        g_path_meta[i].mode = 0664u;
      }
      return &g_path_meta[i];
    }
  }

  return 0;
}

static uint32_t permission_bits_for_identity(const PosixPathMeta *m, int task_id) {
  uint32_t mode;
  if (m == 0 || task_id < 0 || task_id >= MAX_TASKS) {
    return 0;
  }

  if (g_id[task_id].euid == 0) {
    return POSIX_ACC_READ | POSIX_ACC_WRITE | POSIX_ACC_EXEC;
  }

  mode = m->mode;
  if (g_id[task_id].euid == m->uid) {
    return ((mode & POSIX_MODE_RUSR) ? POSIX_ACC_READ : 0) |
           ((mode & POSIX_MODE_WUSR) ? POSIX_ACC_WRITE : 0) |
           ((mode & POSIX_MODE_XUSR) ? POSIX_ACC_EXEC : 0);
  }
  if (g_id[task_id].egid == m->gid) {
    return ((mode & POSIX_MODE_RGRP) ? POSIX_ACC_READ : 0) |
           ((mode & POSIX_MODE_WGRP) ? POSIX_ACC_WRITE : 0) |
           ((mode & POSIX_MODE_XGRP) ? POSIX_ACC_EXEC : 0);
  }

  return ((mode & POSIX_MODE_ROTH) ? POSIX_ACC_READ : 0) |
         ((mode & POSIX_MODE_WOTH) ? POSIX_ACC_WRITE : 0) |
         ((mode & POSIX_MODE_XOTH) ? POSIX_ACC_EXEC : 0);
}

static int task_valid(int task_id) {
  return task_id >= 0 && task_id < MAX_TASKS;
}

void posix_init(void) {
  for (int i = 0; i < POSIX_PATH_META_MAX; i++) {
    g_path_meta[i].used = 0;
    g_path_meta[i].path[0] = '\0';
    g_path_meta[i].uid = 0;
    g_path_meta[i].gid = 0;
    g_path_meta[i].mode = 0664u;
  }

  for (int t = 0; t < MAX_TASKS; t++) {
    g_id[t].uid = 0;
    g_id[t].gid = 0;
    g_id[t].euid = 0;
    g_id[t].egid = 0;
    g_id[t].sid = t;
    g_id[t].pgid = t;
    g_id[t].tty = 0;

    for (int fd = 0; fd < TASK_MAX_FDS; fd++) {
      g_fd_rights[t][fd] = 0;
    }
  }

  for (int tty = 0; tty < POSIX_TTY_MAX; tty++) {
    g_tty_fg_pgrp[tty] = 0;
    g_tty_sid[tty] = 0;
  }

  for (int i = 0; i < POSIX_DB_USERS_MAX; i++) {
    g_users[i].used = 0;
    g_users[i].uid = 0;
    g_users[i].gid = 0;
    clear_name(g_users[i].name, POSIX_NAME_MAX);
  }

  for (int i = 0; i < POSIX_DB_GROUPS_MAX; i++) {
    g_groups[i].used = 0;
    g_groups[i].gid = 0;
    clear_name(g_groups[i].name, POSIX_NAME_MAX);
  }

  db_add_group(0u, "root");
  db_add_group(100u, "lab");
  db_add_group(200u, "guest");
  db_add_user(0u, 0u, "root");
  db_add_user(1000u, 100u, "user1");
  db_add_user(1001u, 100u, "user2");
}

void posix_task_init(int task_id, int parent_task_id) {
  if (!task_valid(task_id)) {
    return;
  }

  if (task_valid(parent_task_id)) {
    g_id[task_id] = g_id[parent_task_id];
  } else {
    g_id[task_id].uid = 0;
    g_id[task_id].gid = 0;
    g_id[task_id].euid = 0;
    g_id[task_id].egid = 0;
    g_id[task_id].sid = task_id;
    g_id[task_id].pgid = task_id;
    g_id[task_id].tty = 0;
  }

  for (int fd = 0; fd < TASK_MAX_FDS; fd++) {
    g_fd_rights[task_id][fd] = 0;
  }
}

void posix_task_on_fork(int parent_task_id, int child_task_id) {
  if (!task_valid(parent_task_id) || !task_valid(child_task_id)) {
    return;
  }
  g_id[child_task_id] = g_id[parent_task_id];
}

uint32_t posix_get_uid(int task_id) {
  return task_valid(task_id) ? g_id[task_id].uid : 0;
}

uint32_t posix_get_euid(int task_id) {
  return task_valid(task_id) ? g_id[task_id].euid : 0;
}

uint32_t posix_get_gid(int task_id) {
  return task_valid(task_id) ? g_id[task_id].gid : 0;
}

uint32_t posix_get_egid(int task_id) {
  return task_valid(task_id) ? g_id[task_id].egid : 0;
}

int posix_set_uid(int task_id, uint32_t uid) {
  if (!task_valid(task_id)) {
    return VFS_ERR_INVALID_ARG;
  }

  if (g_id[task_id].euid == 0) {
    g_id[task_id].uid = uid;
    g_id[task_id].euid = uid;
    return VFS_OK;
  }

  if (uid == g_id[task_id].uid || uid == g_id[task_id].euid) {
    g_id[task_id].euid = uid;
    return VFS_OK;
  }

  return VFS_ERR_PERM;
}

int posix_set_gid(int task_id, uint32_t gid) {
  if (!task_valid(task_id)) {
    return VFS_ERR_INVALID_ARG;
  }

  if (g_id[task_id].euid == 0) {
    g_id[task_id].gid = gid;
    g_id[task_id].egid = gid;
    return VFS_OK;
  }

  if (gid == g_id[task_id].gid || gid == g_id[task_id].egid) {
    g_id[task_id].egid = gid;
    return VFS_OK;
  }

  return VFS_ERR_PERM;
}

int posix_check_path_permission(int task_id, const char *path, int access_mask) {
  PosixPathMeta *m;
  uint32_t bits;

  if (!task_valid(task_id) || path == 0 || path[0] == '\0') {
    return VFS_ERR_INVALID_ARG;
  }

  m = meta_get_or_create(path);
  if (m == 0) {
    return VFS_ERR_NO_SPACE;
  }

  bits = permission_bits_for_identity(m, task_id);
  if (((uint32_t)access_mask & bits) != (uint32_t)access_mask) {
    return VFS_ERR_PERM;
  }
  return VFS_OK;
}

int posix_path_chmod(int task_id, const char *path, int mode) {
  PosixPathMeta *m;
  if (!task_valid(task_id) || path == 0) {
    return VFS_ERR_INVALID_ARG;
  }

  m = meta_get_or_create(path);
  if (m == 0) {
    return VFS_ERR_NO_SPACE;
  }

  if (g_id[task_id].euid != 0 && g_id[task_id].euid != m->uid) {
    return VFS_ERR_PERM;
  }

  m->mode = (uint32_t)(mode & 07777);
  return vfs_chmod(path, mode);
}

int posix_path_chown(int task_id, const char *path, int uid, int gid) {
  PosixPathMeta *m;
  if (!task_valid(task_id) || path == 0) {
    return VFS_ERR_INVALID_ARG;
  }

  if (g_id[task_id].euid != 0) {
    return VFS_ERR_PERM;
  }

  m = meta_get_or_create(path);
  if (m == 0) {
    return VFS_ERR_NO_SPACE;
  }

  m->uid = (uint32_t)uid;
  m->gid = (uint32_t)gid;
  return vfs_chown(path, uid, gid);
}

int posix_get_mode(const char *path, uint32_t *mode_out) {
  PosixPathMeta *m;
  if (path == 0 || mode_out == 0) {
    return VFS_ERR_INVALID_ARG;
  }
  m = meta_get_or_create(path);
  if (m == 0) {
    return VFS_ERR_NO_SPACE;
  }
  *mode_out = m->mode;
  return VFS_OK;
}

int posix_get_owner(const char *path, uint32_t *uid_out, uint32_t *gid_out) {
  PosixPathMeta *m;
  if (path == 0 || uid_out == 0 || gid_out == 0) {
    return VFS_ERR_INVALID_ARG;
  }
  m = meta_get_or_create(path);
  if (m == 0) {
    return VFS_ERR_NO_SPACE;
  }
  *uid_out = m->uid;
  *gid_out = m->gid;
  return VFS_OK;
}

void posix_track_fd_open(int task_id, int fd, const char *path, int flags) {
  int rights = 0;
  uint32_t mode = 0;
  if (!task_valid(task_id) || fd < 0 || fd >= TASK_MAX_FDS || path == 0) {
    return;
  }

  if ((flags & VFS_O_WRONLY) != 0) {
    rights |= POSIX_ACC_WRITE;
  } else if ((flags & VFS_O_RDWR) == VFS_O_RDWR) {
    rights |= POSIX_ACC_READ | POSIX_ACC_WRITE;
  } else {
    rights |= POSIX_ACC_READ;
  }

  if (posix_get_mode(path, &mode) == VFS_OK && (mode & (POSIX_MODE_XUSR | POSIX_MODE_XGRP | POSIX_MODE_XOTH)) != 0u) {
    rights |= POSIX_ACC_EXEC;
  }

  g_fd_rights[task_id][fd] = (uint8_t)rights;
}

void posix_track_fd_close(int task_id, int fd) {
  if (!task_valid(task_id) || fd < 0 || fd >= TASK_MAX_FDS) {
    return;
  }
  g_fd_rights[task_id][fd] = 0;
}

void posix_clone_fd_rights(int src_task_id, int dst_task_id) {
  if (!task_valid(src_task_id) || !task_valid(dst_task_id)) {
    return;
  }
  for (int i = 0; i < TASK_MAX_FDS; i++) {
    g_fd_rights[dst_task_id][i] = g_fd_rights[src_task_id][i];
  }
}

int posix_check_fd_permission(int task_id, int fd, int access_mask) {
  uint8_t rights;
  if (!task_valid(task_id) || fd < 0 || fd >= TASK_MAX_FDS) {
    return VFS_ERR_INVALID_ARG;
  }

  if (g_id[task_id].euid == 0) {
    return VFS_OK;
  }

  rights = g_fd_rights[task_id][fd];
  if (((uint8_t)access_mask & rights) != (uint8_t)access_mask) {
    return VFS_ERR_PERM;
  }
  return VFS_OK;
}

void posix_clear_task_fds(int task_id) {
  if (!task_valid(task_id)) {
    return;
  }
  for (int i = 0; i < TASK_MAX_FDS; i++) {
    g_fd_rights[task_id][i] = 0;
  }
}

int posix_apply_exec_credentials(int task_id, const char *path) {
  uint32_t mode;
  uint32_t owner_uid;
  uint32_t owner_gid;

  if (!task_valid(task_id) || path == 0 || path[0] == '\0') {
    return VFS_ERR_INVALID_ARG;
  }

  if (posix_get_mode(path, &mode) != VFS_OK ||
      posix_get_owner(path, &owner_uid, &owner_gid) != VFS_OK) {
    return VFS_ERR_NOT_FOUND;
  }

  if (mode & POSIX_MODE_SETUID) {
    g_id[task_id].euid = owner_uid;
  }
  if (mode & POSIX_MODE_SETGID) {
    g_id[task_id].egid = owner_gid;
  }
  return VFS_OK;
}

int posix_lookup_user_name(uint32_t uid, char *out, int out_cap) {
  if (out == 0 || out_cap <= 0) {
    return VFS_ERR_INVALID_ARG;
  }

  for (int i = 0; i < POSIX_DB_USERS_MAX; i++) {
    if (g_users[i].used && g_users[i].uid == uid) {
      name_copy(out, out_cap, g_users[i].name);
      return VFS_OK;
    }
  }

  name_copy(out, out_cap, "unknown");
  return VFS_ERR_NOT_FOUND;
}

int posix_lookup_group_name(uint32_t gid, char *out, int out_cap) {
  if (out == 0 || out_cap <= 0) {
    return VFS_ERR_INVALID_ARG;
  }

  for (int i = 0; i < POSIX_DB_GROUPS_MAX; i++) {
    if (g_groups[i].used && g_groups[i].gid == gid) {
      name_copy(out, out_cap, g_groups[i].name);
      return VFS_OK;
    }
  }

  name_copy(out, out_cap, "unknown");
  return VFS_ERR_NOT_FOUND;
}

int posix_tty_tcsetpgrp(int task_id, int tty_id, int pgid) {
  if (!task_valid(task_id) || tty_id < 0 || tty_id >= POSIX_TTY_MAX) {
    return VFS_ERR_INVALID_ARG;
  }

  if (g_id[task_id].euid != 0 && g_tty_sid[tty_id] != g_id[task_id].sid) {
    return VFS_ERR_PERM;
  }

  for (int t = 0; t < MAX_TASKS; t++) {
    if (os_tasks[t].state != TASK_STATE_TERMINATED && g_id[t].pgid == pgid &&
        g_id[t].sid == g_id[task_id].sid) {
      g_tty_fg_pgrp[tty_id] = pgid;
      g_tty_sid[tty_id] = g_id[task_id].sid;
      return VFS_OK;
    }
  }

  return VFS_ERR_NOT_FOUND;
}

int posix_tty_tcgetpgrp(int tty_id) {
  if (tty_id < 0 || tty_id >= POSIX_TTY_MAX) {
    return VFS_ERR_INVALID_ARG;
  }
  return g_tty_fg_pgrp[tty_id];
}

int posix_task_get_pgid(int task_id) {
  if (!task_valid(task_id)) {
    return -1;
  }
  return g_id[task_id].pgid;
}

int posix_task_set_pgid(int task_id, int pgid) {
  if (!task_valid(task_id) || pgid < 0) {
    return VFS_ERR_INVALID_ARG;
  }
  g_id[task_id].pgid = pgid;
  return VFS_OK;
}

int posix_task_signal_pgid(int sender_task_id, int pgid, int signal) {
  int delivered = 0;
  int sender_sid = -1;

  if (!task_valid(sender_task_id) || pgid < 0 || signal <= 0 || signal >= TASK_SIGNAL_MAX) {
    return VFS_ERR_INVALID_ARG;
  }

  sender_sid = g_id[sender_task_id].sid;
  for (int t = 0; t < MAX_TASKS; t++) {
    if (os_tasks[t].state == TASK_STATE_TERMINATED) {
      continue;
    }
    if (g_id[t].pgid == pgid && (g_id[sender_task_id].euid == 0 || g_id[t].sid == sender_sid)) {
      os_tasks[t].signal_pending |= (1u << signal);
      delivered++;
    }
  }

  return delivered > 0 ? VFS_OK : VFS_ERR_NOT_FOUND;
}

void posix_tty_signal_foreground(int tty_id, int signal) {
  int fg;

  if (tty_id < 0 || tty_id >= POSIX_TTY_MAX || signal <= 0 || signal >= TASK_SIGNAL_MAX) {
    return;
  }

  fg = g_tty_fg_pgrp[tty_id];
  for (int t = 0; t < MAX_TASKS; t++) {
    if (os_tasks[t].state == TASK_STATE_TERMINATED) {
      continue;
    }
    if (g_id[t].pgid == fg) {
      os_tasks[t].signal_pending |= (1u << signal);
    }
  }
}
