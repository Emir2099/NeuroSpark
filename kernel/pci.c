
#include "pci.h"

/* Storage Controller Detection & MMIO Prep
 * 
 * Class 0x01 storage controllers detected:
 *   Subclass 0x06 (AHCI): BAR5 MMIO, 0x2000 bytes
 *   Subclass 0x08 (NVMe): BAR0 MMIO, 0x4000 bytes
 *
 * Current: Legacy ATA PIO. Both AHCI and NVMe detected for future drivers.
 */

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

/* ============================================================
 * Port I/O (32-bit)
 * ============================================================ */
static inline void outl(uint16_t port, uint32_t val) {
  __asm__ volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint32_t inl(uint16_t port) {
  uint32_t ret;
  __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

/* ============================================================
 * Shared PCI device list (filled during scan, NEVER printed at scan time)
 * Printing happens post-boot via pci_print_results() called from shell/task.
 * ============================================================ */
PciDevice pci_found[PCI_MAX_DEVICES];
int pci_found_count = 0;

/* ============================================================
 * PCI config read
 * ============================================================ */
uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func,
                         uint8_t offset) {
  uint32_t address =
      (uint32_t)(((uint32_t)bus << 16) | ((uint32_t)slot << 11) |
                 ((uint32_t)func << 8) | (offset & 0xFC) | 0x80000000);
  outl(PCI_CONFIG_ADDRESS, address);
  return inl(PCI_CONFIG_DATA);
}

void pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset,
                      uint32_t value) {
  uint32_t address =
      (uint32_t)(((uint32_t)bus << 16) | ((uint32_t)slot << 11) |
                 ((uint32_t)func << 8) | (offset & 0xFC) | 0x80000000);
  outl(PCI_CONFIG_ADDRESS, address);
  outl(PCI_CONFIG_DATA, value);
}

uint32_t pci_read_bar(int device_index, uint8_t bar_index) {
  if (device_index < 0 || device_index >= pci_found_count || bar_index > 5)
    return 0;
  PciDevice *d = &pci_found[device_index];
  uint32_t bar = pci_read_config(d->bus, d->slot, d->function,
                                 (uint8_t)(0x10 + (bar_index * 4)));
  return bar & 0xFFFFFFF0;
}

/* ============================================================
 * pci_check_function – SILENT: store only, no gprint here.
 * ============================================================ */
void pci_check_function(uint8_t bus, uint8_t device, uint8_t function) {
  uint32_t vendor_device = pci_read_config(bus, device, function, 0x00);
  uint16_t vendor_id = (uint16_t)(vendor_device & 0xFFFF);

  if (vendor_id == 0xFFFF)
    return; /* No device */

  uint16_t device_id = (uint16_t)(vendor_device >> 16);
  uint32_t class_reg = pci_read_config(bus, device, function, 0x08);
  uint8_t class_code = (uint8_t)((class_reg >> 24) & 0xFF);
  uint8_t subclass = (uint8_t)((class_reg >> 16) & 0xFF);

  if (pci_found_count < PCI_MAX_DEVICES) {
    pci_found[pci_found_count].vendor = vendor_id;
    pci_found[pci_found_count].device_id = device_id;
    pci_found[pci_found_count].class_code = class_code;
    pci_found[pci_found_count].subclass = subclass;
    pci_found[pci_found_count].bus = bus;
    pci_found[pci_found_count].slot = device;
    pci_found[pci_found_count].function = function;
    pci_found_count++;
  }
}

/* ============================================================
 * pci_scan_all – just fills pci_found[], never touches gprint
 * ============================================================ */
void pci_scan_all() {
  pci_found_count = 0;
  for (uint16_t bus = 0; bus < 256; bus++) {
    for (uint8_t slot = 0; slot < 32; slot++) {
      pci_check_function((uint8_t)bus, slot, 0);
    }
  }
}

/* ============================================================
 * pci_read_bar5 – Read BAR5 (AHCI ABAR) for a discovered device.
 * BAR5 is at PCI config offset 0x24.
 * Returns the 32-bit base address (memory-mapped).
 * ============================================================ */
uint32_t pci_read_bar5(int device_index) {
  return pci_read_bar(device_index, 5);
}

/* ============================================================
 * pci_find_storage_controller – locate AHCI (01/06) or NVMe (01/08)
 * Returns the index into pci_found[], or -1 if none.
 * ============================================================ */
int pci_find_storage_controller(void) {
  for (int i = 0; i < pci_found_count; i++) {
    if (pci_found[i].class_code == 0x01 &&
        (pci_found[i].subclass == 0x06 || pci_found[i].subclass == 0x08)) {
      return i;
    }
  }
  return -1;
}

int pci_find_network_controller(void) {
  for (int i = 0; i < pci_found_count; i++) {
    if (pci_found[i].class_code == 0x02) {
      return i;
    }
  }
  return -1;
}

int pci_prepare_storage_mmio(uint32_t *mmio_base, uint32_t *mmio_size,
                             uint8_t *storage_subclass) {
  int idx = pci_find_storage_controller();
  if (idx < 0 || mmio_base == 0 || mmio_size == 0 || storage_subclass == 0)
    return 0;

  *storage_subclass = pci_found[idx].subclass;
  if (pci_found[idx].subclass == 0x06) {
    *mmio_base = pci_read_bar5(idx); /* AHCI ABAR */
    *mmio_size = 0x2000;
  } else if (pci_found[idx].subclass == 0x08) {
    *mmio_base = pci_read_bar(idx, 0); /* NVMe BAR0 */
    *mmio_size = 0x4000;
  } else {
    return 0;
  }
  return *mmio_base != 0;
}

/* ============================================================
 * pci_print_results – call this AFTER init_idt() and init_graphics()
 * Prints all found devices via gprint safely.
 * ============================================================ */
extern void gprint(char *str, unsigned int color);
extern void gprint_hex(unsigned int val, int digits, unsigned int color);

void pci_print_results() {
  gprint("--- PCI DEVICES ---\n", 0x00FFFF);
  for (int i = 0; i < pci_found_count; i++) {
    gprint("B:", 0x00FFFF);
    gprint_hex(pci_found[i].bus, 2, 0xFFFFFF);
    gprint(" D:", 0x00FFFF);
    gprint_hex(pci_found[i].slot, 2, 0xFFFFFF);
    gprint(" VEN:", 0x00FFFF);
    gprint_hex(pci_found[i].vendor, 4, 0xFFFF00);
    gprint(" DEV:", 0x00FFFF);
    gprint_hex(pci_found[i].device_id, 4, 0xFFFF00);
    gprint(" CLS:", 0x00FFFF);
    gprint_hex(pci_found[i].class_code, 2, 0xFF8800);
    gprint("/", 0x888888);
    gprint_hex(pci_found[i].subclass, 2, 0xFF8800);

    if (pci_found[i].class_code == 0x01 && pci_found[i].subclass == 0x08) {
      gprint(" <NVMe>", 0x00FF88);
      gprint(" BAR0:", 0xAAAAAA);
      gprint_hex(pci_read_bar(i, 0), 8, 0x00FF88);
      gprint(" BAR5:", 0xAAAAAA);
      gprint_hex(pci_read_bar5(i), 8, 0x00FF88);
    } else if (pci_found[i].class_code == 0x01 &&
               pci_found[i].subclass == 0x06) {
      gprint(" <AHCI>", 0x00FF88);
      gprint(" BAR5:", 0xAAAAAA);
      gprint_hex(pci_read_bar5(i), 8, 0x00FF88);
    } else if (pci_found[i].class_code == 0x02) {
      gprint(" <NET>", 0x88FFFF);
    } else if (pci_found[i].class_code == 0x03) {
      gprint(" <VGA>", 0x8888FF);
    } else if (pci_found[i].class_code == 0x0C &&
               pci_found[i].subclass == 0x03) {
      gprint(" <USB>", 0xFF88FF);
    }
    gprint("\n", 0);
  }
  if (pci_found_count == 0) {
    gprint("No PCI devices found.\n", 0xFF4444);
  }
}
