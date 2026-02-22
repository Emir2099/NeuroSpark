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
} PciDevice;

#define PCI_MAX_DEVICES 16
extern PciDevice pci_found[PCI_MAX_DEVICES];
extern int pci_found_count;

/* Functions */
void pci_scan_all(void);
uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func,
                         uint8_t offset);

#endif
