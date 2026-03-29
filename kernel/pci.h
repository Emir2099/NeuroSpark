#ifndef PCI_H
#define PCI_H

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

/* Shared device record filled during pci_scan_all() */
typedef struct {
  uint16_t vendor;
  uint16_t device_id;
  uint8_t class_code;
  uint8_t subclass;
  uint8_t bus;
  uint8_t slot;
  uint8_t function; /* PCI function number (for multi-function devices) */
} PciDevice;

#define PCI_MAX_DEVICES 16
extern PciDevice pci_found[PCI_MAX_DEVICES];
extern int pci_found_count;

/* Functions */
void pci_scan_all(void);
uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func,
                         uint8_t offset);
void pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset,
                      uint32_t value);
uint32_t pci_read_bar(int device_index, uint8_t bar_index);

/* BAR5 (AHCI ABAR) reading and storage controller discovery */
uint32_t pci_read_bar5(int device_index);
int pci_find_storage_controller(void);
int pci_prepare_storage_mmio(uint32_t *mmio_base, uint32_t *mmio_size,
                             uint8_t *storage_subclass);
int pci_find_network_controller(void);

#endif
