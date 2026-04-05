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
#define RTL_REG_IMR 0x3C
#define RTL_REG_RCR 0x44
#define RTL_REG_CONFIG1 0x52

#define RTL_CMD_RESET 0x10
#define RTL_CMD_RX_ENABLE 0x08
#define RTL_CMD_TX_ENABLE 0x04

#define RTL_RCR_AB 0x00000008
#define RTL_RCR_AM 0x00000004
#define RTL_RCR_APM 0x00000002
#define RTL_RCR_AAP 0x00000001

#define ETH_TYPE_NEURO 0x88B5
#define ETH_TYPE_IPV4 0x0800

#define UDP_CTRL_PORT 39000
#define UDP_DATA_PORT 39001

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

typedef struct {
  uint32_t magic;
  uint32_t version;
  uint32_t type;
  uint32_t flags;
  uint32_t session;
  uint32_t request_id;
  uint32_t token;
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

static uint8_t rtl_rx_buffer[8192 + 16 + 1500] __attribute__((aligned(16)));
static uint8_t rtl_tx_buffer[4][1600] __attribute__((aligned(16)));
static int rtl_tx_slot = 0;

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

static uint16_t be16(uint16_t v) {
  return (uint16_t)((v << 8) | (v >> 8));
}

static uint32_t be32(uint32_t v) {
  return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8) |
         ((v & 0x00FF0000u) >> 8) | ((v & 0xFF000000u) >> 24);
}

static uint16_t ipv4_checksum(const uint8_t *hdr, int len) {
  uint32_t sum = 0;
  for (int i = 0; i + 1 < len; i += 2) {
    uint16_t word = (uint16_t)((hdr[i] << 8) | hdr[i + 1]);
    sum += word;
  }
  while (sum >> 16) {
    sum = (sum & 0xFFFFu) + (sum >> 16);
  }
  return (uint16_t)(~sum & 0xFFFFu);
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

static uint32_t remote_checksum32(const uint8_t *data, uint32_t len) {
  uint32_t hash = 2166136261u;
  for (uint32_t i = 0; i < len; i++) {
    hash ^= data[i];
    hash *= 16777619u;
  }
  return hash;
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
  outw((uint16_t)(iobase + RTL_REG_IMR), 0x0000);
  outl((uint16_t)(iobase + RTL_REG_RCR),
       RTL_RCR_AAP | RTL_RCR_APM | RTL_RCR_AM | RTL_RCR_AB);

  outb((uint16_t)(iobase + RTL_REG_CR), RTL_CMD_RX_ENABLE | RTL_CMD_TX_ENABLE);

  for (int i = 0; i < 6; i++) {
    rtl_mac[i] = inb((uint16_t)(iobase + RTL_REG_IDR0 + i));
  }

  return 1;
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
  uint8_t frame[1514];
  uint16_t ip_len;
  uint16_t udp_len;
  uint16_t frame_len;
  uint16_t csum;

  if (!net_ready || payload == 0) {
    return 0;
  }
  if (payload_len > 1400) {
    return 0;
  }

  mem_set(frame, 0, sizeof(frame));

  for (int i = 0; i < 6; i++) {
    frame[i] = 0xFF;
    frame[6 + i] = rtl_mac[i];
  }
  frame[12] = (uint8_t)(ETH_TYPE_IPV4 >> 8);
  frame[13] = (uint8_t)(ETH_TYPE_IPV4 & 0xFF);

  ip_len = (uint16_t)(20 + 8 + payload_len);
  udp_len = (uint16_t)(8 + payload_len);

  frame[14] = 0x45;
  frame[15] = 0x00;
  *((uint16_t *)&frame[16]) = be16(ip_len);
  *((uint16_t *)&frame[18]) = be16((uint16_t)(remote_seq & 0xFFFFu));
  *((uint16_t *)&frame[20]) = 0x0000;
  frame[22] = 64;
  frame[23] = 17;
  *((uint16_t *)&frame[24]) = 0;
  *((uint32_t *)&frame[26]) = be32(net_ip);
  *((uint32_t *)&frame[30]) = be32(dst_ip);

  csum = ipv4_checksum(&frame[14], 20);
  *((uint16_t *)&frame[24]) = be16(csum);

  *((uint16_t *)&frame[34]) = be16(src_port);
  *((uint16_t *)&frame[36]) = be16(dst_port);
  *((uint16_t *)&frame[38]) = be16(udp_len);
  *((uint16_t *)&frame[40]) = 0;

  mem_copy(&frame[42], payload, payload_len);

  frame_len = (uint16_t)(14 + ip_len);
  if (frame_len < 60) {
    frame_len = 60;
  }
  return rtl8139_send_frame(frame, frame_len);
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
  net_ready = rtl8139_hw_init(rtl_iobase);
  if (net_ready && remote_session == 0) {
    remote_session = (tick ^ 0x4E53504Bu) | 1u;
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

int net_set_ipv4(uint32_t ip, uint32_t mask, uint32_t gw) {
  if (ip == 0 || mask == 0) {
    return 0;
  }
  net_ip = ip;
  net_mask = mask;
  net_gw = gw;
  return 1;
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

  if (!net_ready)
    return 0;

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

  return net_send_udp(0xFFFFFFFFu, UDP_CTRL_PORT, UDP_CTRL_PORT, p, sizeof(p));
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

void remote_set_token(uint32_t token) { remote_token = token; }

void remote_set_auth(uint32_t token, uint32_t ttl_ticks) {
  remote_token = token;
  remote_auth_deadline = ttl_ticks ? (tick + ttl_ticks) : 0;
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
    remote_session = (tick ^ 0x4E53504Bu) | 1u;
  }
  return remote_session;
}

int remote_send_command_result(const char *cmd_text) {
  uint8_t payload[128];
  int n = 0;
  uint32_t request_id;

  if (!remote_enabled || !net_ready || cmd_text == 0 || !remote_auth_valid()) {
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
