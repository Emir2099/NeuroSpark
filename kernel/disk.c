#include "disk.h"

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

void disk_write_sector(uint32_t lba, uint16_t *buffer) {
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

/*
 * disk_read_sector() - Read 512 bytes (256 words) from disk
 * @lba: Logical Block Address (sector number)
 * @buffer: Pointer to buffer for 512 bytes
 */
void disk_read_sector(uint32_t lba, uint16_t *buffer) {
    ata_wait_ready();

    outb(PORT_DEVICE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(PORT_SECCOUNT, 1);
    outb(PORT_LBA_LOW,  (uint8_t)(lba & 0xFF));
    outb(PORT_LBA_MID,  (uint8_t)((lba >> 8) & 0xFF));
    outb(PORT_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));

    outb(PORT_COMMAND, CMD_READ);
    ata_wait_drq();  // Wait for data to be ready

    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(PORT_DATA);
    }
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
