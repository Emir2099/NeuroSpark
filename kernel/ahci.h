#ifndef AHCI_H
#define AHCI_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define AHCI_MAX_PORTS 32

#define AHCI_READ_OK 0
#define AHCI_READ_ERR_NO_CONTROLLER -1
#define AHCI_READ_ERR_NO_READY_PORT -2
#define AHCI_READ_ERR_TIMEOUT -3
#define AHCI_READ_ERR_TFES -4

typedef struct {
  uint8_t port_no;
  uint8_t implemented;
  uint8_t det;
  uint8_t ipm;
  uint8_t ready;
  uint8_t sata;
  uint32_t ssts;
  uint32_t tfd;
  uint32_t sig;
} AhciPortReport;

typedef struct {
  uint8_t success;
  uint8_t port_no;
  uint8_t ata_device;
  uint8_t atapi_device;
  uint32_t signature;
  uint32_t lba28_sectors;
  uint32_t lba48_sectors_low;
  uint32_t lba48_sectors_high;
  char serial[21];
  char firmware[9];
  char model[41];
} AhciIdentifyInfo;

typedef struct {
  uint8_t controller_found;
  uint8_t mmio_mapped;
  uint8_t ahci_enabled;
  uint8_t bus;
  uint8_t slot;
  uint8_t function;
  uint32_t abar;
  uint32_t cap;
  uint32_t ghc;
  uint32_t pi;
  uint32_t version;
  uint8_t implemented_ports;
  uint8_t ready_ports;
  AhciPortReport ports[AHCI_MAX_PORTS];
} AhciProbeReport;

int ahci_probe_and_enumerate(AhciProbeReport *out_report);
int ahci_get_runtime_report(AhciProbeReport *out_report);
int ahci_port_reset_and_start(uint8_t port_no, AhciPortReport *out_report);
int ahci_identify_port(uint8_t port_no, AhciIdentifyInfo *out_info);
int ahci_identify_first_ready(AhciIdentifyInfo *out_info);
int ahci_read_sector(uint8_t port_no, uint32_t lba, uint16_t *out_sector);
int ahci_read_first_ready_sector(uint32_t lba, uint8_t *out_port,
                                 uint16_t *out_sector);

#endif
