#include "net.h"
#include "pci.h"
#include "profiling.h"
#include "storage_manager.h"

typedef unsigned short uint16_t;

#define RTL8139_VENDOR 0x10EC
#define RTL8139_DEVICE 0x8139

#define RTL_REG_IDR0 0x00
#define RTL_REG_TSD0 0x10
#define RTL_REG_TSAD0 0x20
#define RTL_REG_RBSTART 0x30
#define RTL_REG_CR 0x37
#define RTL_REG_CAPR 0x38
#define RTL_REG_CBR 0x3A
#define RTL_REG_IMR 0x3C
#define RTL_REG_ISR 0x3E
#define RTL_REG_RCR 0x44
#define RTL_REG_CONFIG1 0x52
#define RTL_REG_MSR 0x58

#define RTL_MSR_SPEED_10 0x08u

#define RTL_CMD_RESET 0x10
#define RTL_CMD_RX_ENABLE 0x08
#define RTL_CMD_TX_ENABLE 0x04

#define RTL_RCR_AB 0x00000008
#define RTL_RCR_AM 0x00000004
#define RTL_RCR_APM 0x00000002
#define RTL_RCR_AAP 0x00000001

#define RTL_INT_RX_OK 0x0001u
#define RTL_INT_RX_ERR 0x0002u
#define RTL_INT_TX_OK 0x0004u
#define RTL_INT_TX_ERR 0x0008u
#define RTL_INT_RX_OVW 0x0010u
#define RTL_INT_MASK (RTL_INT_RX_OK | RTL_INT_RX_ERR | RTL_INT_TX_OK | RTL_INT_TX_ERR | RTL_INT_RX_OVW)

#define ETH_TYPE_NEURO 0x88B5
#define ETH_TYPE_IPV4 0x0800
#define ETH_TYPE_ARP 0x0806
#define IP_PROTO_ICMP 1u
#define IP_PROTO_UDP 17u
#define IP_PROTO_TCP 6u

#define ICMP_TYPE_ECHO_REPLY 0u
#define ICMP_TYPE_ECHO_REQUEST 8u

#define ARP_HTYPE_ETHERNET 1u
#define ARP_PTYPE_IPV4 0x0800u
#define ARP_OPER_REQUEST 1u
#define ARP_OPER_REPLY 2u

#define RTL_RX_BUFFER_SIZE 8192u
#define RTL_RX_BUFFER_MASK (RTL_RX_BUFFER_SIZE - 1u)

#define UDP_CTRL_PORT 39000
#define UDP_DATA_PORT 39001
#define UDP_MAX_BINDINGS 16
#define UDP_QUEUE_DEPTH 16
#define UDP_MAX_PAYLOAD 1200

#define TCP_MAX_CONNS 32
#define TCP_QUEUE_DEPTH 16
#define TCP_MAX_PAYLOAD 1200
#define TCP_RETX_TICKS 20u
#define TCP_RETX_MAX 5u

#define IPV4_ETH_MTU 1500u
#define IPV4_MAX_HEADER 60u
#define IPV4_REASS_MAX_PAYLOAD 4096u
#define IPV4_REASS_SLOT_MAX 4
#define IPV4_REASS_TIMEOUT_TICKS 300u

#define ROUTE_TABLE_MAX 8

#define TCP_FLAG_FIN 0x01u
#define TCP_FLAG_SYN 0x02u
#define TCP_FLAG_RST 0x04u
#define TCP_FLAG_PSH 0x08u
#define TCP_FLAG_ACK 0x10u

#define REMOTE_PKT_MAGIC 0x52435031u /* RCP1 */
#define REMOTE_PKT_VERSION 1u
#define REMOTE_PKT_TYPE_STATUS 1
#define REMOTE_PKT_TYPE_COMMAND 2
#define REMOTE_PKT_TYPE_SNAPSHOT 3
#define REMOTE_PKT_TYPE_PROFILE 4
#define REMOTE_PKT_TYPE_MANIFEST 5

#define REMOTE_PKT_FLAG_RELIABLE 0x00000001u
#define REMOTE_PKT_FLAG_CHUNKED 0x00000002u
#define REMOTE_PKT_FLAG_CHUNK_FIRST 0x00000004u
#define REMOTE_PKT_FLAG_CHUNK_LAST 0x00000008u

#define REMOTE_CHUNK_MAGIC 0x43484B31u /* CHK1 */

#define REMOTE_STATUS_OK 0u
#define REMOTE_STATUS_AUTH_REQUIRED 1u
#define REMOTE_STATUS_TOKEN_EXPIRED 2u
#define REMOTE_STATUS_INVALID_TOKEN 3u

#define REMOTE_ROLE_VIEWER 1u
#define REMOTE_ROLE_OPERATOR 2u
#define REMOTE_ROLE_ADMIN 3u

typedef struct {
  uint32_t magic;
  uint32_t version;
  uint32_t type;
  uint32_t flags;
  uint32_t session;
  uint32_t request_id;
  uint32_t token;
  uint32_t nonce;
  uint32_t role;
  uint32_t checksum;
  uint32_t status;
  uint32_t payload_len;
} __attribute__((packed)) RemotePacketHeader;

typedef struct {
  uint32_t magic;
  uint32_t transfer_id;
  uint32_t chunk_index;
  uint32_t chunk_count;
  uint32_t total_len;
  uint32_t chunk_offset;
  uint32_t chunk_len;
  uint32_t checksum;
} __attribute__((packed)) RemoteChunkHeader;

typedef struct {
  uint32_t src_ip;
  uint16_t src_port;
  uint16_t len;
  uint8_t data[UDP_MAX_PAYLOAD];
} UdpPacket;

typedef struct {
  int used;
  uint16_t port;
  uint16_t head;
  uint16_t tail;
  uint16_t count;
  uint32_t dropped;
  UdpPacket queue[UDP_QUEUE_DEPTH];
} UdpBinding;

typedef struct {
  uint16_t len;
  uint8_t data[TCP_MAX_PAYLOAD];
} TcpSegment;

typedef struct {
  int used;
  uint32_t network;
  uint32_t mask;
  uint32_t gateway;
  uint8_t prefix_len;
  uint8_t metric;
} RouteEntry;

typedef struct {
  int used;
  uint32_t src_ip;
  uint32_t dst_ip;
  uint16_t ident;
  uint8_t proto;
  uint8_t header_len;
  uint8_t have_first;
  uint8_t total_known;
  uint16_t total_payload_len;
  uint32_t last_tick;
  uint8_t header[IPV4_MAX_HEADER];
  uint8_t payload[IPV4_REASS_MAX_PAYLOAD];
  uint8_t present[(IPV4_REASS_MAX_PAYLOAD / 8u + 7u) / 8u];
} Ipv4ReassSlot;

typedef struct {
  int used;
  int listening;
  int parent_listen;
  int accepted;
  int state;
  uint16_t local_port;
  uint16_t remote_port;
  uint32_t remote_ip;
  uint32_t snd_una;
  uint32_t snd_nxt;
  uint32_t rcv_nxt;
  uint16_t rx_head;
  uint16_t rx_tail;
  uint16_t rx_count;
  uint32_t last_tx_tick;
  uint32_t last_tx_seq;
  uint32_t last_tx_ack;
  uint16_t last_tx_len;
  uint8_t last_tx_flags;
  uint8_t last_tx_retries;
  uint8_t waiting_ack;
  uint8_t last_tx_data[TCP_MAX_PAYLOAD];
  TcpSegment rx_queue[TCP_QUEUE_DEPTH];
} TcpConn;

enum {
  TCP_STATE_CLOSED = 0,
  TCP_STATE_LISTEN = 1,
  TCP_STATE_SYN_SENT = 2,
  TCP_STATE_SYN_RCVD = 3,
  TCP_STATE_ESTABLISHED = 4,
  TCP_STATE_CLOSE_WAIT = 5,
  TCP_STATE_CLOSING = 6
};

extern volatile int ata_disk_available;
extern uint32_t tick;

static int net_ready = 0;
static int net_idx = -1;
static uint32_t rtl_iobase = 0;
static uint8_t rtl_mac[6] = {0, 0, 0, 0, 0, 0};

static uint32_t net_ip = 0x0A00020Fu;   /* 10.0.2.15 */
static uint32_t net_mask = 0xFFFFFF00u; /* /24 */
static uint32_t net_gw = 0x0A000201u;   /* 10.0.2.1 */

static int remote_enabled = 0;
static uint32_t remote_token = 0;
static uint32_t remote_session = 0;
static uint32_t remote_seq = 1;
static uint32_t remote_auth_deadline = 0;
static uint32_t remote_dst_ip = 0xFFFFFFFFu;
static uint32_t remote_role = REMOTE_ROLE_OPERATOR;
static uint32_t remote_key_epoch = 1u;
static uint32_t remote_nonce_state = 0x9E3779B9u;
static uint32_t net_fault_loss_percent = 0u;
static uint32_t net_fault_reorder_percent = 0u;
static uint32_t net_fault_prng_state = 0xC0FFEE11u;

typedef struct {
  int valid;
  uint32_t src_ip;
  uint16_t src_port;
  uint16_t dst_port;
  uint16_t len;
  uint8_t data[UDP_MAX_PAYLOAD];
} UdpReorderHold;

static UdpReorderHold udp_reorder_hold = {0};

static uint8_t rtl_rx_buffer[8192 + 16 + 1500] __attribute__((aligned(16)));
static uint8_t rtl_tx_buffer[4][1600] __attribute__((aligned(16)));
static int rtl_tx_slot = 0;
static uint32_t rtl_rx_offset = 0;
static uint32_t net_rx_poll_budget_cfg = 32u;
static uint32_t net_irq_poll_budget_cfg = 96u;
static NetRxStats net_rx_stats = {0};
static RouteEntry route_table[ROUTE_TABLE_MAX];
static int route_count = 0;
static Ipv4ReassSlot ipv4_reass[IPV4_REASS_SLOT_MAX];
static uint16_t ipv4_tx_ident = 1u;
static uint32_t arp_cache_ip = 0;
static uint8_t arp_cache_mac[6] = {0, 0, 0, 0, 0, 0};
static int arp_cache_valid = 0;
static UdpBinding udp_bindings[UDP_MAX_BINDINGS];
static TcpConn tcp_conns[TCP_MAX_CONNS];
static uint32_t tcp_iss_seed = 0x10203040u;

static int rtl8139_send_frame(const uint8_t *frame, uint32_t len);
static uint16_t internet_checksum(const uint8_t *data, uint32_t len);
static int tcp_send_segment(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port,
                            uint32_t seq, uint32_t ack, uint8_t flags,
                            const uint8_t *payload, uint16_t payload_len);
static int net_rx_poll_budget(uint32_t max_packets);
static int route_table_set(uint32_t network, uint32_t mask, uint32_t gateway,
                           uint8_t metric);
static void route_table_clear(void);
static void route_table_refresh_defaults(void);
static int route_lookup(uint32_t dst_ip, uint32_t *next_hop);
static int net_send_ipv4_raw(uint32_t dst_ip, uint8_t protocol,
                             const uint8_t *payload, uint16_t payload_len,
                             uint8_t ttl, uint16_t ident);
static int net_forward_ipv4_packet(const uint8_t *ip_packet, uint16_t total_len);
static int ipv4_reassemble_packet(uint32_t src_ip, uint32_t dst_ip, uint8_t proto,
                                  uint16_t ident, uint16_t frag_off_flags,
                                  const uint8_t *ip_header, uint8_t header_len,
                                  const uint8_t *payload, uint16_t payload_len,
                                  uint8_t *out_packet, uint16_t *out_total_len);
static void ipv4_reassembly_gc(void);

static int prefix_len_from_mask(uint32_t mask) {
  int bits = 0;
  while ((mask & 0x80000000u) != 0u) {
    bits++;
    mask <<= 1;
  }
  return bits;
}

static void route_table_clear(void) {
  route_count = 0;
  for (int i = 0; i < ROUTE_TABLE_MAX; i++) {
    route_table[i].used = 0;
    route_table[i].network = 0;
    route_table[i].mask = 0;
    route_table[i].gateway = 0;
    route_table[i].prefix_len = 0;
    route_table[i].metric = 0;
  }
}

static int route_table_set(uint32_t network, uint32_t mask, uint32_t gateway,
                           uint8_t metric) {
  int idx = -1;

  if (mask != 0u) {
    network &= mask;
  } else {
    network = 0u;
  }

  for (int i = 0; i < ROUTE_TABLE_MAX; i++) {
    if (route_table[i].used && route_table[i].network == network &&
        route_table[i].mask == mask) {
      idx = i;
      break;
    }
  }

  if (idx < 0) {
    for (int i = 0; i < ROUTE_TABLE_MAX; i++) {
      if (!route_table[i].used) {
        idx = i;
        route_table[i].used = 1;
        route_count++;
        break;
      }
    }
  }

  if (idx < 0) {
    return 0;
  }

  route_table[idx].network = network;
  route_table[idx].mask = mask;
  route_table[idx].gateway = gateway;
  route_table[idx].prefix_len = (uint8_t)prefix_len_from_mask(mask);
  route_table[idx].metric = metric;
  return 1;
}

static void route_table_refresh_defaults(void) {
  route_table_clear();
  (void)route_table_set(net_ip & net_mask, net_mask, 0u, 10u);
  if (net_gw != 0u) {
    (void)route_table_set(0u, 0u, net_gw, 100u);
  }
}

static int route_lookup(uint32_t dst_ip, uint32_t *next_hop) {
  int best_idx = -1;
  int best_prefix = -1;
  uint8_t best_metric = 0xFFu;

  for (int i = 0; i < ROUTE_TABLE_MAX; i++) {
    if (!route_table[i].used) {
      continue;
    }
    if ((dst_ip & route_table[i].mask) != route_table[i].network) {
      continue;
    }
    if ((int)route_table[i].prefix_len > best_prefix ||
        ((int)route_table[i].prefix_len == best_prefix &&
         route_table[i].metric < best_metric)) {
      best_idx = i;
      best_prefix = (int)route_table[i].prefix_len;
      best_metric = route_table[i].metric;
    }
  }

  if (best_idx < 0) {
    return 0;
  }

  if (next_hop != 0) {
    uint32_t gw = route_table[best_idx].gateway;
    *next_hop = (gw != 0u) ? gw : dst_ip;
  }
  return 1;
}

static uint32_t net_route_next_hop(uint32_t dst_ip) {
  uint32_t next_hop = dst_ip;
  if (dst_ip == 0u || dst_ip == 0xFFFFFFFFu) {
    return dst_ip;
  }
  if (route_lookup(dst_ip, &next_hop)) {
    return next_hop;
  }
  return dst_ip;
}

static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val) {
  __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outl(uint16_t port, uint32_t val) {
  __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static inline uint32_t inl(uint16_t port) {
  uint32_t ret;
  __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static inline uint16_t inw(uint16_t port) {
  uint16_t ret;
  __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static uint32_t be32(uint32_t v) {
  return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8) |
         ((v & 0x00FF0000u) >> 8) | ((v & 0xFF000000u) >> 24);
}

static void mem_copy(uint8_t *dst, const uint8_t *src, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    dst[i] = src[i];
  }
}

static void mem_set(uint8_t *dst, uint8_t value, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    dst[i] = value;
  }
}

static uint16_t read_be16(const uint8_t *src) {
  return (uint16_t)(((uint16_t)src[0] << 8) | (uint16_t)src[1]);
}

static uint16_t read_le16(const uint8_t *src) {
  return (uint16_t)(((uint16_t)src[1] << 8) | (uint16_t)src[0]);
}

static uint32_t read_be32(const uint8_t *src) {
  return ((uint32_t)src[0] << 24) | ((uint32_t)src[1] << 16) |
         ((uint32_t)src[2] << 8) | (uint32_t)src[3];
}

static void write_be16(uint8_t *dst, uint16_t value) {
  dst[0] = (uint8_t)(value >> 8);
  dst[1] = (uint8_t)(value & 0xFFu);
}

static void write_be32(uint8_t *dst, uint32_t value) {
  dst[0] = (uint8_t)((value >> 24) & 0xFFu);
  dst[1] = (uint8_t)((value >> 16) & 0xFFu);
  dst[2] = (uint8_t)((value >> 8) & 0xFFu);
  dst[3] = (uint8_t)(value & 0xFFu);
}

static void mac_copy(uint8_t *dst, const uint8_t *src) {
  for (int i = 0; i < 6; i++) {
    dst[i] = src[i];
  }
}

static void copy_ring_bytes(uint32_t offset, uint8_t *dst, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    dst[i] = rtl_rx_buffer[(offset + i) & RTL_RX_BUFFER_MASK];
  }
}

static void net_count_ethertype(uint16_t ethertype) {
  net_rx_stats.total_frames++;
  if (ethertype == ETH_TYPE_IPV4) {
    net_rx_stats.ipv4_frames++;
  } else if (ethertype == ETH_TYPE_ARP) {
    net_rx_stats.arp_frames++;
  } else if (ethertype == ETH_TYPE_NEURO) {
    net_rx_stats.neuro_frames++;
  } else {
    net_rx_stats.unknown_frames++;
  }
}

static void arp_cache_update(uint32_t ip, const uint8_t mac[6]) {
  arp_cache_ip = ip;
  mac_copy(arp_cache_mac, mac);
  arp_cache_valid = 1;
}

static int arp_cache_lookup(uint32_t ip, uint8_t out_mac[6]) {
  if (!arp_cache_valid || arp_cache_ip != ip || out_mac == 0) {
    return 0;
  }
  mac_copy(out_mac, arp_cache_mac);
  return 1;
}

static int net_send_arp_packet(uint16_t op, const uint8_t dst_mac[6], uint32_t sender_ip,
                               uint32_t target_ip, const uint8_t target_mac[6]) {
  uint8_t frame[42];

  if (!net_ready || dst_mac == 0) {
    return 0;
  }

  mac_copy(&frame[0], dst_mac);
  mac_copy(&frame[6], rtl_mac);
  write_be16(&frame[12], ETH_TYPE_ARP);
  write_be16(&frame[14], ARP_HTYPE_ETHERNET);
  write_be16(&frame[16], ARP_PTYPE_IPV4);
  frame[18] = 6;
  frame[19] = 4;
  write_be16(&frame[20], op);
  mac_copy(&frame[22], rtl_mac);
  write_be32(&frame[28], sender_ip);
  if (target_mac != 0) {
    mac_copy(&frame[32], target_mac);
  } else {
    mem_set(&frame[32], 0, 6);
  }
  write_be32(&frame[38], target_ip);

  return rtl8139_send_frame(frame, 42);
}

static int net_send_arp_request(uint32_t target_ip) {
  uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  return net_send_arp_packet(ARP_OPER_REQUEST, broadcast, net_ip, target_ip, 0);
}

static int net_send_arp_reply(const uint8_t requester_mac[6], uint32_t requester_ip) {
  return net_send_arp_packet(ARP_OPER_REPLY, requester_mac, net_ip, requester_ip, requester_mac);
}

static int tcp_alloc_conn(void) {
  for (int i = 0; i < TCP_MAX_CONNS; i++) {
    if (!tcp_conns[i].used) {
      tcp_conns[i].used = 1;
      tcp_conns[i].listening = 0;
      tcp_conns[i].parent_listen = -1;
      tcp_conns[i].accepted = 0;
      tcp_conns[i].state = TCP_STATE_CLOSED;
      tcp_conns[i].local_port = 0;
      tcp_conns[i].remote_port = 0;
      tcp_conns[i].remote_ip = 0;
      tcp_conns[i].snd_una = 0;
      tcp_conns[i].snd_nxt = 0;
      tcp_conns[i].rcv_nxt = 0;
      tcp_conns[i].rx_head = 0;
      tcp_conns[i].rx_tail = 0;
      tcp_conns[i].rx_count = 0;
      tcp_conns[i].last_tx_tick = 0;
      tcp_conns[i].last_tx_seq = 0;
      tcp_conns[i].last_tx_ack = 0;
      tcp_conns[i].last_tx_len = 0;
      tcp_conns[i].last_tx_flags = 0;
      tcp_conns[i].last_tx_retries = 0;
      tcp_conns[i].waiting_ack = 0;
      return i;
    }
  }
  return -1;
}

static void tcp_track_outbound(TcpConn *c, uint32_t seq, uint32_t ack,
                               uint8_t flags, const uint8_t *payload,
                               uint16_t payload_len) {
  if (c == 0) {
    return;
  }

  c->last_tx_tick = tick;
  c->last_tx_seq = seq;
  c->last_tx_ack = ack;
  c->last_tx_flags = flags;
  c->last_tx_len = payload_len;
  c->last_tx_retries = 0;
  c->waiting_ack = (uint8_t)(((flags & TCP_FLAG_SYN) != 0u) ||
                             ((flags & TCP_FLAG_FIN) != 0u) ||
                             (payload_len > 0u));

  if (payload_len > 0u && payload != 0) {
    mem_copy(c->last_tx_data, payload, payload_len);
  }
}

static int tcp_send_and_track(TcpConn *c, uint8_t flags, const uint8_t *payload,
                              uint16_t payload_len) {
  if (c == 0) {
    return 0;
  }
  if (!tcp_send_segment(c->remote_ip, c->local_port, c->remote_port,
                        c->snd_nxt, c->rcv_nxt, flags, payload, payload_len)) {
    return 0;
  }
  tcp_track_outbound(c, c->snd_nxt, c->rcv_nxt, flags, payload, payload_len);
  if ((flags & TCP_FLAG_SYN) != 0u || (flags & TCP_FLAG_FIN) != 0u) {
    c->snd_nxt++;
  }
  c->snd_nxt += payload_len;
  return 1;
}

static int tcp_retransmit_last(TcpConn *c) {
  if (c == 0 || !c->used || c->listening || !c->waiting_ack) {
    return 0;
  }
  if (c->last_tx_retries >= TCP_RETX_MAX) {
    net_rx_stats.tcp_retransmit_failures++;
    if (c->state == TCP_STATE_SYN_SENT || c->state == TCP_STATE_SYN_RCVD ||
        c->state == TCP_STATE_CLOSING) {
      net_rx_stats.tcp_half_open_cleanups++;
    }
    c->used = 0;
    c->state = TCP_STATE_CLOSED;
    return 0;
  }
  if (!tcp_send_segment(c->remote_ip, c->local_port, c->remote_port,
                        c->last_tx_seq, c->last_tx_ack,
                        c->last_tx_flags,
                        (c->last_tx_len > 0u) ? c->last_tx_data : 0,
                        c->last_tx_len)) {
    return 0;
  }
  c->last_tx_tick = tick;
  c->last_tx_retries++;
  net_rx_stats.tcp_retransmits++;
  return 1;
}

static int tcp_find_listener(uint16_t port) {
  for (int i = 0; i < TCP_MAX_CONNS; i++) {
    if (tcp_conns[i].used && tcp_conns[i].listening && tcp_conns[i].local_port == port) {
      return i;
    }
  }
  return -1;
}

static int tcp_find_conn(uint32_t remote_ip, uint16_t remote_port, uint16_t local_port) {
  for (int i = 0; i < TCP_MAX_CONNS; i++) {
    if (!tcp_conns[i].used || tcp_conns[i].listening) {
      continue;
    }
    if (tcp_conns[i].remote_ip == remote_ip && tcp_conns[i].remote_port == remote_port &&
        tcp_conns[i].local_port == local_port && tcp_conns[i].state != TCP_STATE_CLOSED) {
      return i;
    }
  }
  return -1;
}

static int tcp_send_segment(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port,
                            uint32_t seq, uint32_t ack, uint8_t flags,
                            const uint8_t *payload, uint16_t payload_len) {
  uint8_t frame[1514];
  uint8_t dst_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint32_t arp_target_ip = net_route_next_hop(dst_ip);
  uint16_t ip_len;
  uint16_t tcp_len;
  uint16_t frame_len;
  uint16_t csum;
  uint8_t pseudo[1600];
  uint16_t pseudo_len;

  if (!net_ready || payload_len > 1300u) {
    return 0;
  }

  if (dst_ip != 0xFFFFFFFFu) {
    if (!arp_cache_lookup(arp_target_ip, dst_mac)) {
      (void)net_send_arp_request(arp_target_ip);
    }
  }

  mem_set(frame, 0, sizeof(frame));
  for (int i = 0; i < 6; i++) {
    frame[i] = dst_mac[i];
    frame[6 + i] = rtl_mac[i];
  }
  write_be16(&frame[12], ETH_TYPE_IPV4);

  tcp_len = (uint16_t)(20u + payload_len);
  ip_len = (uint16_t)(20u + tcp_len);

  frame[14] = 0x45;
  frame[15] = 0x00;
  write_be16(&frame[16], ip_len);
  write_be16(&frame[18], (uint16_t)(tcp_iss_seed & 0xFFFFu));
  write_be16(&frame[20], 0x0000u);
  frame[22] = 64;
  frame[23] = (uint8_t)IP_PROTO_TCP;
  write_be16(&frame[24], 0x0000u);
  write_be32(&frame[26], net_ip);
  write_be32(&frame[30], dst_ip);
  csum = internet_checksum(&frame[14], 20u);
  write_be16(&frame[24], csum);

  write_be16(&frame[34], src_port);
  write_be16(&frame[36], dst_port);
  write_be32(&frame[38], seq);
  write_be32(&frame[42], ack);
  frame[46] = (uint8_t)(5u << 4);
  frame[47] = flags;
  write_be16(&frame[48], 4096u);
  write_be16(&frame[50], 0u);
  write_be16(&frame[52], 0u);
  if (payload_len > 0 && payload != 0) {
    mem_copy(&frame[54], payload, payload_len);
  }

  write_be32(&pseudo[0], net_ip);
  write_be32(&pseudo[4], dst_ip);
  pseudo[8] = 0;
  pseudo[9] = IP_PROTO_TCP;
  write_be16(&pseudo[10], tcp_len);
  mem_copy(&pseudo[12], &frame[34], tcp_len);
  pseudo_len = (uint16_t)(12u + tcp_len);
  csum = internet_checksum(pseudo, pseudo_len);
  write_be16(&frame[50], csum);

  frame_len = (uint16_t)(14u + ip_len);
  if (frame_len < 60u) {
    frame_len = 60u;
  }
  return rtl8139_send_frame(frame, frame_len);
}

static int tcp_queue_rx(TcpConn *c, const uint8_t *payload, uint16_t len) {
  TcpSegment *seg;
  if (c == 0 || payload == 0 || len == 0) {
    return 1;
  }
  if (len > TCP_MAX_PAYLOAD || c->rx_count >= TCP_QUEUE_DEPTH) {
    return 0;
  }
  seg = &c->rx_queue[c->rx_tail];
  seg->len = len;
  mem_copy(seg->data, payload, len);
  c->rx_tail = (uint16_t)((c->rx_tail + 1u) % TCP_QUEUE_DEPTH);
  c->rx_count++;
  return 1;
}

static uint32_t net_fault_prng_next(void) {
  net_fault_prng_state = net_fault_prng_state * 1664525u + 1013904223u;
  return net_fault_prng_state;
}

static int net_fault_should_apply(uint32_t percent) {
  if (percent == 0u) {
    return 0;
  }
  if (percent >= 100u) {
    return 1;
  }
  return (net_fault_prng_next() % 100u) < percent;
}

static void net_count_malformed(void) {
  net_rx_stats.malformed_frames++;
  net_rx_stats.quarantined_frames++;
  net_rx_stats.dropped_frames++;
}

static int tcp_checksum_valid(uint32_t src_ip, uint32_t dst_ip,
                              const uint8_t *tcp, uint16_t tcp_len) {
  uint8_t pseudo[1600];
  uint16_t pseudo_len;

  if (tcp == 0 || tcp_len < 20u) {
    return 0;
  }
  pseudo_len = (uint16_t)(12u + tcp_len);
  if (pseudo_len > sizeof(pseudo)) {
    return 0;
  }

  write_be32(&pseudo[0], src_ip);
  write_be32(&pseudo[4], dst_ip);
  pseudo[8] = 0;
  pseudo[9] = IP_PROTO_TCP;
  write_be16(&pseudo[10], tcp_len);
  mem_copy(&pseudo[12], tcp, tcp_len);
  return internet_checksum(pseudo, pseudo_len) == 0u;
}

static int net_handle_tcp_segment(uint32_t src_ip, uint32_t dst_ip,
                                  const uint8_t *tcp, uint16_t tcp_len) {
  uint16_t src_port;
  uint16_t dst_port;
  uint32_t seq;
  uint32_t ack;
  uint8_t data_off;
  uint16_t hdr_len;
  uint8_t flags;
  uint16_t seg_payload_len;
  int conn_idx;
  int listener_idx;
  TcpConn *c;

  (void)dst_ip;

  if (tcp == 0 || tcp_len < 20u) {
    return 0;
  }

  if (!tcp_checksum_valid(src_ip, dst_ip, tcp, tcp_len)) {
    net_count_malformed();
    return 0;
  }

  src_port = read_be16(&tcp[0]);
  dst_port = read_be16(&tcp[2]);
  seq = read_be32(&tcp[4]);
  ack = read_be32(&tcp[8]);
  data_off = (uint8_t)(tcp[12] >> 4);
  hdr_len = (uint16_t)(data_off * 4u);
  flags = tcp[13];
  if (hdr_len < 20u || hdr_len > tcp_len) {
    return 0;
  }
  seg_payload_len = (uint16_t)(tcp_len - hdr_len);

  conn_idx = tcp_find_conn(src_ip, src_port, dst_port);

  if (conn_idx < 0) {
    listener_idx = tcp_find_listener(dst_port);
    if (listener_idx >= 0 && (flags & TCP_FLAG_SYN) && !(flags & TCP_FLAG_ACK)) {
      int child = tcp_alloc_conn();
      if (child < 0) {
        return 0;
      }
      c = &tcp_conns[child];
      c->listening = 0;
      c->parent_listen = listener_idx;
      c->accepted = 0;
      c->state = TCP_STATE_SYN_RCVD;
      c->local_port = dst_port;
      c->remote_port = src_port;
      c->remote_ip = src_ip;
      c->rcv_nxt = seq + 1u;
      c->snd_una = tcp_iss_seed;
      c->snd_nxt = tcp_iss_seed + 1u;
      tcp_iss_seed += 0x101u;
      if (!tcp_send_segment(c->remote_ip, c->local_port, c->remote_port,
                            c->snd_una, c->rcv_nxt, TCP_FLAG_SYN | TCP_FLAG_ACK, 0, 0)) {
        c->used = 0;
        return 0;
      }
      c->last_tx_tick = tick;
      c->last_tx_seq = c->snd_una;
      c->last_tx_ack = c->rcv_nxt;
      c->last_tx_flags = TCP_FLAG_SYN | TCP_FLAG_ACK;
      c->last_tx_len = 0;
      c->last_tx_retries = 0;
      c->waiting_ack = 1;
      return 1;
    }
    return 0;
  }

  c = &tcp_conns[conn_idx];

  if ((flags & TCP_FLAG_RST) != 0u) {
    net_rx_stats.tcp_reset_events++;
    c->state = TCP_STATE_CLOSED;
    c->used = 0;
    return 1;
  }

  if (c->state == TCP_STATE_CLOSING) {
    if ((flags & TCP_FLAG_ACK) != 0u && ack >= c->snd_nxt) {
      c->snd_una = ack;
      c->waiting_ack = 0;
      c->state = TCP_STATE_CLOSED;
      c->used = 0;
      return 1;
    }
    if ((flags & TCP_FLAG_FIN) != 0u) {
      if (seq == c->rcv_nxt) {
        c->rcv_nxt++;
      }
      (void)tcp_send_segment(c->remote_ip, c->local_port, c->remote_port,
                             c->snd_nxt, c->rcv_nxt, TCP_FLAG_ACK, 0, 0);
      if ((flags & TCP_FLAG_ACK) != 0u && ack >= c->snd_nxt) {
        c->snd_una = ack;
        c->waiting_ack = 0;
        c->state = TCP_STATE_CLOSED;
        c->used = 0;
      }
      return 1;
    }
  }

  if (c->state == TCP_STATE_SYN_SENT) {
    if ((flags & TCP_FLAG_SYN) && (flags & TCP_FLAG_ACK) && ack == c->snd_nxt) {
      c->rcv_nxt = seq + 1u;
      c->snd_una = ack;
      c->state = TCP_STATE_ESTABLISHED;
      c->waiting_ack = 0;
      (void)tcp_send_segment(c->remote_ip, c->local_port, c->remote_port,
                             c->snd_nxt, c->rcv_nxt, TCP_FLAG_ACK, 0, 0);
      return 1;
    }
  }

  if (c->state == TCP_STATE_SYN_RCVD) {
    if ((flags & TCP_FLAG_ACK) && ack == c->snd_nxt) {
      c->snd_una = ack;
      c->state = TCP_STATE_ESTABLISHED;
      c->waiting_ack = 0;
      return 1;
    }
  }

  if (c->state == TCP_STATE_ESTABLISHED || c->state == TCP_STATE_CLOSE_WAIT) {
    if ((flags & TCP_FLAG_ACK) && ack > c->snd_una) {
      c->snd_una = ack;
      if (ack >= c->snd_nxt) {
        c->waiting_ack = 0;
      }
    }

    if (seg_payload_len > 0u) {
      if (net_fault_should_apply(net_fault_loss_percent)) {
        net_rx_stats.injected_loss_frames++;
        (void)tcp_send_segment(c->remote_ip, c->local_port, c->remote_port,
                               c->snd_nxt, c->rcv_nxt, TCP_FLAG_ACK, 0, 0);
        return 1;
      }
      if (seq == c->rcv_nxt) {
        if (tcp_queue_rx(c, &tcp[hdr_len], seg_payload_len)) {
          c->rcv_nxt += seg_payload_len;
        }
      }
      (void)tcp_send_segment(c->remote_ip, c->local_port, c->remote_port,
                             c->snd_nxt, c->rcv_nxt, TCP_FLAG_ACK, 0, 0);
    }

    if ((flags & TCP_FLAG_FIN) != 0u) {
      if (seq == c->rcv_nxt) {
        c->rcv_nxt++;
      }
      c->state = TCP_STATE_CLOSE_WAIT;
      (void)tcp_send_segment(c->remote_ip, c->local_port, c->remote_port,
                             c->snd_nxt, c->rcv_nxt, TCP_FLAG_ACK, 0, 0);
    }
    return 1;
  }

  return 0;
}

static int udp_find_binding(uint16_t port) {
  for (int i = 0; i < UDP_MAX_BINDINGS; i++) {
    if (udp_bindings[i].used && udp_bindings[i].port == port) {
      return i;
    }
  }
  return -1;
}

static int udp_alloc_binding(uint16_t port) {
  int existing = udp_find_binding(port);
  if (existing >= 0) {
    return existing;
  }
  for (int i = 0; i < UDP_MAX_BINDINGS; i++) {
    if (!udp_bindings[i].used) {
      udp_bindings[i].used = 1;
      udp_bindings[i].port = port;
      udp_bindings[i].head = 0;
      udp_bindings[i].tail = 0;
      udp_bindings[i].count = 0;
      udp_bindings[i].dropped = 0;
      return i;
    }
  }
  return -1;
}

static int udp_enqueue_packet(uint16_t dst_port, uint32_t src_ip,
                              uint16_t src_port, const uint8_t *payload,
                              uint16_t payload_len) {
  int idx = udp_find_binding(dst_port);
  UdpBinding *binding;
  UdpPacket *slot;

  if (idx < 0 || payload == 0) {
    return 0;
  }
  if (payload_len > UDP_MAX_PAYLOAD) {
    udp_bindings[idx].dropped++;
    return 0;
  }

  binding = &udp_bindings[idx];
  if (binding->count >= UDP_QUEUE_DEPTH) {
    binding->dropped++;
    return 0;
  }

  slot = &binding->queue[binding->tail];
  slot->src_ip = src_ip;
  slot->src_port = src_port;
  slot->len = payload_len;
  mem_copy(slot->data, payload, payload_len);

  binding->tail = (uint16_t)((binding->tail + 1u) % UDP_QUEUE_DEPTH);
  binding->count++;
  return 1;
}

static int udp_enqueue_with_faults(uint16_t dst_port, uint32_t src_ip,
                                   uint16_t src_port, const uint8_t *payload,
                                   uint16_t payload_len) {
  int ok;

  if (net_fault_should_apply(net_fault_loss_percent)) {
    net_rx_stats.injected_loss_frames++;
    return 1;
  }

  if (udp_reorder_hold.valid) {
    ok = udp_enqueue_packet(dst_port, src_ip, src_port, payload, payload_len);
    if (!ok) {
      return 0;
    }
    ok = udp_enqueue_packet(udp_reorder_hold.dst_port,
                            udp_reorder_hold.src_ip,
                            udp_reorder_hold.src_port,
                            udp_reorder_hold.data,
                            udp_reorder_hold.len);
    if (!ok) {
      return 0;
    }
    udp_reorder_hold.valid = 0;
    net_rx_stats.injected_reorder_frames++;
    return 1;
  }

  if (net_fault_should_apply(net_fault_reorder_percent) && payload_len <= UDP_MAX_PAYLOAD) {
    udp_reorder_hold.valid = 1;
    udp_reorder_hold.src_ip = src_ip;
    udp_reorder_hold.src_port = src_port;
    udp_reorder_hold.dst_port = dst_port;
    udp_reorder_hold.len = payload_len;
    if (payload_len > 0u) {
      mem_copy(udp_reorder_hold.data, payload, payload_len);
    }
    return 1;
  }

  return udp_enqueue_packet(dst_port, src_ip, src_port, payload, payload_len);
}

static int net_handle_udp_datagram(uint32_t src_ip, uint32_t dst_ip,
                                   const uint8_t *udp_packet,
                                   uint16_t udp_len) {
  uint16_t src_port;
  uint16_t dst_port;
  uint16_t header_len;
  uint16_t payload_len;
  uint16_t checksum;

  (void)dst_ip;

  if (udp_packet == 0 || udp_len < 8u) {
    net_count_malformed();
    return 0;
  }

  src_port = read_be16(&udp_packet[0]);
  dst_port = read_be16(&udp_packet[2]);
  header_len = read_be16(&udp_packet[4]);
  checksum = read_be16(&udp_packet[6]);

  if (header_len < 8u || header_len > udp_len) {
    net_count_malformed();
    return 0;
  }
  payload_len = (uint16_t)(header_len - 8u);

  if (checksum != 0u) {
    uint8_t verify[1600];
    uint16_t verify_len = (uint16_t)(12u + header_len);
    if (verify_len > sizeof(verify)) {
      net_count_malformed();
      return 0;
    }

    write_be32(&verify[0], src_ip);
    write_be32(&verify[4], dst_ip);
    verify[8] = 0;
    verify[9] = IP_PROTO_UDP;
    write_be16(&verify[10], header_len);
    mem_copy(&verify[12], udp_packet, header_len);
    if (internet_checksum(verify, verify_len) != 0u) {
      net_count_malformed();
      return 0;
    }
  }

  return udp_enqueue_with_faults(dst_port, src_ip, src_port, &udp_packet[8], payload_len);
}

static uint16_t internet_checksum(const uint8_t *data, uint32_t len) {
  uint32_t sum = 0;

  for (uint32_t i = 0; i + 1u < len; i += 2u) {
    sum += (uint16_t)(((uint16_t)data[i] << 8) | (uint16_t)data[i + 1u]);
  }
  if ((len & 1u) != 0u) {
    sum += (uint16_t)((uint16_t)data[len - 1u] << 8);
  }
  while ((sum >> 16) != 0u) {
    sum = (sum & 0xFFFFu) + (sum >> 16);
  }
  return (uint16_t)(~sum & 0xFFFFu);
}

static void ipv4_reassembly_gc(void) {
  for (int i = 0; i < IPV4_REASS_SLOT_MAX; i++) {
    if (!ipv4_reass[i].used) {
      continue;
    }
    if ((tick - ipv4_reass[i].last_tick) > IPV4_REASS_TIMEOUT_TICKS) {
      ipv4_reass[i].used = 0;
    }
  }
}

static int ipv4_reass_present_get(const Ipv4ReassSlot *slot, uint16_t block_idx) {
  uint16_t byte_idx = (uint16_t)(block_idx >> 3);
  uint8_t bit = (uint8_t)(1u << (block_idx & 7u));
  if (slot == 0 || byte_idx >= (uint16_t)sizeof(slot->present)) {
    return 0;
  }
  return (slot->present[byte_idx] & bit) != 0u;
}

static void ipv4_reass_present_set(Ipv4ReassSlot *slot, uint16_t block_idx) {
  uint16_t byte_idx = (uint16_t)(block_idx >> 3);
  uint8_t bit = (uint8_t)(1u << (block_idx & 7u));
  if (slot == 0 || byte_idx >= (uint16_t)sizeof(slot->present)) {
    return;
  }
  slot->present[byte_idx] |= bit;
}

static int ipv4_reassemble_packet(uint32_t src_ip, uint32_t dst_ip, uint8_t proto,
                                  uint16_t ident, uint16_t frag_off_flags,
                                  const uint8_t *ip_header, uint8_t header_len,
                                  const uint8_t *payload, uint16_t payload_len,
                                  uint8_t *out_packet, uint16_t *out_total_len) {
  uint16_t frag_units = (uint16_t)(frag_off_flags & 0x1FFFu);
  uint16_t frag_offset = (uint16_t)(frag_units * 8u);
  int more_frags = (frag_off_flags & 0x2000u) != 0u;
  int slot_idx = -1;
  uint32_t oldest_tick = 0xFFFFFFFFu;
  int oldest_idx = -1;
  Ipv4ReassSlot *slot;

  if (payload == 0 || ip_header == 0 || out_packet == 0 || out_total_len == 0) {
    return 0;
  }
  if (header_len < 20u || header_len > IPV4_MAX_HEADER) {
    return 0;
  }
  if ((uint32_t)frag_offset + (uint32_t)payload_len > IPV4_REASS_MAX_PAYLOAD) {
    return 0;
  }

  ipv4_reassembly_gc();

  for (int i = 0; i < IPV4_REASS_SLOT_MAX; i++) {
    if (!ipv4_reass[i].used) {
      if (slot_idx < 0) {
        slot_idx = i;
      }
      continue;
    }
    if (ipv4_reass[i].src_ip == src_ip && ipv4_reass[i].dst_ip == dst_ip &&
        ipv4_reass[i].ident == ident && ipv4_reass[i].proto == proto) {
      slot_idx = i;
      break;
    }
    if (ipv4_reass[i].last_tick < oldest_tick) {
      oldest_tick = ipv4_reass[i].last_tick;
      oldest_idx = i;
    }
  }

  if (slot_idx < 0) {
    slot_idx = (oldest_idx >= 0) ? oldest_idx : 0;
  }

  slot = &ipv4_reass[slot_idx];
  if (!slot->used || slot->src_ip != src_ip || slot->dst_ip != dst_ip ||
      slot->ident != ident || slot->proto != proto) {
    slot->used = 1;
    slot->src_ip = src_ip;
    slot->dst_ip = dst_ip;
    slot->ident = ident;
    slot->proto = proto;
    slot->header_len = 0;
    slot->have_first = 0;
    slot->total_known = 0;
    slot->total_payload_len = 0;
    mem_set(slot->present, 0, (uint32_t)sizeof(slot->present));
  }

  slot->last_tick = tick;

  if (frag_offset == 0u) {
    slot->header_len = header_len;
    slot->have_first = 1;
    mem_copy(slot->header, ip_header, header_len);
  }

  if (payload_len > 0u) {
    mem_copy(&slot->payload[frag_offset], payload, payload_len);
    {
      uint16_t first_block = (uint16_t)(frag_offset / 8u);
      uint16_t last_block = (uint16_t)(((uint32_t)frag_offset + payload_len + 7u) / 8u);
      for (uint16_t b = first_block; b < last_block; b++) {
        ipv4_reass_present_set(slot, b);
      }
    }
  }

  if (!more_frags) {
    slot->total_known = 1;
    slot->total_payload_len = (uint16_t)(frag_offset + payload_len);
  }

  if (!slot->have_first || !slot->total_known) {
    return 0;
  }

  {
    uint16_t blocks_needed = (uint16_t)((slot->total_payload_len + 7u) / 8u);
    for (uint16_t b = 0; b < blocks_needed; b++) {
      if (!ipv4_reass_present_get(slot, b)) {
        return 0;
      }
    }
  }

  mem_copy(out_packet, slot->header, slot->header_len);
  write_be16(&out_packet[2], (uint16_t)(slot->header_len + slot->total_payload_len));
  write_be16(&out_packet[6], 0u);
  write_be16(&out_packet[10], 0u);
  write_be16(&out_packet[10], internet_checksum(out_packet, slot->header_len));
  if (slot->total_payload_len > 0u) {
    mem_copy(&out_packet[slot->header_len], slot->payload, slot->total_payload_len);
  }
  *out_total_len = (uint16_t)(slot->header_len + slot->total_payload_len);
  slot->used = 0;
  return 1;
}

static int net_send_ipv4_raw(uint32_t dst_ip, uint8_t protocol,
                             const uint8_t *payload, uint16_t payload_len,
                             uint8_t ttl, uint16_t ident) {
  uint8_t dst_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint32_t next_hop = net_route_next_hop(dst_ip);
  uint16_t header_len = 20u;
  uint16_t mtu_payload = (uint16_t)((IPV4_ETH_MTU - header_len) & ~7u);

  if (!net_ready || payload == 0) {
    return 0;
  }

  if (dst_ip != 0xFFFFFFFFu) {
    if (!arp_cache_lookup(next_hop, dst_mac)) {
      (void)net_send_arp_request(next_hop);
    }
  }

  if ((uint16_t)(header_len + payload_len) <= IPV4_ETH_MTU) {
    uint8_t frame[1514];
    uint16_t total_len = (uint16_t)(header_len + payload_len);
    uint16_t frame_len = (uint16_t)(14u + total_len);

    mem_set(frame, 0, sizeof(frame));
    mac_copy(&frame[0], dst_mac);
    mac_copy(&frame[6], rtl_mac);
    write_be16(&frame[12], ETH_TYPE_IPV4);

    frame[14] = 0x45;
    frame[15] = 0x00;
    write_be16(&frame[16], total_len);
    write_be16(&frame[18], ident);
    write_be16(&frame[20], 0x0000u);
    frame[22] = ttl;
    frame[23] = protocol;
    write_be16(&frame[24], 0);
    write_be32(&frame[26], net_ip);
    write_be32(&frame[30], dst_ip);
    write_be16(&frame[24], internet_checksum(&frame[14], header_len));

    if (payload_len > 0u) {
      mem_copy(&frame[34], payload, payload_len);
    }

    if (frame_len < 60u) {
      frame_len = 60u;
    }
    return rtl8139_send_frame(frame, frame_len);
  }

  if (mtu_payload == 0u) {
    return 0;
  }

  {
    uint16_t offset = 0;
    while (offset < payload_len) {
      uint8_t frame[1514];
      uint16_t remain = (uint16_t)(payload_len - offset);
      uint16_t frag_payload = (remain > mtu_payload) ? mtu_payload : remain;
      uint16_t flags_off;
      uint16_t total_len;
      uint16_t frame_len;

      if (frag_payload != remain) {
        frag_payload &= (uint16_t)~7u;
      }
      if (frag_payload == 0u) {
        return 0;
      }

      flags_off = (uint16_t)((offset / 8u) & 0x1FFFu);
      if ((uint16_t)(offset + frag_payload) < payload_len) {
        flags_off |= 0x2000u;
      }

      total_len = (uint16_t)(header_len + frag_payload);
      frame_len = (uint16_t)(14u + total_len);

      mem_set(frame, 0, sizeof(frame));
      mac_copy(&frame[0], dst_mac);
      mac_copy(&frame[6], rtl_mac);
      write_be16(&frame[12], ETH_TYPE_IPV4);

      frame[14] = 0x45;
      frame[15] = 0x00;
      write_be16(&frame[16], total_len);
      write_be16(&frame[18], ident);
      write_be16(&frame[20], flags_off);
      frame[22] = ttl;
      frame[23] = protocol;
      write_be16(&frame[24], 0);
      write_be32(&frame[26], net_ip);
      write_be32(&frame[30], dst_ip);
      write_be16(&frame[24], internet_checksum(&frame[14], header_len));
      mem_copy(&frame[34], &payload[offset], frag_payload);

      if (frame_len < 60u) {
        frame_len = 60u;
      }
      if (!rtl8139_send_frame(frame, frame_len)) {
        return 0;
      }
      offset = (uint16_t)(offset + frag_payload);
    }
  }

  return 1;
}

static int net_forward_ipv4_packet(const uint8_t *ip_packet, uint16_t total_len) {
  uint8_t frame[1514];
  uint8_t dst_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t ihl;
  uint8_t ttl;
  uint32_t dst_ip;
  uint32_t next_hop;
  uint16_t frame_len;

  if (!net_ready || ip_packet == 0 || total_len < 20u || total_len > IPV4_ETH_MTU) {
    return 0;
  }

  ihl = (uint8_t)((ip_packet[0] & 0x0Fu) * 4u);
  if (ihl < 20u || ihl > total_len) {
    return 0;
  }

  ttl = ip_packet[8];
  if (ttl <= 1u) {
    return 0;
  }

  dst_ip = read_be32(&ip_packet[16]);
  if (!route_lookup(dst_ip, &next_hop)) {
    return 0;
  }
  if (!arp_cache_lookup(next_hop, dst_mac)) {
    (void)net_send_arp_request(next_hop);
  }

  mem_set(frame, 0, sizeof(frame));
  mac_copy(&frame[0], dst_mac);
  mac_copy(&frame[6], rtl_mac);
  write_be16(&frame[12], ETH_TYPE_IPV4);
  mem_copy(&frame[14], ip_packet, total_len);
  frame[22] = (uint8_t)(ttl - 1u);
  write_be16(&frame[24], 0);
  write_be16(&frame[24], internet_checksum(&frame[14], ihl));

  frame_len = (uint16_t)(14u + total_len);
  if (frame_len < 60u) {
    frame_len = 60u;
  }
  return rtl8139_send_frame(frame, frame_len);
}

static int net_send_icmp_echo_reply(const uint8_t dst_mac[6], uint32_t dst_ip,
                                    const uint8_t *icmp_request, uint16_t icmp_len) {
  uint8_t icmp[IPV4_REASS_MAX_PAYLOAD];
  uint16_t checksum;

  (void)dst_mac;

  if (!net_ready || icmp_request == 0) {
    return 0;
  }
  if (icmp_len < 8u || icmp_len > IPV4_REASS_MAX_PAYLOAD) {
    return 0;
  }

  mem_copy(icmp, icmp_request, icmp_len);
  icmp[0] = ICMP_TYPE_ECHO_REPLY;
  icmp[1] = 0x00;
  write_be16(&icmp[2], 0x0000u);
  checksum = internet_checksum(icmp, icmp_len);
  write_be16(&icmp[2], checksum);

  return net_send_ipv4_raw(dst_ip, (uint8_t)IP_PROTO_ICMP, icmp, icmp_len,
                           64u, ipv4_tx_ident++);
}

static int net_handle_ipv4_frame(const uint8_t *frame, uint16_t len) {
  uint8_t version_ihl;
  uint16_t header_len;
  uint16_t total_len;
  uint16_t ident;
  uint16_t frag_off_flags;
  uint32_t src_ip;
  uint32_t dst_ip;
  uint8_t protocol;
  const uint8_t *src_mac;
  const uint8_t *payload;
  uint8_t assembled[IPV4_MAX_HEADER + IPV4_REASS_MAX_PAYLOAD];
  uint16_t assembled_len = 0;
  const uint8_t *ip = &frame[14];

  if (frame == 0 || len < 34u) {
    net_count_malformed();
    return 0;
  }

  ipv4_reassembly_gc();

  version_ihl = frame[14];
  if ((version_ihl >> 4) != 4u) {
    return 0;
  }

  header_len = (uint16_t)((version_ihl & 0x0Fu) * 4u);
  if (header_len < 20u || header_len > 60u || len < (uint16_t)(14u + header_len)) {
    net_count_malformed();
    return 0;
  }

  total_len = read_be16(&frame[16]);
  if (total_len < header_len || len < (uint16_t)(14u + total_len)) {
    net_count_malformed();
    return 0;
  }

  if (internet_checksum(&frame[14], header_len) != 0u) {
    net_count_malformed();
    return 0;
  }

  protocol = frame[23];
  src_ip = read_be32(&frame[26]);
  dst_ip = read_be32(&frame[30]);
  ident = read_be16(&frame[18]);
  frag_off_flags = read_be16(&frame[20]);
  src_mac = &frame[6];
  arp_cache_update(src_ip, src_mac);

  if (dst_ip != net_ip && dst_ip != 0xFFFFFFFFu) {
    return net_forward_ipv4_packet(&frame[14], total_len);
  }

  if ((frag_off_flags & 0x3FFFu) != 0u) {
    if (!ipv4_reassemble_packet(src_ip, dst_ip, protocol, ident, frag_off_flags,
                                &frame[14], (uint8_t)header_len,
                                &frame[14u + header_len],
                                (uint16_t)(total_len - header_len),
                                assembled, &assembled_len)) {
      return 1;
    }

    ip = assembled;
    version_ihl = ip[0];
    header_len = (uint16_t)((version_ihl & 0x0Fu) * 4u);
    if (assembled_len < header_len) {
      return 0;
    }
    total_len = assembled_len;
    protocol = ip[9];
    src_ip = read_be32(&ip[12]);
    dst_ip = read_be32(&ip[16]);
  }

  if (dst_ip != net_ip && dst_ip != 0xFFFFFFFFu) {
    return 1;
  }

  if (protocol == IP_PROTO_ICMP) {
    uint16_t icmp_len = (uint16_t)(total_len - header_len);
    if (icmp_len < 8u) {
      return 0;
    }

    payload = &ip[header_len];
    if (payload[0] == ICMP_TYPE_ECHO_REQUEST && payload[1] == 0x00) {
      return net_send_icmp_echo_reply(src_mac, src_ip, payload, icmp_len);
    }
  } else if (protocol == IP_PROTO_UDP) {
    uint16_t udp_len = (uint16_t)(total_len - header_len);
    if (udp_len < 8u) {
      return 0;
    }
    payload = &ip[header_len];
    return net_handle_udp_datagram(src_ip, dst_ip, payload, udp_len);
  } else if (protocol == IP_PROTO_TCP) {
    uint16_t tcp_len = (uint16_t)(total_len - header_len);
    if (tcp_len < 20u) {
      return 0;
    }
    payload = &ip[header_len];
    return net_handle_tcp_segment(src_ip, dst_ip, payload, tcp_len);
  }

  return 1;
}

static int net_handle_arp_frame(const uint8_t *frame, uint16_t len) {
  uint16_t htype;
  uint16_t ptype;
  uint16_t oper;
  uint32_t sender_ip;
  uint32_t target_ip;
  const uint8_t *sender_mac;
  const uint8_t *target_mac;

  if (frame == 0 || len < 42) {
    net_count_malformed();
    return 0;
  }

  htype = read_be16(&frame[14]);
  ptype = read_be16(&frame[16]);
  oper = read_be16(&frame[20]);
  if (htype != ARP_HTYPE_ETHERNET || ptype != ARP_PTYPE_IPV4 || frame[18] != 6 || frame[19] != 4) {
    return 0;
  }

  sender_mac = &frame[22];
  sender_ip = read_be32(&frame[28]);
  target_mac = &frame[32];
  target_ip = read_be32(&frame[38]);

  arp_cache_update(sender_ip, sender_mac);

  if (oper == ARP_OPER_REQUEST && target_ip == net_ip) {
    (void)net_send_arp_reply(sender_mac, sender_ip);
  }

  (void)target_mac;
  return 1;
}

static int net_handle_eth_frame(const uint8_t *frame, uint16_t len) {
  uint16_t ethertype;

  if (frame == 0 || len < 14) {
    net_rx_stats.dropped_frames++;
    return 0;
  }

  net_rx_stats.total_bytes += len;
  ethertype = read_be16(&frame[12]);
  net_count_ethertype(ethertype);
  if (ethertype == ETH_TYPE_IPV4) {
    return net_handle_ipv4_frame(frame, len);
  }
  if (ethertype == ETH_TYPE_ARP) {
    return net_handle_arp_frame(frame, len);
  }
  return 1;
}

static uint32_t remote_checksum32(const uint8_t *data, uint32_t len) {
  uint32_t hash = 2166136261u;
  for (uint32_t i = 0; i < len; i++) {
    hash ^= data[i];
    hash *= 16777619u;
  }
  return hash;
}

static void remote_seed_nonce_state(void) {
  remote_nonce_state = remote_token ^ remote_session ^ remote_key_epoch ^ 0x9E3779B9u;
  if (remote_nonce_state == 0u) {
    remote_nonce_state = 0xA341316Cu;
  }
}

static uint32_t remote_next_nonce(uint32_t request_id) {
  remote_nonce_state ^= remote_nonce_state << 13;
  remote_nonce_state ^= remote_nonce_state >> 17;
  remote_nonce_state ^= remote_nonce_state << 5;
  remote_nonce_state ^= request_id;
  if (remote_nonce_state == 0u) {
    remote_nonce_state = 0x7F4A7C15u ^ request_id;
  }
  return remote_nonce_state;
}

static void remote_rotate_session_key_internal(void) {
  uint32_t base = (tick ^ remote_token ^ (remote_key_epoch * 0x45D9F3Bu));
  remote_key_epoch++;
  remote_session = (base | 1u);
  remote_seq = 1u;
  remote_seed_nonce_state();
}

static int remote_auth_valid(void) {
  if (remote_token == 0) {
    return 0;
  }
  if (remote_auth_deadline == 0) {
    return 0;
  }
  if (tick > remote_auth_deadline) {
    return 0;
  }
  if (remote_role < REMOTE_ROLE_VIEWER || remote_role > REMOTE_ROLE_ADMIN) {
    return 0;
  }
  return 1;
}

static uint32_t remote_next_request_id(void) {
  uint32_t req = remote_seq++;
  if (req == 0) {
    req = remote_seq++;
  }
  return req;
}

static int read_rtl8139_bar0_iobase(int idx, uint32_t *out_iobase) {
  uint32_t bar0;
  PciDevice *d;

  if (idx < 0 || idx >= pci_found_count || out_iobase == 0)
    return 0;

  d = &pci_found[idx];
  bar0 = pci_read_config(d->bus, d->slot, d->function, 0x10);

  if ((bar0 & 0x1) == 0) {
    return 0;
  }

  *out_iobase = bar0 & 0xFFFFFFFC;
  return *out_iobase != 0;
}

static int rtl8139_enable_busmaster(int idx) {
  uint32_t cmd;
  PciDevice *d;

  if (idx < 0 || idx >= pci_found_count)
    return 0;
  d = &pci_found[idx];

  cmd = pci_read_config(d->bus, d->slot, d->function, 0x04);
  cmd |= 0x00000007; /* IO + MEM + BUSMASTER */
  pci_write_config(d->bus, d->slot, d->function, 0x04, cmd);
  return 1;
}

static int rtl8139_hw_init(uint32_t iobase) {
  uint32_t tries;

  outb((uint16_t)(iobase + RTL_REG_CONFIG1), 0x00);

  outb((uint16_t)(iobase + RTL_REG_CR), RTL_CMD_RESET);
  tries = 0;
  while (tries < 100000) {
    if ((inb((uint16_t)(iobase + RTL_REG_CR)) & RTL_CMD_RESET) == 0)
      break;
    tries++;
  }
  if (tries >= 100000)
    return 0;

  outl((uint16_t)(iobase + RTL_REG_RBSTART), (uint32_t)rtl_rx_buffer);
  rtl_rx_offset = 0;
  outw((uint16_t)(iobase + RTL_REG_CAPR), 0xFFF0u);
  outw((uint16_t)(iobase + RTL_REG_IMR), RTL_INT_MASK);
  outw((uint16_t)(iobase + RTL_REG_ISR), 0xFFFFu);
  outl((uint16_t)(iobase + RTL_REG_RCR),
       RTL_RCR_AAP | RTL_RCR_APM | RTL_RCR_AM | RTL_RCR_AB);

  outb((uint16_t)(iobase + RTL_REG_CR), RTL_CMD_RX_ENABLE | RTL_CMD_TX_ENABLE);

  for (int i = 0; i < 6; i++) {
    rtl_mac[i] = inb((uint16_t)(iobase + RTL_REG_IDR0 + i));
  }

  return 1;
}

static int net_rx_poll_budget(uint32_t max_packets) {
  uint32_t guard = 0;
  int processed = 0;

  if (!net_ready || rtl_iobase == 0) {
    return 0;
  }

  while (guard < max_packets) {
    uint32_t hw_offset = (uint32_t)inw((uint16_t)(rtl_iobase + RTL_REG_CBR)) & RTL_RX_BUFFER_MASK;
    uint8_t hdr[4];
    uint16_t status;
    uint16_t pkt_len;
    uint32_t frame_len;
    uint8_t frame[2048];

    if (rtl_rx_offset == hw_offset) {
      break;
    }

    copy_ring_bytes(rtl_rx_offset, hdr, 4);
    status = read_le16(&hdr[0]);
    pkt_len = read_le16(&hdr[2]);
    if ((status & 0x0001u) == 0u || pkt_len < 4u || pkt_len > (uint16_t)(sizeof(frame) + 4u)) {
      net_rx_stats.dropped_frames++;
      break;
    }

    frame_len = (uint32_t)pkt_len - 4u;
    if (frame_len > sizeof(frame)) {
      frame_len = sizeof(frame);
    }
    copy_ring_bytes((rtl_rx_offset + 4u) & RTL_RX_BUFFER_MASK, frame, frame_len);
    (void)net_handle_eth_frame(frame, (uint16_t)frame_len);

    rtl_rx_offset = (rtl_rx_offset + (uint32_t)pkt_len + 4u + 3u) & ~3u;
    rtl_rx_offset &= RTL_RX_BUFFER_MASK;
    outw((uint16_t)(rtl_iobase + RTL_REG_CAPR), (uint16_t)((rtl_rx_offset - 0x10u) & 0xFFFFu));
    processed++;
    guard++;
  }

  return processed;
}

void net_set_rx_coalesce(uint32_t poll_budget, uint32_t irq_budget) {
  if (poll_budget < 4u) {
    poll_budget = 4u;
  }
  if (poll_budget > 256u) {
    poll_budget = 256u;
  }
  if (irq_budget < 8u) {
    irq_budget = 8u;
  }
  if (irq_budget > 512u) {
    irq_budget = 512u;
  }
  net_rx_poll_budget_cfg = poll_budget;
  net_irq_poll_budget_cfg = irq_budget;
}

void net_get_rx_coalesce(uint32_t *poll_budget, uint32_t *irq_budget) {
  if (poll_budget != 0) {
    *poll_budget = net_rx_poll_budget_cfg;
  }
  if (irq_budget != 0) {
    *irq_budget = net_irq_poll_budget_cfg;
  }
}

int net_rx_poll(void) {
  int processed = net_rx_poll_budget(net_rx_poll_budget_cfg);
  if ((uint32_t)processed >= net_rx_poll_budget_cfg) {
    net_rx_stats.coalesced_batches++;
  }
  return processed;
}

int net_irq_handler(void) {
  uint16_t isr;
  int processed = 0;

  if (!net_ready || rtl_iobase == 0) {
    return 0;
  }

  isr = inw((uint16_t)(rtl_iobase + RTL_REG_ISR));
  if ((isr & RTL_INT_MASK) == 0u) {
    return 0;
  }
  net_rx_stats.irq_count++;

  outw((uint16_t)(rtl_iobase + RTL_REG_ISR), isr);

  if ((isr & (RTL_INT_RX_OK | RTL_INT_RX_ERR | RTL_INT_RX_OVW)) != 0u) {
    net_rx_stats.irq_rx_events++;
    processed = net_rx_poll_budget(net_irq_poll_budget_cfg);
    if ((uint32_t)processed >= net_irq_poll_budget_cfg) {
      net_rx_stats.coalesced_batches++;
    }
  }

  return processed;
}

void net_get_rx_stats(NetRxStats *out) {
  if (out == 0) {
    return;
  }
  net_rx_stats.poll_budget = net_rx_poll_budget_cfg;
  net_rx_stats.irq_poll_budget = net_irq_poll_budget_cfg;
  *out = net_rx_stats;
}

void net_set_fault_injection(uint32_t loss_percent, uint32_t reorder_percent) {
  if (loss_percent > 100u) {
    loss_percent = 100u;
  }
  if (reorder_percent > 100u) {
    reorder_percent = 100u;
  }
  net_fault_loss_percent = loss_percent;
  net_fault_reorder_percent = reorder_percent;
  if (loss_percent == 0u && reorder_percent == 0u) {
    udp_reorder_hold.valid = 0;
  }
}

void net_get_fault_injection(uint32_t *loss_percent, uint32_t *reorder_percent) {
  if (loss_percent != 0) {
    *loss_percent = net_fault_loss_percent;
  }
  if (reorder_percent != 0) {
    *reorder_percent = net_fault_reorder_percent;
  }
}

static int rtl8139_send_frame(const uint8_t *frame, uint32_t len) {
  uint32_t tx_addr_port;
  uint32_t tx_stat_port;
  int slot;

  if (!net_ready || rtl_iobase == 0 || frame == 0)
    return 0;
  if (len == 0 || len > 1514)
    return 0;

  slot = rtl_tx_slot;
  rtl_tx_slot = (rtl_tx_slot + 1) & 3;

  mem_copy(rtl_tx_buffer[slot], frame, len);

  tx_addr_port = rtl_iobase + RTL_REG_TSAD0 + (slot * 4);
  tx_stat_port = rtl_iobase + RTL_REG_TSD0 + (slot * 4);

  outl((uint16_t)tx_addr_port, (uint32_t)rtl_tx_buffer[slot]);
  outl((uint16_t)tx_stat_port, len & 0x1FFF);
  return 1;
}

static int net_send_udp(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port,
                        const uint8_t *payload, uint16_t payload_len) {
  uint8_t udp_packet[IPV4_REASS_MAX_PAYLOAD + 8u];
  uint16_t udp_len;

  if (!net_ready || payload == 0) {
    return 0;
  }
  if ((uint32_t)payload_len > IPV4_REASS_MAX_PAYLOAD) {
    return 0;
  }

  udp_len = (uint16_t)(8 + payload_len);
  write_be16(&udp_packet[0], src_port);
  write_be16(&udp_packet[2], dst_port);
  write_be16(&udp_packet[4], udp_len);
  write_be16(&udp_packet[6], 0u);
  if (payload_len > 0u) {
    mem_copy(&udp_packet[8], payload, payload_len);
  }

  if (dst_ip == net_ip || dst_ip == 0x7F000001u || dst_ip == 0xFFFFFFFFu) {
    (void)udp_enqueue_with_faults(dst_port, net_ip, src_port, payload, payload_len);
  }

  return net_send_ipv4_raw(dst_ip, (uint8_t)IP_PROTO_UDP, udp_packet, udp_len,
                           64u, ipv4_tx_ident++);
}

static int remote_send_payload(uint8_t type, const uint8_t *data, uint16_t len,
                               uint32_t flags, uint32_t request_id,
                               uint32_t status) {
  uint8_t payload[512];
  RemotePacketHeader header;
  uint16_t total;
  int sent;

  if (!net_ready || !remote_enabled || !remote_auth_valid()) {
    return 0;
  }
  if (data == 0 && len != 0) {
    return 0;
  }
  if (len > (uint16_t)(sizeof(payload) - sizeof(RemotePacketHeader))) {
    return 0;
  }

  header.magic = be32(REMOTE_PKT_MAGIC);
  header.version = be32(REMOTE_PKT_VERSION);
  header.type = be32((uint32_t)type);
  header.flags = be32(flags);
  header.session = be32(remote_session);
  header.request_id = be32(request_id);
  header.token = be32(remote_token);
  header.nonce = be32(remote_next_nonce(request_id));
  header.role = be32(remote_role);
  header.checksum = 0;
  header.status = be32(status);
  header.payload_len = be32((uint32_t)len);

  mem_copy(payload, (const uint8_t *)&header, (uint32_t)sizeof(header));
  if (len > 0) {
    mem_copy(&payload[sizeof(RemotePacketHeader)], data, len);
  }

  header.checksum = be32(remote_checksum32(payload, (uint32_t)(sizeof(RemotePacketHeader) + len)));
  mem_copy(payload, (const uint8_t *)&header, (uint32_t)sizeof(header));

  total = (uint16_t)(sizeof(RemotePacketHeader) + len);
  sent = net_send_udp(remote_dst_ip, UDP_CTRL_PORT, UDP_DATA_PORT, payload, total);
  if ((flags & REMOTE_PKT_FLAG_RELIABLE) != 0u && sent) {
    sent = net_send_udp(remote_dst_ip, UDP_CTRL_PORT, UDP_DATA_PORT, payload, total);
  }
  return sent;
}

static int remote_send_chunked_payload(uint8_t type, const uint8_t *data,
                                       uint16_t len, uint32_t flags,
                                       uint32_t request_id, uint32_t status) {
  uint32_t transfer_checksum;
  uint32_t chunk_count;
  uint32_t chunk_index;
  uint32_t offset;
  uint16_t chunk_limit;
  uint8_t chunk_payload[512];
  RemoteChunkHeader chunk_header;

  if (data == 0 && len != 0) {
    return 0;
  }

  if (len == 0) {
    chunk_count = 1;
  } else {
    chunk_limit = (uint16_t)(sizeof(chunk_payload) - sizeof(RemoteChunkHeader));
    if (chunk_limit == 0) {
      return 0;
    }
    chunk_count = (uint32_t)((len + chunk_limit - 1u) / chunk_limit);
  }

  transfer_checksum = remote_checksum32(data, len);
  if (request_id == 0) {
    request_id = remote_next_request_id();
  }

  chunk_limit = (uint16_t)(sizeof(chunk_payload) - sizeof(RemoteChunkHeader));
  offset = 0;
  for (chunk_index = 0; chunk_index < chunk_count; chunk_index++) {
    uint32_t remaining = (offset < len) ? (uint32_t)(len - offset) : 0u;
    uint32_t this_len = remaining > chunk_limit ? (uint32_t)chunk_limit : remaining;
    uint32_t chunk_flags = flags | REMOTE_PKT_FLAG_CHUNKED;

    if (chunk_index == 0) {
      chunk_flags |= REMOTE_PKT_FLAG_CHUNK_FIRST;
    }
    if (chunk_index + 1u == chunk_count) {
      chunk_flags |= REMOTE_PKT_FLAG_CHUNK_LAST;
    }

    chunk_header.magic = be32(REMOTE_CHUNK_MAGIC);
    chunk_header.transfer_id = be32(request_id);
    chunk_header.chunk_index = be32(chunk_index);
    chunk_header.chunk_count = be32(chunk_count);
    chunk_header.total_len = be32((uint32_t)len);
    chunk_header.chunk_offset = be32(offset);
    chunk_header.chunk_len = be32(this_len);
    chunk_header.checksum = be32(transfer_checksum);

    mem_copy(chunk_payload, (const uint8_t *)&chunk_header,
             (uint32_t)sizeof(chunk_header));
    if (this_len > 0 && data != 0) {
      mem_copy(&chunk_payload[sizeof(RemoteChunkHeader)], &data[offset], this_len);
    }

    if (!remote_send_payload(type, chunk_payload,
                             (uint16_t)(sizeof(RemoteChunkHeader) + this_len),
                             chunk_flags, request_id, status)) {
      return 0;
    }

    offset += this_len;
  }

  return 1;
}

int net_init(void) {
  for (int i = 0; i < pci_found_count; i++) {
    if (pci_found[i].class_code == 0x02 && pci_found[i].vendor == RTL8139_VENDOR &&
        pci_found[i].device_id == RTL8139_DEVICE) {
      net_idx = i;
      break;
    }
  }

  if (net_idx < 0) {
    net_ready = 0;
    return 0;
  }

  if (!read_rtl8139_bar0_iobase(net_idx, &rtl_iobase)) {
    net_ready = 0;
    return 0;
  }

  rtl8139_enable_busmaster(net_idx);
  rtl_rx_offset = 0;
  net_rx_stats.total_frames = 0;
  net_rx_stats.total_bytes = 0;
  net_rx_stats.ipv4_frames = 0;
  net_rx_stats.arp_frames = 0;
  net_rx_stats.neuro_frames = 0;
  net_rx_stats.unknown_frames = 0;
  net_rx_stats.dropped_frames = 0;
  net_rx_stats.malformed_frames = 0;
  net_rx_stats.quarantined_frames = 0;
  net_rx_stats.injected_loss_frames = 0;
  net_rx_stats.injected_reorder_frames = 0;
  net_rx_stats.tcp_retransmits = 0;
  net_rx_stats.tcp_retransmit_failures = 0;
  net_rx_stats.tcp_half_open_cleanups = 0;
  net_rx_stats.tcp_reset_events = 0;
  net_rx_stats.irq_count = 0;
  net_rx_stats.irq_rx_events = 0;
  net_rx_stats.tx_probe_ok = 0;
  net_rx_stats.tx_probe_fail = 0;
  net_fault_loss_percent = 0u;
  net_fault_reorder_percent = 0u;
  udp_reorder_hold.valid = 0;
  route_table_refresh_defaults();
  ipv4_tx_ident = 1u;
  for (int i = 0; i < IPV4_REASS_SLOT_MAX; i++) {
    ipv4_reass[i].used = 0;
  }
  for (int i = 0; i < UDP_MAX_BINDINGS; i++) {
    udp_bindings[i].used = 0;
    udp_bindings[i].port = 0;
    udp_bindings[i].head = 0;
    udp_bindings[i].tail = 0;
    udp_bindings[i].count = 0;
    udp_bindings[i].dropped = 0;
  }
  for (int i = 0; i < TCP_MAX_CONNS; i++) {
    tcp_conns[i].used = 0;
    tcp_conns[i].listening = 0;
    tcp_conns[i].parent_listen = -1;
    tcp_conns[i].accepted = 0;
    tcp_conns[i].state = TCP_STATE_CLOSED;
    tcp_conns[i].local_port = 0;
    tcp_conns[i].remote_port = 0;
    tcp_conns[i].remote_ip = 0;
    tcp_conns[i].snd_una = 0;
    tcp_conns[i].snd_nxt = 0;
    tcp_conns[i].rcv_nxt = 0;
    tcp_conns[i].rx_head = 0;
    tcp_conns[i].rx_tail = 0;
    tcp_conns[i].rx_count = 0;
    tcp_conns[i].last_tx_tick = 0;
    tcp_conns[i].last_tx_seq = 0;
    tcp_conns[i].last_tx_ack = 0;
    tcp_conns[i].last_tx_len = 0;
    tcp_conns[i].last_tx_flags = 0;
    tcp_conns[i].last_tx_retries = 0;
    tcp_conns[i].waiting_ack = 0;
  }
  net_ready = rtl8139_hw_init(rtl_iobase);
  if (net_ready && remote_session == 0) {
    remote_rotate_session_key_internal();
  }
  return net_ready;
}

int net_up(void) {
  if (net_ready) {
    return 1;
  }
  return net_init();
}

int net_is_ready(void) { return net_ready; }

uint32_t net_nic_io_base(void) { return rtl_iobase; }

int net_nic_index(void) { return net_idx; }

const char *net_driver_name(void) {
  if (!net_ready)
    return "NONE";
  return "RTL8139";
}

void net_get_mac(uint8_t out[6]) {
  if (out == 0)
    return;
  for (int i = 0; i < 6; i++)
    out[i] = rtl_mac[i];
}

int net_set_mac(const uint8_t mac[6]) {
  if (mac == 0 || !net_ready) {
    return 0;
  }
  for (int i = 0; i < 6; i++) {
    rtl_mac[i] = mac[i];
    outb((uint16_t)(rtl_iobase + RTL_REG_IDR0 + i), mac[i]);
  }
  return 1;
}

int net_link_speed_mbps(void) {
  uint8_t msr;

  if (!net_ready) {
    return 0;
  }

  msr = inb((uint16_t)(rtl_iobase + RTL_REG_MSR));
  if ((msr & RTL_MSR_SPEED_10) != 0u) {
    return 10;
  }
  return 100;
}

uint16_t net_mtu_bytes(void) { return 1500u; }

int net_supports_jumbo(void) { return 0; }

int net_set_ipv4(uint32_t ip, uint32_t mask, uint32_t gw) {
  if (ip == 0 || mask == 0) {
    return 0;
  }
  net_ip = ip;
  net_mask = mask;
  net_gw = gw;
  route_table_refresh_defaults();
  return 1;
}

int net_udp_bind(uint16_t port) {
  if (port == 0u) {
    return 0;
  }
  return udp_alloc_binding(port) >= 0;
}

int net_udp_unbind(uint16_t port) {
  int idx = udp_find_binding(port);
  if (idx < 0) {
    return 0;
  }
  udp_bindings[idx].used = 0;
  udp_bindings[idx].port = 0;
  udp_bindings[idx].head = 0;
  udp_bindings[idx].tail = 0;
  udp_bindings[idx].count = 0;
  udp_bindings[idx].dropped = 0;
  return 1;
}

int net_udp_has_data(uint16_t port) {
  int idx = udp_find_binding(port);

  if (idx < 0) {
    return 0;
  }
  return udp_bindings[idx].count > 0u;
}

int net_udp_send(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port,
                 const void *payload, uint16_t payload_len) {
  if (payload == 0) {
    return 0;
  }
  return net_send_udp(dst_ip, src_port, dst_port, (const uint8_t *)payload, payload_len);
}

int net_udp_recv(uint16_t port, uint32_t *src_ip, uint16_t *src_port,
                 void *buf, uint16_t max_len) {
  int idx = udp_find_binding(port);
  UdpBinding *binding;
  UdpPacket *pkt;
  uint16_t copy_len;

  if (idx < 0 || buf == 0 || max_len == 0u) {
    return 0;
  }

  binding = &udp_bindings[idx];
  if (binding->count == 0u) {
    return 0;
  }

  pkt = &binding->queue[binding->head];
  copy_len = pkt->len;
  if (copy_len > max_len) {
    copy_len = max_len;
  }
  mem_copy((uint8_t *)buf, pkt->data, copy_len);
  if (src_ip != 0) {
    *src_ip = pkt->src_ip;
  }
  if (src_port != 0) {
    *src_port = pkt->src_port;
  }

  binding->head = (uint16_t)((binding->head + 1u) % UDP_QUEUE_DEPTH);
  binding->count--;
  return (int)copy_len;
}

int net_tcp_listen(uint16_t port) {
  int idx;
  if (port == 0u || tcp_find_listener(port) >= 0) {
    return -1;
  }
  idx = tcp_alloc_conn();
  if (idx < 0) {
    return -1;
  }
  tcp_conns[idx].listening = 1;
  tcp_conns[idx].state = TCP_STATE_LISTEN;
  tcp_conns[idx].local_port = port;
  tcp_conns[idx].accepted = 1;
  return idx;
}

int net_tcp_unlisten(uint16_t port) {
  int idx = tcp_find_listener(port);
  if (idx < 0) {
    return 0;
  }
  tcp_conns[idx].used = 0;
  return 1;
}

int net_tcp_accept_ready(uint16_t listen_port) {
  int listener = tcp_find_listener(listen_port);

  if (listener < 0) {
    return 0;
  }
  for (int i = 0; i < TCP_MAX_CONNS; i++) {
    if (!tcp_conns[i].used || tcp_conns[i].listening) {
      continue;
    }
    if (tcp_conns[i].parent_listen == listener && tcp_conns[i].state == TCP_STATE_ESTABLISHED &&
        !tcp_conns[i].accepted) {
      return 1;
    }
  }
  return 0;
}

int net_tcp_connect(uint32_t dst_ip, uint16_t dst_port, uint16_t src_port) {
  int idx;
  if (dst_ip == 0 || dst_port == 0 || src_port == 0) {
    return -1;
  }
  idx = tcp_alloc_conn();
  if (idx < 0) {
    return -1;
  }
  tcp_conns[idx].listening = 0;
  tcp_conns[idx].parent_listen = -1;
  tcp_conns[idx].accepted = 1;
  tcp_conns[idx].state = TCP_STATE_SYN_SENT;
  tcp_conns[idx].local_port = src_port;
  tcp_conns[idx].remote_port = dst_port;
  tcp_conns[idx].remote_ip = dst_ip;
  tcp_conns[idx].snd_una = tcp_iss_seed;
  tcp_conns[idx].snd_nxt = tcp_iss_seed + 1u;
  tcp_conns[idx].rcv_nxt = 0;
  tcp_iss_seed += 0x101u;

  if (!tcp_send_segment(dst_ip, src_port, dst_port, tcp_conns[idx].snd_una, 0,
                        TCP_FLAG_SYN, 0, 0)) {
    tcp_conns[idx].used = 0;
    return -1;
  }
  tcp_conns[idx].last_tx_tick = tick;
  tcp_conns[idx].last_tx_seq = tcp_conns[idx].snd_una;
  tcp_conns[idx].last_tx_ack = 0;
  tcp_conns[idx].last_tx_len = 0;
  tcp_conns[idx].last_tx_flags = TCP_FLAG_SYN;
  tcp_conns[idx].last_tx_retries = 0;
  tcp_conns[idx].waiting_ack = 1;
  return idx;
}

int net_tcp_accept(uint16_t listen_port, uint32_t *peer_ip, uint16_t *peer_port) {
  int listener = tcp_find_listener(listen_port);
  if (listener < 0) {
    return -1;
  }
  for (int i = 0; i < TCP_MAX_CONNS; i++) {
    if (!tcp_conns[i].used || tcp_conns[i].listening) {
      continue;
    }
    if (tcp_conns[i].parent_listen == listener && tcp_conns[i].state == TCP_STATE_ESTABLISHED &&
        !tcp_conns[i].accepted) {
      tcp_conns[i].accepted = 1;
      if (peer_ip != 0) {
        *peer_ip = tcp_conns[i].remote_ip;
      }
      if (peer_port != 0) {
        *peer_port = tcp_conns[i].remote_port;
      }
      return i;
    }
  }
  return -1;
}

int net_tcp_is_established(int conn_id) {
  TcpConn *c;

  if (conn_id < 0 || conn_id >= TCP_MAX_CONNS) {
    return 0;
  }
  c = &tcp_conns[conn_id];
  return c->used && !c->listening && c->state == TCP_STATE_ESTABLISHED;
}

int net_tcp_can_recv(int conn_id) {
  TcpConn *c;

  if (conn_id < 0 || conn_id >= TCP_MAX_CONNS) {
    return 0;
  }
  c = &tcp_conns[conn_id];
  return c->used && !c->listening && c->rx_count > 0u;
}

int net_tcp_can_send(int conn_id) {
  TcpConn *c;

  if (conn_id < 0 || conn_id >= TCP_MAX_CONNS) {
    return 0;
  }
  c = &tcp_conns[conn_id];
  return c->used && !c->listening && c->state == TCP_STATE_ESTABLISHED;
}

int net_tcp_send(int conn_id, const void *payload, uint16_t payload_len) {
  TcpConn *c;
  if (conn_id < 0 || conn_id >= TCP_MAX_CONNS || payload == 0 || payload_len == 0u) {
    return 0;
  }
  c = &tcp_conns[conn_id];
  if (!c->used || c->listening || c->state != TCP_STATE_ESTABLISHED) {
    return 0;
  }
  if (payload_len > TCP_MAX_PAYLOAD) {
    payload_len = TCP_MAX_PAYLOAD;
  }
  if (!tcp_send_and_track(c, TCP_FLAG_ACK | TCP_FLAG_PSH,
                          (const uint8_t *)payload, payload_len)) {
    return 0;
  }
  return (int)payload_len;
}

int net_tcp_recv(int conn_id, void *buf, uint16_t max_len) {
  TcpConn *c;
  TcpSegment *seg;
  uint16_t copy_len;
  if (conn_id < 0 || conn_id >= TCP_MAX_CONNS || buf == 0 || max_len == 0u) {
    return 0;
  }
  c = &tcp_conns[conn_id];
  if (!c->used || c->listening || c->rx_count == 0u) {
    return 0;
  }
  seg = &c->rx_queue[c->rx_head];
  copy_len = seg->len;
  if (copy_len > max_len) {
    copy_len = max_len;
  }
  mem_copy((uint8_t *)buf, seg->data, copy_len);
  c->rx_head = (uint16_t)((c->rx_head + 1u) % TCP_QUEUE_DEPTH);
  c->rx_count--;
  return (int)copy_len;
}

int net_tcp_close(int conn_id) {
  TcpConn *c;
  if (conn_id < 0 || conn_id >= TCP_MAX_CONNS) {
    return 0;
  }
  c = &tcp_conns[conn_id];
  if (!c->used) {
    return 0;
  }
  if (c->listening) {
    c->used = 0;
    c->state = TCP_STATE_CLOSED;
    return 1;
  }
  if (c->state == TCP_STATE_ESTABLISHED || c->state == TCP_STATE_CLOSE_WAIT) {
    if (tcp_send_and_track(c, TCP_FLAG_FIN | TCP_FLAG_ACK, 0, 0)) {
      c->state = TCP_STATE_CLOSING;
      return 1;
    }
  }
  c->used = 0;
  c->state = TCP_STATE_CLOSED;
  return 1;
}

int net_tcp_poll(void) {
  int retransmits = 0;
  for (int i = 0; i < TCP_MAX_CONNS; i++) {
    TcpConn *c = &tcp_conns[i];
    if (!c->used || c->listening || !c->waiting_ack) {
      continue;
    }
    if ((tick - c->last_tx_tick) >= TCP_RETX_TICKS) {
      if (tcp_retransmit_last(c)) {
        retransmits++;
      }
    }
  }
  return retransmits;
}

void net_get_ipv4(uint32_t *ip, uint32_t *mask, uint32_t *gw) {
  if (ip)
    *ip = net_ip;
  if (mask)
    *mask = net_mask;
  if (gw)
    *gw = net_gw;
}

int net_send_probe(void) {
  uint8_t p[24];
  int sent;

  if (!net_ready) {
    net_rx_stats.tx_probe_fail++;
    return 0;
  }

  p[0] = 'N';
  p[1] = 'S';
  p[2] = 'P';
  p[3] = 'R';
  p[4] = 'O';
  p[5] = 'B';
  p[6] = 'E';
  p[7] = 0;
  *((uint32_t *)&p[8]) = be32(net_ip);
  *((uint32_t *)&p[12]) = be32(net_mask);
  *((uint32_t *)&p[16]) = be32(net_gw);
  *((uint32_t *)&p[20]) = be32(remote_session);

  sent = net_send_udp(0xFFFFFFFFu, UDP_CTRL_PORT, UDP_CTRL_PORT, p, sizeof(p));
  if (sent) {
    net_rx_stats.tx_probe_ok++;
  } else {
    net_rx_stats.tx_probe_fail++;
  }
  return sent;
}

int net_export_snapshot(int slot) {
  int v = 0, w = 0, t = 0;
  char tag[16];
  uint8_t payload[64];
  int tag_len = 0;
  int total;
  uint32_t checksum;
  uint32_t request_id;

  if (!net_ready)
    return 0;
  if (!storage_get_snapshot_signature(slot, &v, &w, &t))
    return 0;
  if (!storage_get_snapshot_tag(slot, tag, sizeof(tag))) {
    tag[0] = '-';
    tag[1] = '\0';
  }

  payload[0] = 'S';
  payload[1] = 'N';
  payload[2] = 'A';
  payload[3] = 'P';
  payload[4] = (uint8_t)slot;
  payload[5] = 0;
  payload[6] = 0;
  payload[7] = 0;
  *((uint32_t *)&payload[8]) = be32((uint32_t)v);
  *((uint32_t *)&payload[12]) = be32((uint32_t)w);
  *((uint32_t *)&payload[16]) = be32((uint32_t)t);

  while (tag[tag_len] && tag_len < 15) {
    payload[20 + tag_len] = (uint8_t)tag[tag_len];
    tag_len++;
  }
  payload[20 + tag_len] = '\0';
  total = 21 + tag_len;
  checksum = remote_checksum32(payload, (uint32_t)total);
  request_id = remote_next_request_id();

  payload[21 + tag_len] = (uint8_t)(checksum & 0xFFu);
  payload[22 + tag_len] = (uint8_t)((checksum >> 8) & 0xFFu);
  payload[23 + tag_len] = (uint8_t)((checksum >> 16) & 0xFFu);
  payload[24 + tag_len] = (uint8_t)((checksum >> 24) & 0xFFu);
  total += 4;

  if (remote_enabled && remote_auth_valid()) {
    return remote_send_chunked_payload(REMOTE_PKT_TYPE_SNAPSHOT, payload,
                                          (uint16_t)total, 0, request_id,
                                          REMOTE_STATUS_OK);
  }

  return net_send_udp(0xFFFFFFFFu, UDP_DATA_PORT, UDP_DATA_PORT, payload,
                      (uint16_t)total);
}

int net_export_profile(void) {
  ProfileSnapshot snap;
  uint8_t payload[64];
  uint32_t render_avg = 0;
  uint32_t sched_avg = 0;
  uint32_t spike_avg = 0;
  uint32_t cmd_avg = 0;
  uint32_t request_id;
  uint32_t checksum;

  if (!net_ready) {
    return 0;
  }

  profile_snapshot(&snap);
  if (snap.slots[PROFILE_SLOT_RENDER_PASS].count > 0) {
    render_avg = snap.slots[PROFILE_SLOT_RENDER_PASS].total_cycles /
                 snap.slots[PROFILE_SLOT_RENDER_PASS].count;
  }
  if (snap.slots[PROFILE_SLOT_SCHED_TICK].count > 0) {
    sched_avg = snap.slots[PROFILE_SLOT_SCHED_TICK].total_cycles /
                snap.slots[PROFILE_SLOT_SCHED_TICK].count;
  }
  if (snap.slots[PROFILE_SLOT_SPIKE_UPDATE].count > 0) {
    spike_avg = snap.slots[PROFILE_SLOT_SPIKE_UPDATE].total_cycles /
                snap.slots[PROFILE_SLOT_SPIKE_UPDATE].count;
  }
  if (snap.slots[PROFILE_SLOT_COMMAND].count > 0) {
    cmd_avg =
        snap.slots[PROFILE_SLOT_COMMAND].total_cycles / snap.slots[PROFILE_SLOT_COMMAND].count;
  }

  payload[0] = 'P';
  payload[1] = 'R';
  payload[2] = 'F';
  payload[3] = '1';
  *((uint32_t *)&payload[4]) = be32(render_avg);
  *((uint32_t *)&payload[8]) = be32(sched_avg);
  *((uint32_t *)&payload[12]) = be32(spike_avg);
  *((uint32_t *)&payload[16]) = be32(cmd_avg);
  *((uint32_t *)&payload[20]) = be32((uint32_t)snap.enabled);
  *((uint32_t *)&payload[24]) = be32(tick);

  checksum = remote_checksum32(payload, 28);
  request_id = remote_next_request_id();
  *((uint32_t *)&payload[28]) = be32(checksum);

  if (remote_enabled && remote_auth_valid()) {
    return remote_send_chunked_payload(REMOTE_PKT_TYPE_PROFILE, payload, 32, 0,
                                          request_id, REMOTE_STATUS_OK);
  }

  return net_send_udp(0xFFFFFFFFu, UDP_DATA_PORT, UDP_DATA_PORT, payload, 32);
}

int net_export_manifest(void) {
  char summary[192];
  uint8_t payload[256];
  uint32_t checksum;
  uint32_t request_id;
  int len;
  int pos;

  if (!net_ready) {
    return 0;
  }

  if (!storage_manifest_summary(summary, (int)sizeof(summary))) {
    return 0;
  }

  len = 0;
  payload[len++] = 'M';
  payload[len++] = 'A';
  payload[len++] = 'N';
  payload[len++] = '1';
  pos = 0;
  while (summary[pos] && len < (int)sizeof(payload) - 8) {
    payload[len++] = (uint8_t)summary[pos++];
  }
  payload[len++] = 0;
  checksum = remote_checksum32(payload, (uint32_t)len);
  *((uint32_t *)&payload[len]) = be32(checksum);
  len += 4;
  request_id = remote_next_request_id();

  if (remote_enabled && remote_auth_valid()) {
    return remote_send_chunked_payload(REMOTE_PKT_TYPE_MANIFEST, payload,
                                       (uint16_t)len, 0, request_id,
                                       REMOTE_STATUS_OK);
  }

  return net_send_udp(0xFFFFFFFFu, UDP_DATA_PORT, UDP_DATA_PORT, payload,
                      (uint16_t)len);
}

void remote_set_enabled(int enabled) { remote_enabled = enabled ? 1 : 0; }

int remote_is_enabled(void) { return remote_enabled; }

void remote_set_token(uint32_t token) {
  remote_token = token;
  remote_seed_nonce_state();
}

void remote_set_auth(uint32_t token, uint32_t ttl_ticks) {
  remote_token = token;
  remote_auth_deadline = ttl_ticks ? (tick + ttl_ticks) : 0;
  remote_rotate_session_key_internal();
}

void remote_clear_auth(void) {
  remote_auth_deadline = 0;
  remote_token = 0;
}

int remote_is_authorized(void) { return remote_auth_valid(); }

uint32_t remote_get_token(void) { return remote_token; }

uint32_t remote_get_auth_deadline(void) { return remote_auth_deadline; }

uint32_t remote_get_session(void) {
  if (remote_session == 0) {
    remote_rotate_session_key_internal();
  }
  return remote_session;
}

void remote_rotate_session_key(void) { remote_rotate_session_key_internal(); }

void remote_set_role(uint32_t role) {
  if (role < REMOTE_ROLE_VIEWER) {
    role = REMOTE_ROLE_VIEWER;
  }
  if (role > REMOTE_ROLE_ADMIN) {
    role = REMOTE_ROLE_ADMIN;
  }
  remote_role = role;
}

uint32_t remote_get_role(void) { return remote_role; }

int remote_send_command_result(const char *cmd_text) {
  uint8_t payload[128];
  int n = 0;
  uint32_t request_id;

  if (!remote_enabled || !net_ready || cmd_text == 0 || !remote_auth_valid()) {
    return 0;
  }
  if (remote_role < REMOTE_ROLE_OPERATOR) {
    return 0;
  }

  payload[n++] = 'C';
  payload[n++] = 'M';
  payload[n++] = 'D';
  payload[n++] = ':';
  while (*cmd_text && n < (int)sizeof(payload) - 1) {
    payload[n++] = (uint8_t)(*cmd_text);
    cmd_text++;
  }
  payload[n] = '\0';
  request_id = remote_next_request_id();

  return remote_send_chunked_payload(REMOTE_PKT_TYPE_COMMAND, payload,
                                     (uint16_t)(n + 1), REMOTE_PKT_FLAG_RELIABLE,
                                     request_id, REMOTE_STATUS_OK);
}
