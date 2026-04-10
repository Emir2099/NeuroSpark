#ifndef DISK_H
#define DISK_H

/* Driver and Platform Completion - Storage Controllers
 *
 * PRIMARY (In-Use): ATA PIO via legacy ports (0x1F0-0x1F7, 0x3F6)
 *   - Compatible with all QEMU variants
 *   - Used by VFS and storage_manager via disk_read/write_sector
 *
 * SECONDARY (Detection Ready, No Driver Yet):
 *   - AHCI (class 0x01, subclass 0x06): BAR5 MMIO
 *   - NVMe (class 0x01, subclass 0x08): BAR0 MMIO
 *
 * Architecture: All I/O uses disk_read/write_sector abstraction.
 * Future AHCI/NVMe drivers will use same interface without changing VFS.
 */


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

typedef enum {
    DISK_BACKEND_AUTO = 0,
    DISK_BACKEND_ATA = 1,
    DISK_BACKEND_AHCI = 2,
} DiskBackendPreference;

typedef enum {
    DISK_IO_OK = 0,
    DISK_IO_ERR_NO_DEVICE = -1,
    DISK_IO_ERR_NO_READY_PORT = -2,
    DISK_IO_ERR_TIMEOUT = -3,
    DISK_IO_ERR_TFES = -4,
    DISK_IO_ERR_UNSUPPORTED = -5,
} DiskIoResult;

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

typedef struct {
    uint32_t uncached_reads;
    uint32_t uncached_writes;
    uint32_t cached_reads;
    uint32_t cached_writes;
} DiskIoStats;

typedef struct {
    uint32_t lba;
    uint16_t *buffer;
    uint8_t write;
    int result;
} DiskBatchRequest;

typedef struct {
    uint32_t batches;
    uint32_t requests;
    uint32_t reordered_requests;
    uint32_t max_batch_depth;
    uint32_t last_lba;
} DiskSchedulerStats;

typedef struct {
    uint32_t total_sectors_low;
    uint32_t total_sectors_high;
    uint32_t sector_size;
    uint32_t backend;
    char model[41];
} DiskGeometryInfo;

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
} DiskHealthInfo;

// Disk driver functions
int  ata_detect_disk(void);
void ata_wait_ready(void);
void disk_write_sector(uint32_t lba, uint16_t *buffer);
void disk_read_sector(uint32_t lba, uint16_t *buffer);
int  disk_read_sector_backend(uint32_t lba, uint16_t *buffer, uint8_t backend);
int  disk_read_sector_ex(uint32_t lba, uint16_t *buffer);
int  disk_read_sector_uncached(uint32_t lba, uint16_t *buffer);
void disk_write_sector_uncached(uint32_t lba, const uint16_t *buffer);
void disk_set_preferred_backend(uint8_t backend);
uint8_t disk_get_preferred_backend(void);
const char *disk_read_error_string(int code);
int  find_free_slot(FileEntry *dir);
void disk_get_io_stats(DiskIoStats *out_stats);
void disk_reset_io_stats(void);
int  disk_process_batch(DiskBatchRequest *reqs, uint32_t count);
void disk_get_scheduler_stats(DiskSchedulerStats *out_stats);
void disk_reset_scheduler_stats(void);
int  disk_get_geometry(DiskGeometryInfo *out_info);
int  disk_get_health(DiskHealthInfo *out_info);

#endif
