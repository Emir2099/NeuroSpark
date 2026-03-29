#include "net.h"
#include "pci.h"
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

extern volatile int ata_disk_available;

static int net_ready = 0;
static int net_idx = -1;
static uint32_t rtl_iobase = 0;
static uint8_t rtl_mac[6] = {0, 0, 0, 0, 0, 0};

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

static void mem_copy(uint8_t *dst, const uint8_t *src, uint32_t len) {
  for (uint32_t i = 0; i < len; i++) {
    dst[i] = src[i];
  }
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
  return net_ready;
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

int net_send_probe(void) {
  uint8_t frame[64];

  if (!net_ready)
    return 0;

  for (int i = 0; i < 6; i++)
    frame[i] = 0xFF;
  for (int i = 0; i < 6; i++)
    frame[6 + i] = rtl_mac[i];

  frame[12] = (uint8_t)(ETH_TYPE_NEURO >> 8);
  frame[13] = (uint8_t)(ETH_TYPE_NEURO & 0xFF);

  frame[14] = 'N';
  frame[15] = 'E';
  frame[16] = 'U';
  frame[17] = 'R';
  frame[18] = 'O';
  frame[19] = 'S';
  frame[20] = 'P';
  frame[21] = 'A';
  frame[22] = 'R';
  frame[23] = 'K';
  frame[24] = '-';
  frame[25] = 'P';
  frame[26] = 'R';
  frame[27] = 'O';
  frame[28] = 'B';
  frame[29] = 'E';

  for (int i = 30; i < 64; i++)
    frame[i] = 0;

  return rtl8139_send_frame(frame, sizeof(frame));
}

int net_export_snapshot(int slot) {
  int v = 0, w = 0, t = 0;
  char tag[16];
  uint8_t frame[96];

  if (!net_ready)
    return 0;
  if (!storage_get_snapshot_signature(slot, &v, &w, &t))
    return 0;
  if (!storage_get_snapshot_tag(slot, tag, sizeof(tag))) {
    tag[0] = '-';
    tag[1] = '\0';
  }

  for (int i = 0; i < 6; i++)
    frame[i] = 0xFF;
  for (int i = 0; i < 6; i++)
    frame[6 + i] = rtl_mac[i];
  frame[12] = (uint8_t)(ETH_TYPE_NEURO >> 8);
  frame[13] = (uint8_t)(ETH_TYPE_NEURO & 0xFF);

  frame[14] = 'S';
  frame[15] = 'N';
  frame[16] = 'A';
  frame[17] = 'P';
  frame[18] = (uint8_t)slot;

  frame[19] = (uint8_t)(v & 0xFF);
  frame[20] = (uint8_t)((v >> 8) & 0xFF);
  frame[21] = (uint8_t)((v >> 16) & 0xFF);
  frame[22] = (uint8_t)((v >> 24) & 0xFF);

  frame[23] = (uint8_t)(w & 0xFF);
  frame[24] = (uint8_t)((w >> 8) & 0xFF);
  frame[25] = (uint8_t)((w >> 16) & 0xFF);
  frame[26] = (uint8_t)((w >> 24) & 0xFF);

  frame[27] = (uint8_t)(t & 0xFF);
  frame[28] = (uint8_t)((t >> 8) & 0xFF);
  frame[29] = (uint8_t)((t >> 16) & 0xFF);
  frame[30] = (uint8_t)((t >> 24) & 0xFF);

  for (int i = 0; i < 16; i++) {
    frame[31 + i] = (uint8_t)tag[i];
    if (tag[i] == '\0')
      break;
  }
  for (int i = 47; i < 96; i++) {
    frame[i] = 0;
  }

  return rtl8139_send_frame(frame, sizeof(frame));
}
