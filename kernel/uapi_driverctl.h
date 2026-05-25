#ifndef UAPI_DRIVERCTL_H
#define UAPI_DRIVERCTL_H

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

enum {
  NS_SYS_IOCTL = 59,
};

enum {
  NS_IOCTL_DISK_GET_GEOMETRY = 0x1001,
  NS_IOCTL_DISK_GET_HEALTH = 0x1002,
  NS_IOCTL_DISK_SET_BACKEND = 0x1003,
  NS_IOCTL_DISK_GET_BACKEND = 0x1004,
  NS_IOCTL_DISK_RESET_IO_STATS = 0x1005,
  NS_IOCTL_DISK_GET_IO_STATS = 0x1006,
  NS_IOCTL_NIC_GET_INFO = 0x2001,
  NS_IOCTL_NIC_SET_MAC = 0x2002,
};

typedef struct {
  uint32_t total_sectors_low;
  uint32_t total_sectors_high;
  uint32_t sector_size;
  uint32_t backend;
  char model[41];
} NsDiskGeometry;

typedef struct {
  uint32_t available;
  uint32_t backend;
  uint32_t preferred_backend;
  uint32_t ahci_ready_ports;
  uint32_t smart_supported;
  uint32_t smart_enabled;
  uint32_t temperature_c;
  uint32_t life_percent;
  uint32_t read_error_count;
  uint32_t write_error_count;
  uint32_t last_error_code;
} NsDiskHealth;

typedef struct {
  uint32_t uncached_reads;
  uint32_t uncached_writes;
  uint32_t cached_reads;
  uint32_t cached_writes;
} NsDiskIoStats;

typedef struct {
  uint32_t ready;
  uint32_t nic_index;
  uint32_t io_base;
  uint32_t link_mbps;
  uint8_t mac[6];
  char driver[16];
} NsNicInfo;

static inline int ns_syscall3(int num, int a, int b, int c) {
  int ret;
  __asm__ volatile("int $0x80"
                   : "=a"(ret)
                   : "a"(num), "b"(a), "c"(b), "d"(c)
                   : "memory");
  return ret;
}

static inline int ns_ioctl(int fd, int cmd, void *arg) {
  return ns_syscall3(NS_SYS_IOCTL, fd, cmd, (int)arg);
}

#endif
