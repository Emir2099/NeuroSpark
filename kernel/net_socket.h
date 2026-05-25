#ifndef NET_SOCKET_H
#define NET_SOCKET_H

#ifndef _UINT32_T_DEFINED
#define _UINT32_T_DEFINED
typedef unsigned int uint32_t;
#endif

#ifndef _UINT16_T_DEFINED
#define _UINT16_T_DEFINED
typedef unsigned short uint16_t;
#endif

#ifndef _UINT8_T_DEFINED
#define _UINT8_T_DEFINED
typedef unsigned char uint8_t;
#endif

#define NS_SOCKET_FD_BASE 1024
#define NS_SOCKET_MAX_PER_TASK 64

#define NS_O_NONBLOCK 0x800

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

#define NS_EPOLL_FD_BASE 2048
#define NS_EPOLL_MAX_PER_TASK 8
#define NS_EPOLL_MAX_WATCH 64

#define NS_AF_INET 2
#define NS_SOCK_STREAM 1
#define NS_SOCK_DGRAM 2

#define NS_SOL_SOCKET 1
#define NS_SO_REUSEADDR 2
#define NS_SO_RCVTIMEO 3
#define NS_SO_SNDTIMEO 4
#define NS_SO_KEEPALIVE 5

#define NS_SHUT_RD 0
#define NS_SHUT_WR 1
#define NS_SHUT_RDWR 2

typedef struct {
  uint16_t sin_family;
  uint16_t sin_port;
  uint32_t sin_addr;
} NsSockAddrIn;

typedef struct {
  int used;
  int type;
  int protocol;
  int flags;
  int bound;
  uint16_t local_port;
  uint32_t peer_ip;
  uint16_t peer_port;
  int listening;
  int net_conn_id;
  uint32_t rcv_timeout_ticks;
  uint32_t snd_timeout_ticks;
  uint8_t reuse_addr;
  uint8_t keepalive;
  uint8_t shut_rd;
  uint8_t shut_wr;
} NsSocketEntry;

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

typedef struct {
  int used;
  int fd;
  uint16_t events;
} NsEpollWatch;

typedef struct {
  int used;
  NsEpollWatch watches[NS_EPOLL_MAX_WATCH];
} NsEpollObject;

/* Initialization */
void ns_socket_init(void);

/* Socket API */
int ns_socket_fd_to_slot(int fd);
NsSocketEntry *ns_socket_entry_for_fd(int task_id, int fd);
int ns_socket_alloc_fd(int task_id, int type, int protocol);
void ns_socket_close_entry(NsSocketEntry *s);
void ns_socket_close_all_for_task(int task_id);
int ns_socket_is_nonblocking(const NsSocketEntry *s);
int ns_socket_ready_for_read(const NsSocketEntry *s);
int ns_socket_ready_for_write(const NsSocketEntry *s);
int ns_socket_wait_for_readable(const NsSocketEntry *s, uint32_t timeout_ticks);
int ns_socket_wait_for_writable(const NsSocketEntry *s, uint32_t timeout_ticks);

/* Epoll API */
int ns_epoll_fd_to_slot(int fd);
NsEpollObject *ns_epoll_object_for_fd(int task_id, int fd);
int ns_epoll_alloc_fd(int task_id);
void ns_epoll_close_object(NsEpollObject *obj);
void ns_epoll_close_all_for_task(int task_id);
int ns_epoll_ctl_apply(NsEpollObject *obj, const NsEpollCtlReq *req);
int ns_epoll_collect_ready(NsEpollObject *obj, NsPollFd *out, uint32_t maxevents);
int ns_epoll_wait_ready(NsEpollObject *obj, NsPollFd *out, uint32_t maxevents, uint32_t timeout_ticks);

/* Poll API */
int ns_fd_poll_revents(int task_id, int fd, uint16_t events, uint16_t *revents_out);
int ns_poll_socket_array(int task_id, NsPollFd *fds, uint32_t count);
int ns_poll_socket_array_wait(int task_id, NsPollFd *fds, uint32_t count, uint32_t timeout_ticks);

#endif
