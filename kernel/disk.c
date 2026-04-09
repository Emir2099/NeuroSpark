#include "disk.h"
#include "ahci.h"
#include "page_cache.h"

// ============================================================================
// DISK DRIVER FUNCTIONS (ATA PIO Mode)
// ============================================================================

/* Low-level I/O helpers */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

// Global flag: is ATA disk controller present?
volatile int ata_disk_available = 0;
static uint8_t disk_preferred_backend = DISK_BACKEND_AUTO;
static DiskIoStats disk_io_stats = {0, 0, 0, 0};
static uint32_t disk_read_errors = 0;
static uint32_t disk_write_errors = 0;
static int disk_last_error = DISK_IO_OK;

#define DISK_READ_RETRY_COUNT 3

static void copy_text(char *dst, const char *src, uint32_t cap) {
    uint32_t i = 0;
    if (dst == 0 || src == 0 || cap == 0) {
        return;
    }
    while (i + 1 < cap && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static int ahci_reset_first_available_port(void) {
    AhciProbeReport report;

    if (!ahci_get_runtime_report(&report) || !report.controller_found ||
        report.implemented_ports == 0) {
        return 0;
    }

    for (uint8_t port = 0; port < AHCI_MAX_PORTS; port++) {
        AhciPortReport ignored;
        if (!report.ports[port].implemented) {
            continue;
        }
        if (ahci_port_reset_and_start(port, &ignored)) {
            return 1;
        }
    }
    return 0;
}

static void disk_note_error(int rc, int is_write) {
    if (rc == DISK_IO_OK) {
        return;
    }
    disk_last_error = rc;
    if (is_write) {
        disk_write_errors++;
    } else {
        disk_read_errors++;
    }
}

void disk_get_io_stats(DiskIoStats *out_stats) {
    if (out_stats == 0) {
        return;
    }
    *out_stats = disk_io_stats;
}

void disk_reset_io_stats(void) {
    disk_io_stats.uncached_reads = 0;
    disk_io_stats.uncached_writes = 0;
    disk_io_stats.cached_reads = 0;
    disk_io_stats.cached_writes = 0;
    disk_read_errors = 0;
    disk_write_errors = 0;
    disk_last_error = DISK_IO_OK;
}

void disk_set_preferred_backend(uint8_t backend) {
    if (backend != DISK_BACKEND_AUTO && backend != DISK_BACKEND_ATA &&
        backend != DISK_BACKEND_AHCI) {
        backend = DISK_BACKEND_AUTO;
    }
    disk_preferred_backend = backend;
}

uint8_t disk_get_preferred_backend(void) {
    return disk_preferred_backend;
}

const char *disk_read_error_string(int code) {
    switch (code) {
    case DISK_IO_OK:
        return "OK";
    case DISK_IO_ERR_NO_DEVICE:
        return "NO_DEVICE";
    case DISK_IO_ERR_NO_READY_PORT:
        return "NO_READY_PORT";
    case DISK_IO_ERR_TIMEOUT:
        return "TIMEOUT";
    case DISK_IO_ERR_TFES:
        return "TFES";
    case DISK_IO_ERR_UNSUPPORTED:
        return "UNSUPPORTED";
    default:
        return "ERR";
    }
}

/*
 * ata_detect_disk() - Check if an ATA disk controller is present
 * Returns 1 if disk is available, 0 if not (floating bus = 0xFF)
 */
int ata_detect_disk(void) {
    // 1. Disable IDE interrupts at the controller level (nIEN = bit 1)
    //    This prevents IRQ 14 from firing and causing a triple fault
    outb(PORT_CONTROL, 0x02);  // Set nIEN bit, clear SRST
    
    // 2. Check for floating bus (no controller at all)
    uint8_t status = inb(PORT_COMMAND);
    if (status == 0xFF) return 0;
    
    // 3. Wait for BSY to clear with timeout
    for (int i = 0; i < 200000; i++) {
        status = inb(PORT_COMMAND);
        if (!(status & STATUS_BSY)) {
            // 4. Verify the controller responds sensibly
            //    Write a test value to LBA_LOW and read it back
            outb(PORT_LBA_LOW, 0xAB);
            uint8_t val = inb(PORT_LBA_LOW);
            if (val == 0xAB) {
                // Restore the register
                outb(PORT_LBA_LOW, 0x00);
                return 1;
            }
            return 0;
        }
    }
    return 0;  // Timed out waiting for BSY
}

/*
 * ata_wait_ready() - Wait for disk to be ready
 * Polls the status register until BSY is clear and RDY is set
 */
void ata_wait_ready(void) {
    uint8_t status;
    int timeout = 500000; // Bounded wait to prevent infinite loop
    do {
        status = inb(PORT_COMMAND);
        if (--timeout == 0) return; // Give up after timeout
    } while ((status & STATUS_BSY) || !(status & STATUS_RDY));
}

/*
 * disk_write_sector() - Write 512 bytes (256 words) to disk
 * @lba: Logical Block Address (sector number)
 * @buffer: Pointer to 512 bytes of data
 */
// Wait for DRQ (Data Request) bit to be set after issuing a command
static void ata_wait_drq(void) {
    int timeout = 500000;
    uint8_t status;
    do {
        status = inb(PORT_COMMAND);
        if (--timeout == 0) return;
    } while (!(status & 0x08)); // DRQ = bit 3
}

static int ata_read_sector_ex(uint32_t lba, uint16_t *buffer) {
    int timeout = 500000;
    uint8_t status;

    if (!ata_disk_available || buffer == 0) {
        return DISK_IO_ERR_NO_DEVICE;
    }

    do {
        status = inb(PORT_COMMAND);
        if (status == 0xFF) {
            return DISK_IO_ERR_NO_DEVICE;
        }
        if (!(status & STATUS_BSY)) {
            break;
        }
    } while (--timeout > 0);

    if (timeout <= 0) {
        return DISK_IO_ERR_TIMEOUT;
    }

    outb(PORT_DEVICE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(PORT_SECCOUNT, 1);
    outb(PORT_LBA_LOW,  (uint8_t)(lba & 0xFF));
    outb(PORT_LBA_MID,  (uint8_t)((lba >> 8) & 0xFF));
    outb(PORT_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));

    outb(PORT_COMMAND, CMD_READ);
    do {
        status = inb(PORT_COMMAND);
        if (status & 0x01) {
            return DISK_IO_ERR_NO_DEVICE;
        }
        if (status & 0x08) {
            break;
        }
    } while (--timeout > 0);

    if (timeout <= 0) {
        return DISK_IO_ERR_TIMEOUT;
    }

    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(PORT_DATA);
    }

    return DISK_IO_OK;
}

static int ahci_read_sector_ex(uint32_t lba, uint16_t *buffer) {
    AhciProbeReport report;

    if (buffer == 0) {
        return DISK_IO_ERR_UNSUPPORTED;
    }

    if (!ahci_get_runtime_report(&report) || !report.controller_found ||
        report.ready_ports == 0) {
        return DISK_IO_ERR_NO_READY_PORT;
    }

    for (uint8_t port = 0; port < AHCI_MAX_PORTS; port++) {
        if (!report.ports[port].implemented || !report.ports[port].ready ||
            !report.ports[port].sata) {
            continue;
        }

        for (int attempt = 0; attempt < DISK_READ_RETRY_COUNT; attempt++) {
            int rc = ahci_read_sector(port, lba, buffer);
            if (rc == AHCI_READ_OK) {
                return DISK_IO_OK;
            }
            if (rc == AHCI_READ_ERR_TIMEOUT || rc == AHCI_READ_ERR_TFES) {
                (void)ahci_port_reset_and_start(port, 0);
                continue;
            }
            if (rc == AHCI_READ_ERR_NO_READY_PORT) {
                break;
            }
            return DISK_IO_ERR_NO_DEVICE;
        }
    }

    return DISK_IO_ERR_NO_READY_PORT;
}

static int ahci_write_sector_ex(uint32_t lba, const uint16_t *buffer) {
    AhciProbeReport report;

    if (buffer == 0) {
        return DISK_IO_ERR_UNSUPPORTED;
    }

    if (!ahci_get_runtime_report(&report) || !report.controller_found ||
        report.ready_ports == 0) {
        return DISK_IO_ERR_NO_READY_PORT;
    }

    for (uint8_t port = 0; port < AHCI_MAX_PORTS; port++) {
        if (!report.ports[port].implemented || !report.ports[port].ready ||
            !report.ports[port].sata) {
            continue;
        }

        for (int attempt = 0; attempt < DISK_READ_RETRY_COUNT; attempt++) {
            int rc = ahci_write_sector(port, lba, buffer);
            if (rc == AHCI_READ_OK) {
                return DISK_IO_OK;
            }
            if (rc == AHCI_READ_ERR_TIMEOUT || rc == AHCI_READ_ERR_TFES) {
                (void)ahci_port_reset_and_start(port, 0);
                continue;
            }
            if (rc == AHCI_READ_ERR_NO_READY_PORT) {
                break;
            }
            return DISK_IO_ERR_NO_DEVICE;
        }
    }

    return DISK_IO_ERR_NO_READY_PORT;
}

void disk_write_sector_uncached(uint32_t lba, const uint16_t *buffer) {
    int rc = DISK_IO_OK;

    disk_io_stats.uncached_writes++;

    if (buffer == 0) {
        disk_note_error(DISK_IO_ERR_UNSUPPORTED, 1);
        return;
    }

    if (disk_preferred_backend == DISK_BACKEND_AHCI) {
        rc = ahci_write_sector_ex(lba, buffer);
        if (rc == DISK_IO_OK) {
            return;
        }
        if (rc == DISK_IO_ERR_TIMEOUT || rc == DISK_IO_ERR_TFES) {
            (void)ahci_reset_first_available_port();
            rc = ahci_write_sector_ex(lba, buffer);
            if (rc == DISK_IO_OK) {
                return;
            }
        }
        if (!ata_disk_available) {
            disk_note_error(rc, 1);
            return;
        }
    }

    if (disk_preferred_backend == DISK_BACKEND_AUTO) {
        rc = ahci_write_sector_ex(lba, buffer);
        if (rc == DISK_IO_OK) {
            return;
        }
        if (rc == DISK_IO_ERR_TIMEOUT || rc == DISK_IO_ERR_TFES) {
            (void)ahci_reset_first_available_port();
            rc = ahci_write_sector_ex(lba, buffer);
            if (rc == DISK_IO_OK) {
                return;
            }
        }
    }

    if (!ata_disk_available) {
        disk_note_error((rc == DISK_IO_OK) ? DISK_IO_ERR_NO_DEVICE : rc, 1);
        return;
    }

    ata_wait_ready();

    outb(PORT_DEVICE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(PORT_SECCOUNT, 1);
    outb(PORT_LBA_LOW,  (uint8_t)(lba & 0xFF));
    outb(PORT_LBA_MID,  (uint8_t)((lba >> 8) & 0xFF));
    outb(PORT_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));

    outb(PORT_COMMAND, CMD_WRITE);
    ata_wait_drq();  // Wait for drive to request data

    for (int i = 0; i < 256; i++) {
        outw(PORT_DATA, buffer[i]);
    }

    // Flush write cache: send CACHE FLUSH command (0xE7)
    outb(PORT_COMMAND, 0xE7);
    ata_wait_ready();
}

int disk_read_sector_backend(uint32_t lba, uint16_t *buffer, uint8_t backend) {
    if (backend == DISK_BACKEND_ATA) {
        return ata_read_sector_ex(lba, buffer);
    }
    if (backend == DISK_BACKEND_AHCI) {
        return ahci_read_sector_ex(lba, buffer);
    }
    return DISK_IO_ERR_UNSUPPORTED;
}

int disk_read_sector_uncached(uint32_t lba, uint16_t *buffer) {
    disk_io_stats.uncached_reads++;
    int rc;

    if (disk_preferred_backend == DISK_BACKEND_ATA) {
        rc = ata_read_sector_ex(lba, buffer);
        if (rc != DISK_IO_OK) {
            disk_note_error(rc, 0);
        }
        return rc;
    }
    if (disk_preferred_backend == DISK_BACKEND_AHCI) {
        rc = ahci_read_sector_ex(lba, buffer);
        if (rc != DISK_IO_OK) {
            disk_note_error(rc, 0);
        }
        return rc;
    }

    rc = ahci_read_sector_ex(lba, buffer);
    if (rc == DISK_IO_OK) {
        return DISK_IO_OK;
    }

    if (rc == DISK_IO_ERR_TIMEOUT || rc == DISK_IO_ERR_TFES) {
        (void)ahci_reset_first_available_port();
        rc = ahci_read_sector_ex(lba, buffer);
        if (rc == DISK_IO_OK) {
            return DISK_IO_OK;
        }
    }

    if (ata_disk_available) {
        int ata_rc = DISK_IO_ERR_TIMEOUT;
        for (int attempt = 0; attempt < DISK_READ_RETRY_COUNT; attempt++) {
            ata_rc = ata_read_sector_ex(lba, buffer);
            if (ata_rc == DISK_IO_OK) {
                return DISK_IO_OK;
            }
        }
        disk_note_error(ata_rc, 0);
        return ata_rc;
    }
    disk_note_error(rc, 0);
    return rc;
}

int disk_get_geometry(DiskGeometryInfo *out_info) {
    AhciIdentifyInfo ahci_info;

    if (out_info == 0) {
        return 0;
    }

    out_info->total_sectors_low = 0;
    out_info->total_sectors_high = 0;
    out_info->sector_size = 512;
    out_info->backend = disk_preferred_backend;
    out_info->model[0] = '\0';

    if (ahci_identify_first_ready(&ahci_info) && ahci_info.success) {
        out_info->backend = DISK_BACKEND_AHCI;
        out_info->total_sectors_low = ahci_info.lba48_sectors_low;
        out_info->total_sectors_high = ahci_info.lba48_sectors_high;
        copy_text(out_info->model, ahci_info.model, sizeof(out_info->model));
        return 1;
    }

    if (ata_disk_available) {
        out_info->backend = DISK_BACKEND_ATA;
        copy_text(out_info->model, "ATA-LEGACY", sizeof(out_info->model));
        return 1;
    }

    copy_text(out_info->model, "NO-DISK", sizeof(out_info->model));
    return 0;
}

int disk_get_health(DiskHealthInfo *out_info) {
    AhciProbeReport report;
    AhciIdentifyInfo id;

    if (out_info == 0) {
        return 0;
    }

    out_info->available = 0u;
    out_info->backend = DISK_BACKEND_AUTO;
    out_info->preferred_backend = disk_preferred_backend;
    out_info->ahci_ready_ports = 0u;
    out_info->smart_supported = 0u;
    out_info->smart_enabled = 0u;
    out_info->temperature_c = 0u;
    out_info->life_percent = 100u;
    out_info->read_error_count = disk_read_errors;
    out_info->write_error_count = disk_write_errors;
    out_info->last_error_code = (uint32_t)disk_last_error;

    if (ahci_get_runtime_report(&report) && report.controller_found) {
        out_info->ahci_ready_ports = report.ready_ports;
        if (report.ready_ports > 0u) {
            out_info->available = 1u;
            out_info->backend = DISK_BACKEND_AHCI;
        }
    }

    if (ahci_identify_first_ready(&id) && id.success) {
        out_info->available = 1u;
        out_info->backend = DISK_BACKEND_AHCI;
        out_info->smart_supported = id.smart_supported ? 1u : 0u;
        out_info->smart_enabled = id.smart_enabled ? 1u : 0u;
    } else if (ata_disk_available) {
        out_info->available = 1u;
        if (out_info->backend == DISK_BACKEND_AUTO) {
            out_info->backend = DISK_BACKEND_ATA;
        }
    }

    return out_info->available != 0u;
}

int disk_read_sector_ex(uint32_t lba, uint16_t *buffer) {
    return disk_read_sector_uncached(lba, buffer);
}

/*
 * disk_read_sector() - Read 512 bytes (256 words) from disk
 * @lba: Logical Block Address (sector number)
 * @buffer: Pointer to buffer for 512 bytes
 */
void disk_read_sector(uint32_t lba, uint16_t *buffer) {
    disk_io_stats.cached_reads++;
    (void)page_cache_read_sector(lba, buffer);
}

void disk_write_sector(uint32_t lba, uint16_t *buffer) {
    disk_io_stats.cached_writes++;
    (void)page_cache_write_sector(lba, buffer);
}

/* TFS: Find first empty slot in root directory */
int find_free_slot(FileEntry *dir) {
    for (int i = 0; i < TFS_MAX_FILES; i++) {
        if (dir[i].flags == 0) {
            return i;
        }
    }
    return -1; // Directory is full
}
