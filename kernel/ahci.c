#include "ahci.h"
#include "paging.h"
#include "pci.h"

#define AHCI_CLASS_CODE 0x01
#define AHCI_SUBCLASS 0x06

#define PCI_COMMAND_REG 0x04
#define PCI_CMD_IO_SPACE 0x0001
#define PCI_CMD_MEM_SPACE 0x0002
#define PCI_CMD_BUS_MASTER 0x0004

#define AHCI_GHC_AE (1U << 31)

#define HBA_PxCMD_ST (1U << 0)
#define HBA_PxCMD_FRE (1U << 4)
#define HBA_PxCMD_FR (1U << 14)
#define HBA_PxCMD_CR (1U << 15)

#define HBA_PxIS_TFES (1U << 30)

#define SATA_SIG_ATA 0x00000101U
#define SATA_SIG_ATAPI 0xEB140101U

#define FIS_TYPE_REG_H2D 0x27

#define ATA_CMD_IDENTIFY 0xEC
#define ATA_CMD_READ_DMA_EXT 0x25
#define ATA_CMD_WRITE_DMA_EXT 0x35

#define ATA_TFD_BSY 0x80
#define ATA_TFD_DRQ 0x08

typedef struct {
  uint8_t cfl;
  uint8_t pm_flags;
  uint16_t prdtl;
  uint32_t prdbc;
  uint32_t ctba;
  uint32_t ctbau;
  uint32_t reserved[4];
} __attribute__((packed)) HbaCmdHeader;

typedef struct {
  uint32_t dba;
  uint32_t dbau;
  uint32_t reserved;
  uint32_t dbc_i;
} __attribute__((packed)) HbaPrdtEntry;

typedef struct {
  uint8_t cfis[64];
  uint8_t acmd[16];
  uint8_t reserved[48];
  HbaPrdtEntry prdt[1];
} __attribute__((packed)) HbaCmdTable;

typedef struct {
  uint8_t fis_type;
  uint8_t pmport_c;
  uint8_t command;
  uint8_t featurel;
  uint8_t lba0;
  uint8_t lba1;
  uint8_t lba2;
  uint8_t device;
  uint8_t lba3;
  uint8_t lba4;
  uint8_t lba5;
  uint8_t featureh;
  uint8_t countl;
  uint8_t counth;
  uint8_t icc;
  uint8_t control;
  uint8_t reserved[4];
} __attribute__((packed)) FisRegH2D;

static volatile uint32_t *g_abar = 0;
static uint32_t g_abar_phys = 0;
static uint32_t g_pi_mask = 0;

static HbaCmdHeader g_cmd_list[32] __attribute__((aligned(1024)));
static uint8_t g_rx_fis[256] __attribute__((aligned(256)));
static HbaCmdTable g_cmd_table __attribute__((aligned(128)));
static uint16_t g_identify_buf[256] __attribute__((aligned(512)));
static uint16_t g_read_buf[256] __attribute__((aligned(512)));

static void ahci_zero_report(AhciProbeReport *out_report) {
  if (out_report == 0) {
    return;
  }
  for (uint32_t i = 0; i < (uint32_t)sizeof(AhciProbeReport); i++) {
    ((uint8_t *)out_report)[i] = 0;
  }
}

static int ahci_find_controller_index(void) {
  for (int i = 0; i < pci_found_count; i++) {
    if (pci_found[i].class_code == AHCI_CLASS_CODE &&
        pci_found[i].subclass == AHCI_SUBCLASS) {
      return i;
    }
  }
  return -1;
}

static void ahci_enable_pci_mmio_and_bm(int pci_index) {
  PciDevice *d = &pci_found[pci_index];
  uint32_t cmd_status = pci_read_config(d->bus, d->slot, d->function, PCI_COMMAND_REG);
  uint16_t cmd = (uint16_t)(cmd_status & 0xFFFF);

  cmd |= (PCI_CMD_IO_SPACE | PCI_CMD_MEM_SPACE | PCI_CMD_BUS_MASTER);
  cmd_status = (cmd_status & 0xFFFF0000U) | (uint32_t)cmd;

  pci_write_config(d->bus, d->slot, d->function, PCI_COMMAND_REG, cmd_status);
}

static void ahci_delay(volatile uint32_t loops) {
  while (loops--) {
    __asm__ volatile("nop");
  }
}

static void zero_bytes(void *ptr, uint32_t len) {
  uint8_t *p = (uint8_t *)ptr;
  for (uint32_t i = 0; i < len; i++) {
    p[i] = 0;
  }
}

static void copy_words(uint16_t *dst, const uint16_t *src, uint32_t words) {
  if (dst == 0 || src == 0) {
    return;
  }
  for (uint32_t i = 0; i < words; i++) {
    dst[i] = src[i];
  }
}

static void identity_map_buffer(void *ptr, uint32_t len) {
  uint32_t base = ((uint32_t)ptr) & 0xFFFFF000U;
  uint32_t end = (((uint32_t)ptr) + len + 0xFFFU) & 0xFFFFF000U;
  for (uint32_t a = base; a < end; a += 4096U) {
    map_page(a, a);
  }
}

static int ahci_select_controller(void) {
  int pci_idx;
  PciDevice *dev;

  pci_idx = ahci_find_controller_index();
  if (pci_idx < 0) {
    g_abar = 0;
    g_abar_phys = 0;
    g_pi_mask = 0;
    return 0;
  }

  dev = &pci_found[pci_idx];
  ahci_enable_pci_mmio_and_bm(pci_idx);

  g_abar_phys = pci_read_bar5(pci_idx) & 0xFFFFF000U;
  if (g_abar_phys == 0) {
    g_abar = 0;
    g_pi_mask = 0;
    return 0;
  }

  map_mmio_region(g_abar_phys, 0x2000);
  g_abar = (volatile uint32_t *)g_abar_phys;

  {
    uint32_t ghc = g_abar[0x04 / 4U];
    if ((ghc & AHCI_GHC_AE) == 0) {
      g_abar[0x04 / 4U] = ghc | AHCI_GHC_AE;
    }
  }

  g_pi_mask = g_abar[0x0C / 4U];
  (void)dev;
  return 1;
}

static uint32_t ahci_port_reg_index(uint8_t port, uint32_t byte_off) {
  return (0x100U + ((uint32_t)port * 0x80U) + byte_off) / 4U;
}

static int ahci_wait_port_idle(uint8_t port, uint32_t timeout_loops) {
  while (timeout_loops--) {
    uint32_t cmd = g_abar[ahci_port_reg_index(port, 0x18U)];
    if ((cmd & (HBA_PxCMD_CR | HBA_PxCMD_FR)) == 0) {
      return 1;
    }
  }
  return 0;
}

static int ahci_wait_port_ready(uint8_t port, uint32_t timeout_loops) {
  while (timeout_loops--) {
    uint32_t ssts = g_abar[ahci_port_reg_index(port, 0x28U)];
    uint8_t det = (uint8_t)(ssts & 0x0F);
    uint8_t ipm = (uint8_t)((ssts >> 8) & 0x0F);
    if (det == 0x03 && ipm == 0x01) {
      return 1;
    }
  }
  return 0;
}

static int ahci_stop_port_engine(uint8_t port) {
  uint32_t cmd = g_abar[ahci_port_reg_index(port, 0x18U)];
  cmd &= ~(HBA_PxCMD_ST | HBA_PxCMD_FRE);
  g_abar[ahci_port_reg_index(port, 0x18U)] = cmd;
  return ahci_wait_port_idle(port, 300000U);
}

static void ahci_start_port_engine(uint8_t port) {
  uint32_t cmd = g_abar[ahci_port_reg_index(port, 0x18U)];
  cmd |= HBA_PxCMD_FRE;
  g_abar[ahci_port_reg_index(port, 0x18U)] = cmd;
  cmd |= HBA_PxCMD_ST;
  g_abar[ahci_port_reg_index(port, 0x18U)] = cmd;
}

static void ahci_fill_port_report(uint8_t port, AhciPortReport *pr) {
  uint32_t ssts;
  uint32_t tfd;

  if (pr == 0) {
    return;
  }

  zero_bytes(pr, (uint32_t)sizeof(*pr));
  pr->port_no = port;
  if ((g_pi_mask & (1U << port)) == 0) {
    return;
  }

  pr->implemented = 1;
  pr->sig = g_abar[ahci_port_reg_index(port, 0x24U)];
  pr->sata = (pr->sig == SATA_SIG_ATA);
  tfd = g_abar[ahci_port_reg_index(port, 0x20U)];
  pr->tfd = tfd;

  ssts = g_abar[ahci_port_reg_index(port, 0x28U)];
  pr->ssts = ssts;
  pr->det = (uint8_t)(ssts & 0x0F);
  pr->ipm = (uint8_t)((ssts >> 8) & 0x0F);

  if (pr->det == 0x03 && pr->ipm == 0x01 &&
      (tfd & (ATA_TFD_BSY | ATA_TFD_DRQ)) == 0) {
    pr->ready = 1;
  }
}

static int ahci_prepare_io_structures(uint8_t port) {
  if (g_abar == 0) {
    return 0;
  }

  if (!ahci_stop_port_engine(port)) {
    return 0;
  }

  identity_map_buffer(g_cmd_list, (uint32_t)sizeof(g_cmd_list));
  identity_map_buffer(g_rx_fis, (uint32_t)sizeof(g_rx_fis));
  identity_map_buffer(&g_cmd_table, (uint32_t)sizeof(g_cmd_table));
  identity_map_buffer(g_identify_buf, (uint32_t)sizeof(g_identify_buf));
  identity_map_buffer(g_read_buf, (uint32_t)sizeof(g_read_buf));

  zero_bytes(g_cmd_list, (uint32_t)sizeof(g_cmd_list));
  zero_bytes(g_rx_fis, (uint32_t)sizeof(g_rx_fis));
  zero_bytes(&g_cmd_table, (uint32_t)sizeof(g_cmd_table));
  zero_bytes(g_identify_buf, (uint32_t)sizeof(g_identify_buf));
  zero_bytes(g_read_buf, (uint32_t)sizeof(g_read_buf));

  g_abar[ahci_port_reg_index(port, 0x00U)] = (uint32_t)g_cmd_list;
  g_abar[ahci_port_reg_index(port, 0x04U)] = 0;
  g_abar[ahci_port_reg_index(port, 0x08U)] = (uint32_t)g_rx_fis;
  g_abar[ahci_port_reg_index(port, 0x0CU)] = 0;

  g_abar[ahci_port_reg_index(port, 0x10U)] = 0xFFFFFFFFU;
  g_abar[ahci_port_reg_index(port, 0x30U)] = 0xFFFFFFFFU;

  ahci_start_port_engine(port);
  return 1;
}

static void ata_string_from_words(const uint16_t *id, int word_start, int words,
                                  char *out, int out_len) {
  int o = 0;
  if (out == 0 || out_len <= 0) {
    return;
  }
  for (int i = 0; i < words; i++) {
    uint16_t w = id[word_start + i];
    char hi = (char)((w >> 8) & 0xFF);
    char lo = (char)(w & 0xFF);
    if (o < out_len - 1) {
      out[o++] = hi;
    }
    if (o < out_len - 1) {
      out[o++] = lo;
    }
  }
  while (o > 0 && out[o - 1] == ' ') {
    o--;
  }
  out[o] = '\0';
}

int ahci_probe_and_enumerate(AhciProbeReport *out_report) {
  int pci_idx;
  PciDevice *dev;

  ahci_zero_report(out_report);
  if (out_report == 0) {
    return 0;
  }

  pci_idx = ahci_find_controller_index();
  if (pci_idx < 0) {
    return 0;
  }

  dev = &pci_found[pci_idx];
  out_report->controller_found = 1;
  out_report->bus = dev->bus;
  out_report->slot = dev->slot;
  out_report->function = dev->function;

  if (!ahci_select_controller()) {
    return 1;
  }

  out_report->abar = g_abar_phys;
  out_report->mmio_mapped = 1;
  out_report->cap = g_abar[0x00 / 4U];
  out_report->ghc = g_abar[0x04 / 4U];
  out_report->pi = g_abar[0x0C / 4U];
  out_report->version = g_abar[0x10 / 4U];
  out_report->ahci_enabled = ((out_report->ghc & AHCI_GHC_AE) != 0);

  for (uint8_t port = 0; port < AHCI_MAX_PORTS; port++) {
    AhciPortReport *pr = &out_report->ports[port];
    ahci_fill_port_report(port, pr);
    if (pr->implemented) {
      out_report->implemented_ports++;
      if (pr->ready) {
        out_report->ready_ports++;
      }
    }
  }

  return 1;
}

int ahci_get_runtime_report(AhciProbeReport *out_report) {
  return ahci_probe_and_enumerate(out_report);
}

int ahci_port_reset_and_start(uint8_t port_no, AhciPortReport *out_report) {
  if (out_report != 0) {
    zero_bytes(out_report, (uint32_t)sizeof(*out_report));
    out_report->port_no = port_no;
  }

  if (!ahci_select_controller() || port_no >= AHCI_MAX_PORTS) {
    return 0;
  }
  if ((g_pi_mask & (1U << port_no)) == 0) {
    return 0;
  }

  if (!ahci_stop_port_engine(port_no)) {
    return 0;
  }

  g_abar[ahci_port_reg_index(port_no, 0x10U)] = 0xFFFFFFFFU;
  g_abar[ahci_port_reg_index(port_no, 0x30U)] = 0xFFFFFFFFU;

  g_abar[ahci_port_reg_index(port_no, 0x2CU)] = 0x00000001U;
  ahci_delay(20000U);
  g_abar[ahci_port_reg_index(port_no, 0x2CU)] = 0x00000000U;

  if (!ahci_wait_port_ready(port_no, 300000U)) {
    if (out_report != 0) {
      ahci_fill_port_report(port_no, out_report);
    }
    return 0;
  }

  ahci_start_port_engine(port_no);
  ahci_delay(50000U);

  if (out_report != 0) {
    ahci_fill_port_report(port_no, out_report);
  }
  return 1;
}

int ahci_identify_port(uint8_t port_no, AhciIdentifyInfo *out_info) {
  HbaCmdHeader *hdr;
  FisRegH2D *fis;

  if (out_info == 0) {
    return 0;
  }
  zero_bytes(out_info, (uint32_t)sizeof(*out_info));
  out_info->port_no = port_no;

  if (!ahci_select_controller() || port_no >= AHCI_MAX_PORTS) {
    return 0;
  }
  if ((g_pi_mask & (1U << port_no)) == 0) {
    return 0;
  }

  {
    AhciPortReport pr;
    ahci_fill_port_report(port_no, &pr);
    out_info->signature = pr.sig;
    if (pr.sig == SATA_SIG_ATA) {
      out_info->ata_device = 1;
    }
    if (pr.sig == SATA_SIG_ATAPI) {
      out_info->atapi_device = 1;
      return 0;
    }
    if (!pr.ready || !pr.sata) {
      return 0;
    }
  }

  if (!ahci_prepare_io_structures(port_no)) {
    return 0;
  }

  hdr = &g_cmd_list[0];
  hdr->cfl = (uint8_t)(sizeof(FisRegH2D) / sizeof(uint32_t));
  hdr->pm_flags = 0;
  hdr->prdtl = 1;
  hdr->prdbc = 0;
  hdr->ctba = (uint32_t)&g_cmd_table;
  hdr->ctbau = 0;

  g_cmd_table.prdt[0].dba = (uint32_t)g_identify_buf;
  g_cmd_table.prdt[0].dbau = 0;
  g_cmd_table.prdt[0].reserved = 0;
  g_cmd_table.prdt[0].dbc_i = 511U;

  fis = (FisRegH2D *)&g_cmd_table.cfis[0];
  zero_bytes(fis, (uint32_t)sizeof(FisRegH2D));
  fis->fis_type = FIS_TYPE_REG_H2D;
  fis->pmport_c = (1U << 7);
  fis->command = ATA_CMD_IDENTIFY;
  fis->device = 0;

  {
    uint32_t timeout = 500000U;
    while (timeout--) {
      uint32_t tfd = g_abar[ahci_port_reg_index(port_no, 0x20U)];
      if ((tfd & (ATA_TFD_BSY | ATA_TFD_DRQ)) == 0) {
        break;
      }
    }
  }

  g_abar[ahci_port_reg_index(port_no, 0x10U)] = 0xFFFFFFFFU;
  g_abar[ahci_port_reg_index(port_no, 0x30U)] = 0xFFFFFFFFU;
  g_abar[ahci_port_reg_index(port_no, 0x38U)] = 1U;

  {
    uint32_t timeout = 1000000U;
    while (timeout--) {
      uint32_t ci = g_abar[ahci_port_reg_index(port_no, 0x38U)];
      uint32_t is = g_abar[ahci_port_reg_index(port_no, 0x10U)];
      if ((is & HBA_PxIS_TFES) != 0) {
        return 0;
      }
      if ((ci & 1U) == 0) {
        break;
      }
    }
  }

  out_info->lba28_sectors =
      (uint32_t)g_identify_buf[60] | ((uint32_t)g_identify_buf[61] << 16);
  out_info->lba48_sectors_low =
      (uint32_t)g_identify_buf[100] | ((uint32_t)g_identify_buf[101] << 16);
  out_info->lba48_sectors_high =
      (uint32_t)g_identify_buf[102] | ((uint32_t)g_identify_buf[103] << 16);

  out_info->smart_supported = (uint8_t)((g_identify_buf[82] & 0x0001U) != 0U);
  out_info->smart_enabled = (uint8_t)((g_identify_buf[85] & 0x0001U) != 0U);

  ata_string_from_words(g_identify_buf, 10, 10, out_info->serial,
                        (int)sizeof(out_info->serial));
  ata_string_from_words(g_identify_buf, 23, 4, out_info->firmware,
                        (int)sizeof(out_info->firmware));
  ata_string_from_words(g_identify_buf, 27, 20, out_info->model,
                        (int)sizeof(out_info->model));

  out_info->success = 1;
  return 1;
}

int ahci_identify_first_ready(AhciIdentifyInfo *out_info) {
  AhciProbeReport report;

  if (out_info == 0) {
    return 0;
  }

  if (!ahci_probe_and_enumerate(&report)) {
    return 0;
  }

  for (uint8_t port = 0; port < AHCI_MAX_PORTS; port++) {
    if (!report.ports[port].implemented || !report.ports[port].ready ||
        !report.ports[port].sata) {
      continue;
    }
    if (ahci_identify_port(port, out_info)) {
      return 1;
    }
  }
  return 0;
}

int ahci_read_sector(uint8_t port_no, uint32_t lba, uint16_t *out_sector) {
  HbaCmdHeader *hdr;
  FisRegH2D *fis;

  if (out_sector == 0) {
    return AHCI_READ_ERR_NO_READY_PORT;
  }

  if (!ahci_select_controller() || port_no >= AHCI_MAX_PORTS) {
    return AHCI_READ_ERR_NO_CONTROLLER;
  }
  if ((g_pi_mask & (1U << port_no)) == 0) {
    return AHCI_READ_ERR_NO_READY_PORT;
  }

  {
    AhciPortReport pr;
    ahci_fill_port_report(port_no, &pr);
    if (!pr.ready || !pr.sata) {
      return AHCI_READ_ERR_NO_READY_PORT;
    }
  }

  if (!ahci_prepare_io_structures(port_no)) {
    return AHCI_READ_ERR_TIMEOUT;
  }

  hdr = &g_cmd_list[0];
  hdr->cfl = (uint8_t)(sizeof(FisRegH2D) / sizeof(uint32_t));
  hdr->pm_flags = 0;
  hdr->prdtl = 1;
  hdr->prdbc = 0;
  hdr->ctba = (uint32_t)&g_cmd_table;
  hdr->ctbau = 0;

  g_cmd_table.prdt[0].dba = (uint32_t)g_read_buf;
  g_cmd_table.prdt[0].dbau = 0;
  g_cmd_table.prdt[0].reserved = 0;
  g_cmd_table.prdt[0].dbc_i = 511U;

  fis = (FisRegH2D *)&g_cmd_table.cfis[0];
  zero_bytes(fis, (uint32_t)sizeof(FisRegH2D));
  fis->fis_type = FIS_TYPE_REG_H2D;
  fis->pmport_c = (1U << 7);
  fis->command = ATA_CMD_READ_DMA_EXT;
  fis->device = 0x40;
  fis->lba0 = (uint8_t)(lba & 0xFFU);
  fis->lba1 = (uint8_t)((lba >> 8) & 0xFFU);
  fis->lba2 = (uint8_t)((lba >> 16) & 0xFFU);
  fis->lba3 = (uint8_t)((lba >> 24) & 0xFFU);
  fis->lba4 = 0;
  fis->lba5 = 0;
  fis->countl = 1;
  fis->counth = 0;

  {
    uint32_t timeout = 500000U;
    while (timeout--) {
      uint32_t tfd = g_abar[ahci_port_reg_index(port_no, 0x20U)];
      if ((tfd & (ATA_TFD_BSY | ATA_TFD_DRQ)) == 0) {
        break;
      }
    }
    if (timeout == 0) {
      return AHCI_READ_ERR_TIMEOUT;
    }
  }

  g_abar[ahci_port_reg_index(port_no, 0x10U)] = 0xFFFFFFFFU;
  g_abar[ahci_port_reg_index(port_no, 0x30U)] = 0xFFFFFFFFU;
  g_abar[ahci_port_reg_index(port_no, 0x38U)] = 1U;

  {
    uint32_t timeout = 1000000U;
    while (timeout--) {
      uint32_t ci = g_abar[ahci_port_reg_index(port_no, 0x38U)];
      uint32_t is = g_abar[ahci_port_reg_index(port_no, 0x10U)];
      if ((is & HBA_PxIS_TFES) != 0) {
        return AHCI_READ_ERR_TFES;
      }
      if ((ci & 1U) == 0) {
        break;
      }
    }
    if (timeout == 0) {
      return AHCI_READ_ERR_TIMEOUT;
    }
  }

  copy_words(out_sector, g_read_buf, 256U);
  return AHCI_READ_OK;
}

int ahci_read_first_ready_sector(uint32_t lba, uint8_t *out_port,
                                 uint16_t *out_sector) {
  AhciProbeReport report;

  if (out_sector == 0) {
    return AHCI_READ_ERR_NO_READY_PORT;
  }

  if (!ahci_probe_and_enumerate(&report)) {
    return AHCI_READ_ERR_NO_CONTROLLER;
  }

  for (uint8_t port = 0; port < AHCI_MAX_PORTS; port++) {
    if (!report.ports[port].implemented || !report.ports[port].ready ||
        !report.ports[port].sata) {
      continue;
    }
    if (out_port != 0) {
      *out_port = port;
    }
    return ahci_read_sector(port, lba, out_sector);
  }

  return AHCI_READ_ERR_NO_READY_PORT;
}

int ahci_write_sector(uint8_t port_no, uint32_t lba, const uint16_t *in_sector) {
  HbaCmdHeader *hdr;
  FisRegH2D *fis;

  if (in_sector == 0) {
    return AHCI_READ_ERR_NO_READY_PORT;
  }

  if (!ahci_select_controller() || port_no >= AHCI_MAX_PORTS) {
    return AHCI_READ_ERR_NO_CONTROLLER;
  }
  if ((g_pi_mask & (1U << port_no)) == 0) {
    return AHCI_READ_ERR_NO_READY_PORT;
  }

  {
    AhciPortReport pr;
    ahci_fill_port_report(port_no, &pr);
    if (!pr.ready || !pr.sata) {
      return AHCI_READ_ERR_NO_READY_PORT;
    }
  }

  if (!ahci_prepare_io_structures(port_no)) {
    return AHCI_READ_ERR_TIMEOUT;
  }

  copy_words(g_read_buf, in_sector, 256U);

  hdr = &g_cmd_list[0];
  hdr->cfl = (uint8_t)(sizeof(FisRegH2D) / sizeof(uint32_t));
  hdr->pm_flags = (1U << 6); /* Write */
  hdr->prdtl = 1;
  hdr->prdbc = 0;
  hdr->ctba = (uint32_t)&g_cmd_table;
  hdr->ctbau = 0;

  g_cmd_table.prdt[0].dba = (uint32_t)g_read_buf;
  g_cmd_table.prdt[0].dbau = 0;
  g_cmd_table.prdt[0].reserved = 0;
  g_cmd_table.prdt[0].dbc_i = 511U;

  fis = (FisRegH2D *)&g_cmd_table.cfis[0];
  zero_bytes(fis, (uint32_t)sizeof(FisRegH2D));
  fis->fis_type = FIS_TYPE_REG_H2D;
  fis->pmport_c = (1U << 7);
  fis->command = ATA_CMD_WRITE_DMA_EXT;
  fis->device = 0x40;
  fis->lba0 = (uint8_t)(lba & 0xFFU);
  fis->lba1 = (uint8_t)((lba >> 8) & 0xFFU);
  fis->lba2 = (uint8_t)((lba >> 16) & 0xFFU);
  fis->lba3 = (uint8_t)((lba >> 24) & 0xFFU);
  fis->lba4 = 0;
  fis->lba5 = 0;
  fis->countl = 1;
  fis->counth = 0;

  {
    uint32_t timeout = 500000U;
    while (timeout--) {
      uint32_t tfd = g_abar[ahci_port_reg_index(port_no, 0x20U)];
      if ((tfd & (ATA_TFD_BSY | ATA_TFD_DRQ)) == 0) {
        break;
      }
    }
    if (timeout == 0) {
      return AHCI_READ_ERR_TIMEOUT;
    }
  }

  g_abar[ahci_port_reg_index(port_no, 0x10U)] = 0xFFFFFFFFU;
  g_abar[ahci_port_reg_index(port_no, 0x30U)] = 0xFFFFFFFFU;
  g_abar[ahci_port_reg_index(port_no, 0x38U)] = 1U;

  {
    uint32_t timeout = 1000000U;
    while (timeout--) {
      uint32_t ci = g_abar[ahci_port_reg_index(port_no, 0x38U)];
      uint32_t is = g_abar[ahci_port_reg_index(port_no, 0x10U)];
      if ((is & HBA_PxIS_TFES) != 0) {
        return AHCI_READ_ERR_TFES;
      }
      if ((ci & 1U) == 0) {
        break;
      }
    }
    if (timeout == 0) {
      return AHCI_READ_ERR_TIMEOUT;
    }
  }

  return AHCI_READ_OK;
}

int ahci_write_first_ready_sector(uint32_t lba, uint8_t *out_port,
                                  const uint16_t *in_sector) {
  AhciProbeReport report;

  if (in_sector == 0) {
    return AHCI_READ_ERR_NO_READY_PORT;
  }

  if (!ahci_probe_and_enumerate(&report)) {
    return AHCI_READ_ERR_NO_CONTROLLER;
  }

  for (uint8_t port = 0; port < AHCI_MAX_PORTS; port++) {
    if (!report.ports[port].implemented || !report.ports[port].ready ||
        !report.ports[port].sata) {
      continue;
    }
    if (out_port != 0) {
      *out_port = port;
    }
    return ahci_write_sector(port, lba, in_sector);
  }

  return AHCI_READ_ERR_NO_READY_PORT;
}
