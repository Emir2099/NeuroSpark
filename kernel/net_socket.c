#include "net_socket.h"
#include "task.h"
#include "scheduler.h"
#include "net.h"
#include "vfs.h"

#include "net.h"
#include "vfs.h"

extern void task_sleep(uint32_t ticks);
extern uint32_t scheduler_now_ticks(void);

static NsSocketEntry task_sockets[MAX_TASKS][NS_SOCKET_MAX_PER_TASK];
static NsEpollObject task_epolls[MAX_TASKS][NS_EPOLL_MAX_PER_TASK];

void ns_socket_init(void) {
  for (int i = 0; i < MAX_TASKS; i++) {
    for (int j = 0; j < NS_SOCKET_MAX_PER_TASK; j++) {
      task_sockets[i][j].used = 0;
    }
    for (int j = 0; j < NS_EPOLL_MAX_PER_TASK; j++) {
      task_epolls[i][j].used = 0;
    }
  }
}

int ns_socket_fd_to_slot(int fd) {
  int slot = fd - NS_SOCKET_FD_BASE;
  if (slot < 0 || slot >= NS_SOCKET_MAX_PER_TASK) {
    return -1;
  }
  return slot;
}

NsSocketEntry *ns_socket_entry_for_fd(int task_id, int fd) {
  int slot;
  if (task_id < 0 || task_id >= MAX_TASKS) {
    return 0;
  }
  slot = ns_socket_fd_to_slot(fd);
  if (slot < 0 || !task_sockets[task_id][slot].used) {
    return 0;
  }
  return &task_sockets[task_id][slot];
}

int ns_socket_alloc_fd(int task_id, int type, int protocol) {
  if (task_id < 0 || task_id >= MAX_TASKS) {
    return -1;
  }
  for (int i = 0; i < NS_SOCKET_MAX_PER_TASK; i++) {
    if (!task_sockets[task_id][i].used) {
      task_sockets[task_id][i].used = 1;
      task_sockets[task_id][i].type = type;
      task_sockets[task_id][i].protocol = protocol;
      task_sockets[task_id][i].flags = 0;
      task_sockets[task_id][i].bound = 0;
      task_sockets[task_id][i].local_port = 0;
      task_sockets[task_id][i].peer_ip = 0;
      task_sockets[task_id][i].peer_port = 0;
      task_sockets[task_id][i].listening = 0;
      task_sockets[task_id][i].net_conn_id = -1;
      task_sockets[task_id][i].rcv_timeout_ticks = 0xFFFFFFFFu;
      task_sockets[task_id][i].snd_timeout_ticks = 0xFFFFFFFFu;
      task_sockets[task_id][i].reuse_addr = 0;
      task_sockets[task_id][i].keepalive = 0;
      task_sockets[task_id][i].shut_rd = 0;
      task_sockets[task_id][i].shut_wr = 0;
      return NS_SOCKET_FD_BASE + i;
    }
  }
  return -1;
}

void ns_socket_close_entry(NsSocketEntry *s) {
  if (s == 0 || !s->used) {
    return;
  }
  if (s->type == NS_SOCK_DGRAM && s->bound) {
    (void)net_udp_unbind(s->local_port);
  }
  if (s->type == NS_SOCK_STREAM) {
    if (s->listening && s->bound) {
      (void)net_tcp_unlisten(s->local_port);
    } else if (s->net_conn_id >= 0) {
      (void)net_tcp_close(s->net_conn_id);
    }
  }
  s->used = 0;
  s->type = 0;
  s->protocol = 0;
  s->flags = 0;
  s->bound = 0;
  s->local_port = 0;
  s->peer_ip = 0;
  s->peer_port = 0;
  s->listening = 0;
  s->net_conn_id = -1;
  s->rcv_timeout_ticks = 0xFFFFFFFFu;
  s->snd_timeout_ticks = 0xFFFFFFFFu;
  s->reuse_addr = 0;
  s->keepalive = 0;
  s->shut_rd = 0;
  s->shut_wr = 0;
}

int ns_socket_is_nonblocking(const NsSocketEntry *s) {
  return s != 0 && (s->flags & NS_O_NONBLOCK);
}

int ns_socket_ready_for_read(const NsSocketEntry *s) {
  if (s == 0 || !s->used) {
    return 0;
  }
  if (s->shut_rd) {
    return 0;
  }
  if (s->type == NS_SOCK_DGRAM) {
    return s->bound && net_udp_has_data(s->local_port);
  }
  if (s->type == NS_SOCK_STREAM) {
    if (s->listening) {
      return net_tcp_accept_ready(s->local_port);
    }
    return s->net_conn_id >= 0 && net_tcp_can_recv(s->net_conn_id);
  }
  return 0;
}

int ns_socket_ready_for_write(const NsSocketEntry *s) {
  if (s == 0 || !s->used) {
    return 0;
  }
  if (s->shut_wr) {
    return 0;
  }
  if (s->type == NS_SOCK_DGRAM) {
    return s->peer_ip != 0u && s->peer_port != 0u;
  }
  if (s->type == NS_SOCK_STREAM) {
    return s->net_conn_id >= 0 && net_tcp_can_send(s->net_conn_id);
  }
  return 0;
}

int ns_socket_wait_for_readable(const NsSocketEntry *s, uint32_t timeout_ticks) {
  uint32_t deadline = 0;
  int infinite = (timeout_ticks == 0xFFFFFFFFu);

  if (!infinite) {
    deadline = scheduler_now_ticks() + timeout_ticks;
  }

  while (!ns_socket_ready_for_read(s)) {
    if (!infinite && scheduler_now_ticks() >= deadline) {
      return 0;
    }
    task_sleep(1);
  }
  return 1;
}

int ns_socket_wait_for_writable(const NsSocketEntry *s, uint32_t timeout_ticks) {
  uint32_t deadline = 0;
  int infinite = (timeout_ticks == 0xFFFFFFFFu);

  if (!infinite) {
    deadline = scheduler_now_ticks() + timeout_ticks;
  }

  while (!ns_socket_ready_for_write(s)) {
    if (!infinite && scheduler_now_ticks() >= deadline) {
      return 0;
    }
    task_sleep(1);
  }
  return 1;
}

int ns_epoll_fd_to_slot(int fd) {
  int slot = fd - NS_EPOLL_FD_BASE;

  if (slot < 0 || slot >= NS_EPOLL_MAX_PER_TASK) {
    return -1;
  }
  return slot;
}

NsEpollObject *ns_epoll_object_for_fd(int task_id, int fd) {
  int slot;

  if (task_id < 0 || task_id >= MAX_TASKS) {
    return 0;
  }
  slot = ns_epoll_fd_to_slot(fd);
  if (slot < 0 || !task_epolls[task_id][slot].used) {
    return 0;
  }
  return &task_epolls[task_id][slot];
}

int ns_epoll_alloc_fd(int task_id) {
  if (task_id < 0 || task_id >= MAX_TASKS) {
    return -1;
  }
  for (int i = 0; i < NS_EPOLL_MAX_PER_TASK; i++) {
    if (!task_epolls[task_id][i].used) {
      task_epolls[task_id][i].used = 1;
      for (int j = 0; j < NS_EPOLL_MAX_WATCH; j++) {
        task_epolls[task_id][i].watches[j].used = 0;
        task_epolls[task_id][i].watches[j].fd = -1;
        task_epolls[task_id][i].watches[j].events = 0;
      }
      return NS_EPOLL_FD_BASE + i;
    }
  }
  return -1;
}

void ns_epoll_close_object(NsEpollObject *obj) {
  if (obj == 0 || !obj->used) {
    return;
  }
  obj->used = 0;
  for (int i = 0; i < NS_EPOLL_MAX_WATCH; i++) {
    obj->watches[i].used = 0;
    obj->watches[i].fd = -1;
    obj->watches[i].events = 0;
  }
}

void ns_epoll_close_all_for_task(int task_id) {
  if (task_id < 0 || task_id >= MAX_TASKS) {
    return;
  }
  for (int i = 0; i < NS_EPOLL_MAX_PER_TASK; i++) {
    if (task_epolls[task_id][i].used) {
      ns_epoll_close_object(&task_epolls[task_id][i]);
    }
  }
}

int ns_fd_poll_revents(int task_id, int fd, uint16_t events, uint16_t *revents_out) {
  NsSocketEntry *s = ns_socket_entry_for_fd(task_id, fd);
  uint16_t revents = 0;

  if (s != 0) {
    if (ns_socket_ready_for_read(s) && (events & NS_POLLIN) != 0u) {
      revents |= NS_POLLIN;
    }
    if (ns_socket_ready_for_write(s) && (events & NS_POLLOUT) != 0u) {
      revents |= NS_POLLOUT;
    }
    if (revents_out != 0) {
      *revents_out = revents;
    }
    return revents != 0u;
  }

  if (vfs_fcntl(fd, VFS_F_GETFL, 0) >= 0) {
    if ((events & NS_POLLIN) != 0u) {
      revents |= NS_POLLIN;
    }
    if ((events & NS_POLLOUT) != 0u) {
      revents |= NS_POLLOUT;
    }
    if (revents_out != 0) {
      *revents_out = revents;
    }
    return revents != 0u;
  }

  if (revents_out != 0) {
    *revents_out = NS_POLLERR;
  }
  return 0;
}

int ns_poll_socket_array(int task_id, NsPollFd *fds, uint32_t count) {
  int ready = 0;

  for (uint32_t i = 0; i < count; i++) {
    fds[i].revents = 0;
    (void)ns_fd_poll_revents(task_id, fds[i].fd, fds[i].events, &fds[i].revents);
    if (fds[i].revents != 0u) {
      ready++;
    }
  }

  return ready;
}

int ns_poll_socket_array_wait(int task_id, NsPollFd *fds, uint32_t count, uint32_t timeout_ticks) {
  uint32_t deadline = 0;

  if (timeout_ticks != 0u) {
    deadline = scheduler_now_ticks() + timeout_ticks;
  }

  while (1) {
    int ready = ns_poll_socket_array(task_id, fds, count);
    if (ready > 0 || timeout_ticks == 0u) {
      return ready;
    }
    if (scheduler_now_ticks() >= deadline) {
      return 0;
    }
    task_sleep(1);
  }
}

int ns_epoll_ctl_apply(NsEpollObject *obj, const NsEpollCtlReq *req) {
  int free_slot = -1;
  int found_slot = -1;

  if (obj == 0 || req == 0 || req->event.fd < 0) {
    return -22; /* NS_ERR_INVALID_ARG */
  }

  for (int i = 0; i < NS_EPOLL_MAX_WATCH; i++) {
    if (!obj->watches[i].used && free_slot < 0) {
      free_slot = i;
    }
    if (obj->watches[i].used && obj->watches[i].fd == req->event.fd) {
      found_slot = i;
      break;
    }
  }

  if (req->op == NS_EPOLL_CTL_ADD) {
    if (found_slot >= 0) {
      return -22;
    }
    if (free_slot < 0) {
      return -28; /* VFS_ERR_NO_SPACE */
    }
    obj->watches[free_slot].used = 1;
    obj->watches[free_slot].fd = req->event.fd;
    obj->watches[free_slot].events = req->event.events;
    return 0;
  }

  if (req->op == NS_EPOLL_CTL_MOD) {
    if (found_slot < 0) {
      return -22;
    }
    obj->watches[found_slot].events = req->event.events;
    return 0;
  }

  if (req->op == NS_EPOLL_CTL_DEL) {
    if (found_slot < 0) {
      return -22;
    }
    obj->watches[found_slot].used = 0;
    obj->watches[found_slot].fd = -1;
    obj->watches[found_slot].events = 0;
    return 0;
  }

  return -22;
}

int ns_epoll_collect_ready(NsEpollObject *obj, NsPollFd *out, uint32_t maxevents) {
  int ready = 0;

  if (obj == 0 || out == 0 || maxevents == 0u) {
    return 0;
  }

  for (int i = 0; i < NS_EPOLL_MAX_WATCH; i++) {
    uint16_t revents = 0;

    if (!obj->watches[i].used) {
      continue;
    }
    (void)ns_fd_poll_revents(os_current_task, obj->watches[i].fd, obj->watches[i].events, &revents);
    if (revents == 0u) {
      continue;
    }

    out[ready].fd = obj->watches[i].fd;
    out[ready].events = obj->watches[i].events;
    out[ready].revents = revents;
    ready++;
    if ((uint32_t)ready >= maxevents) {
      break;
    }
  }

  return ready;
}

int ns_epoll_wait_ready(NsEpollObject *obj, NsPollFd *out, uint32_t maxevents,
                            uint32_t timeout_ticks) {
  uint32_t deadline = 0;
  int infinite = (timeout_ticks == 0xFFFFFFFFu);

  if (!infinite && timeout_ticks != 0u) {
    deadline = scheduler_now_ticks() + timeout_ticks;
  }

  while (1) {
    int ready = ns_epoll_collect_ready(obj, out, maxevents);
    if (ready > 0 || timeout_ticks == 0u) {
      return ready;
    }
    if (!infinite && scheduler_now_ticks() >= deadline) {
      return 0;
    }
    task_sleep(1);
  }
}

void ns_socket_close_all_for_task(int task_id) {
  if (task_id < 0 || task_id >= MAX_TASKS) {
    return;
  }
  for (int i = 0; i < NS_SOCKET_MAX_PER_TASK; i++) {
    if (task_sockets[task_id][i].used) {
      ns_socket_close_entry(&task_sockets[task_id][i]);
    }
  }
}
