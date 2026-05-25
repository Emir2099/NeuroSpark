#ifndef UAPI_NETIO_H
#define UAPI_NETIO_H

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;

#define NS_SYS_SELECT 69
#define NS_SYS_POLL 70
#define NS_SYS_EPOLL 71

#define NS_POLLIN 0x0001u
#define NS_POLLOUT 0x0004u
#define NS_POLLERR 0x0008u

#define NS_EPOLL_OP_CREATE 1
#define NS_EPOLL_OP_CTL 2
#define NS_EPOLL_OP_WAIT 3
#define NS_EPOLL_OP_CLOSE 4

#define NS_EPOLL_CTL_ADD 1
#define NS_EPOLL_CTL_MOD 2
#define NS_EPOLL_CTL_DEL 3

typedef struct {
  int fd;
  uint16_t events;
  uint16_t revents;
} NsPollFd;

typedef struct {
  int op;
  NsPollFd event;
} NsEpollCtlReq;

typedef struct {
  NsPollFd *events;
  uint32_t maxevents;
  uint32_t timeout_ticks;
} NsEpollWaitReq;

static inline int ns_syscall3(int num, int a, int b, int c) {
  int ret;
  __asm__ volatile("int $0x80"
                   : "=a"(ret)
                   : "a"(num), "b"(a), "c"(b), "d"(c)
                   : "memory");
  return ret;
}

static inline int ns_select(NsPollFd *fds, uint32_t count, uint32_t timeout_ticks) {
  return ns_syscall3(NS_SYS_SELECT, (int)fds, (int)count, (int)timeout_ticks);
}

static inline int ns_poll(NsPollFd *fds, uint32_t count, uint32_t timeout_ticks) {
  return ns_syscall3(NS_SYS_POLL, (int)fds, (int)count, (int)timeout_ticks);
}

static inline int ns_epoll_create(void) {
  return ns_syscall3(NS_SYS_EPOLL, NS_EPOLL_OP_CREATE, 0, 0);
}

static inline int ns_epoll_close(int epfd) {
  return ns_syscall3(NS_SYS_EPOLL, NS_EPOLL_OP_CLOSE, epfd, 0);
}

static inline int ns_epoll_ctl(int epfd, int op, int fd, uint16_t events) {
  NsEpollCtlReq req;

  req.op = op;
  req.event.fd = fd;
  req.event.events = events;
  req.event.revents = 0;
  return ns_syscall3(NS_SYS_EPOLL, NS_EPOLL_OP_CTL, epfd, (int)&req);
}

static inline int ns_epoll_wait(int epfd, NsPollFd *events, uint32_t maxevents,
                                uint32_t timeout_ticks) {
  NsEpollWaitReq req;

  req.events = events;
  req.maxevents = maxevents;
  req.timeout_ticks = timeout_ticks;
  return ns_syscall3(NS_SYS_EPOLL, NS_EPOLL_OP_WAIT, epfd, (int)&req);
}

#endif
