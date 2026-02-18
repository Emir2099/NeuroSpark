#ifndef DISK_H
#define DISK_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

// ===== DISK DRIVER CONFIGURATION =====
#define DISK_DATA_OFFSET 66   // Start saving neural data at LBA 66 (after 1 boot + 32 kernel sectors + gap)

// ATA I/O Port Addresses
#define PORT_DATA      0x1F0
#define PORT_SECCOUNT  0x1F2
#define PORT_LBA_LOW   0x1F3
#define PORT_LBA_MID   0x1F4
#define PORT_LBA_HIGH  0x1F5
#define PORT_DEVICE    0x1F6
#define PORT_COMMAND   0x1F7
#define PORT_CONTROL   0x3F6  // Device Control Register (alternate status / control)

// ATA Status Register Bits
#define STATUS_BSY     0x80
#define STATUS_RDY     0x40

// ATA Commands
#define CMD_READ       0x20
#define CMD_WRITE      0x30

// Global flag: is ATA disk controller present?
extern volatile int ata_disk_available;

// ===== TINY FILE SYSTEM (TFS) =====
#define TFS_DIR_LBA    150   // Root directory stored at LBA 150
#define TFS_DATA_START 200   // Neural snapshots start at LBA 200+
#define TFS_MAX_FILES  25    // One 512-byte sector holds 25 entries (20 bytes each)

typedef struct {
    char name[12];     // Name of the neural snapshot
    uint32_t lba;      // The physical sector on disk
    uint32_t size;     // Size in bytes
    uint32_t flags;    // 1 = Active, 0 = Empty
} FileEntry;

// Disk driver functions
int  ata_detect_disk(void);
void ata_wait_ready(void);
void disk_write_sector(uint32_t lba, uint16_t *buffer);
void disk_read_sector(uint32_t lba, uint16_t *buffer);
int  find_free_slot(FileEntry *dir);

#endif
