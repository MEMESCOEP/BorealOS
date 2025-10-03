#ifndef BOREALOS_ATACOMMON_H
#define BOREALOS_ATACOMMON_H

// Thank you mr https://wiki.osdev.org/ATA_PIO_Mode

#include <Definitions.h>
#include <Core/Memory/VirtualMemoryManager.h>

#include "../Block.h"
#if BOREALOS_ENABLE_ATA


#define ATA_MAX_DRIVES 4 // Support up to 4 ATA drives (2 channels, master/slave each)
#define ATA_SECTOR_SIZE 512 // Standard sector size for ATA devices
#define ATAPI_SECTOR_SIZE 2048 // Standard sector size for ATAPI devices (CD-ROMs)

// ATA bus definitions
#define ATA_PRIMARY_BUS 0x1F0 // Until 0x1F7 are the data and command ports
#define ATA_PRIMARY_CONTROL 0x3F6 // Control port for primary bus
#define ATA_SECONDARY_BUS 0x170 // Until 0x177 are the data and command ports
#define ATA_SECONDARY_CONTROL 0x376 // Control port for secondary bus
#define ATA_PRIMARY_IRQ 14
#define ATA_SECONDARY_IRQ 15

// ATA registers (I/O base + offset) (bits in parentheses are sizes, first is LBA28, second is LBA48)
#define ATA_REG_DATA 0x00 // Data Register (16 bits)
#define ATA_REG_ERROR 0x01 // Error Register (read) (8 bit/16 bits)
#define ATA_REG_FEATURES 0x01 // Features Register (write) (8 bit/16 bits)
#define ATA_REG_SECCOUNT0 0x02 // Sector Count Register 0 (read/write) (8 bit/16 bits)
#define ATA_REG_LBA0 0x03 // LBA Low Register 0 (read/write) (8 bit/16 bits)
#define ATA_REG_LBA1 0x04 // LBA Mid Register 1 (read/write) (8 bit/16 bits)
#define ATA_REG_LBA2 0x05 // LBA High Register 2 (read/write) (8 bit/16 bits)
#define ATA_REG_HDDEVSEL 0x06 // Drive/Head Register (8 bits)
#define ATA_REG_COMMAND 0x07 // Command Register (write) (8 bits)
#define ATA_REG_STATUS 0x07 // Status Register (read) (8 bits)

// ATA registers (Control base + offset)
#define ATA_REG_ALT_STATUS 0x00 // Alternate Status Register (read) (8 bits)
#define ATA_REG_DEVICE_CONTROL 0x00 // Device Control Register (write) (8 bits)
#define ATA_REG_DRIVE_ADDRESS 0x01 // Drive Address Register (read) (8 bits)

// ATA error register bits
#define ATA_ERR_AMNF 0x01 // Address mark not found
#define ATA_ERR_TKZNF 0x02 // Track 0 not found
#define ATA_ERR_ABRT 0x04 // Aborted command
#define ATA_ERR_MCR 0x08 // Media change request
#define ATA_ERR_IDNF 0x10 // ID not found
#define ATA_ERR_MC 0x20 // Media changed
#define ATA_ERR_UNC 0x40 // Uncorrectable data error
#define ATA_ERR_BBK 0x80 // Bad block

// ATA drive/head register bits
#define ATA_DRIVE_HEAD_DRV 0x08 // Drive number (0 = master, 1 = slave)
#define ATA_DRIVE_HEAD_LBA 0x40 // LBA mode (To check if LBA addressing is used)

// ATA status flags (Status Register (I/O base + 7))
#define ATA_STATUS_ERR 0x01 // Error
#define ATA_STATUS_IDX 0x02 // Index (always 0)
#define ATA_STATUS_CORR 0x04 // Corrected data
#define ATA_STATUS_DRQ 0x08 // Data request
#define ATA_STATUS_DSC 0x10 // Drive seek complete
#define ATA_STATUS_DF 0x20 // Drive fault error (does not set ERR)
#define ATA_STATUS_RDY 0x40 // Drive ready (DRDY)
#define ATA_STATUS_BSY 0x80 // Busy

// ATA device control register bits (Control Register (Control base + 0))
#define ATA_CTRL_NIEN 0x02 // Disable interrupts when set
#define ATA_CTRL_SRST 0x04 // Software reset
#define ATA_CTRL_HOB 0x80 // High order byte (for LBA48)

// ATA commands
#define ATA_CMD_IDENTIFY 0xEC // Identify command
#define ATA_CMD_IDENTIFY_PACKET_DEVICE 0xA1 // Identify Packet Device command
#define ATA_CMD_READ_SECTORS 0x20 // Read sectors command (LBA28)
#define ATA_CMD_READ_SECTORS_EXT 0x24 // Read sectors command (LBA48)
#define ATA_CMD_READ_CAPACITY 0x25 // Read capacity command (LBA48)
#define ATA_CMD_WRITE_SECTORS 0x30 // Write sectors command (LBA28)
#define ATA_CMD_WRITE_SECTORS_EXT 0x34 // Write sectors command (LBA48)
#define ATA_CMD_PACKET 0xA0 // Packet command
#define ATA_CMD_DEVICE_RESET 0x08 // Device reset command
#define ATA_CMD_CACHE_FLUSH 0xE7 // Cache flush command (LBA
#define ATA_CMD_CACHE_FLUSH_EXT 0xEA // Cache flush command (LBA48)
#define ATA_CMD_STANDBY_IMMEDIATE 0xE0 // Standby immediate command
#define ATA_BSY_TIMEOUT 100000

typedef enum {
    ATA_BUS_PRIMARY,
    ATA_BUS_SECONDARY
} ATABus;

typedef enum {
    ATA_DRIVE_MASTER,
    ATA_DRIVE_SLAVE
} ATADrive;

typedef enum {
    ATA_DRIVE,
    ATAPI_DRIVE
} ATADriveType;

typedef struct ATABlockDriverData {
    ATABus Bus;
    ATADrive Drive;
    uint16_t IOBase; // I/O base port for the bus
    uint16_t ControlBase; // Control base port for the bus
    ATADriveType Type; // ATA or ATAPI
} ATABlockDriverData;

typedef struct ATAState {
    ATABus Bus; // Primary or secondary bus, this is to know which ports to use.
    ATADrive Drive; // Master or slave drive on the bus

    BlockDevice Devices[ATA_MAX_DRIVES]; // Block devices for each detected ATA drive
    ATABlockDriverData DeviceData[ATA_MAX_DRIVES]; // Driver-specific data for each device
    BlockDevice *CurrentDevice; // Currently selected device for operations
    uint8_t DeviceCount; // Number of detected ATA drives
} ATAState;

extern ATAState KernelATA;

/// Initialize the ATA subsystem, detecting connected drives and setting up BlockDevice structures for them.
Status ATAInit(void);

void ATASelectDrive(uint16_t ioBase, uint8_t drive);
void ATASelectDevice(BlockDevice *device, ATABlockDriverData *data);
void ATAParseIdentify(uint16_t buffer[256], bool* isHDD, bool* supportsLBA48, size_t* totalSectors);
void ATAWait450ns(uint16_t ioBase);
Status ATAPollStatus(uint16_t ioBase, uint8_t mask, uint32_t timeout);
void ATASetCommandRegisters(uint16_t ioBase, uint32_t lba, uint8_t sectorCount, ATADrive drive, uint8_t command);

#endif // BOREALOS_ENABLE_ATA

#endif //BOREALOS_ATACOMMON_H