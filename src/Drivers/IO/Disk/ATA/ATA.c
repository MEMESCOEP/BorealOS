#include "ATA.h"
#include <Core/Kernel.h>
#include "Utility/SerialOperations.h"
#include "ATACommon.h"
#include <Drivers/IO/Disk/Block.h>

#if BOREALOS_ENABLE_ATA

Status ReadSectors(uint32_t count, void *buffer, uint16_t ioBase) {
    for (uint32_t sectorIdx = 0; sectorIdx < count; sectorIdx++) {
        // Wait for DRQ for this sector
        if (ATAPollStatus(ioBase, ATA_STATUS_DRQ, ATA_BSY_TIMEOUT) != STATUS_SUCCESS) {
            LOG(LOG_ERROR, "Timeout waiting for DRQ after issuing read command.\n");
            return STATUS_FAILURE;
        }

        // Read one sector (256 words)
        for (int i = 0; i < ATA_SECTOR_SIZE / 2; i++) {
            uint16_t data = inw(ioBase + ATA_REG_DATA);
            ((uint16_t*)buffer)[sectorIdx * (ATA_SECTOR_SIZE / 2) + i] = data;
        }
    }

    // Check for errors
    uint8_t status = inb(ioBase + ATA_REG_STATUS);
    if (status & ATA_STATUS_ERR) {
        LOG(LOG_ERROR, "Error occurred during read operation.\n");
        return STATUS_FAILURE;
    }

    return STATUS_SUCCESS;
}

Status WriteSectors(uint32_t count, const void *buffer, uint16_t ioBase, bool lba48) {
    for (uint32_t sectorIdx = 0; sectorIdx < count; sectorIdx++) {
        // Wait for DRQ for this sector
        if (ATAPollStatus(ioBase, ATA_STATUS_DRQ, ATA_BSY_TIMEOUT) != STATUS_SUCCESS) {
            LOG(LOG_ERROR, "Timeout waiting for DRQ after issuing write command.\n");
            return STATUS_FAILURE;
        }

        // Write one sector (256 words)
        for (int i = 0; i < ATA_SECTOR_SIZE / 2; i++) {
            uint16_t data = ((uint16_t*)buffer)[sectorIdx * (ATA_SECTOR_SIZE / 2) + i];
            outw(ioBase + ATA_REG_DATA, data);
        }

        // Flush
        outb(ioBase + ATA_REG_COMMAND, lba48 ? ATA_CMD_CACHE_FLUSH_EXT : ATA_CMD_CACHE_FLUSH);
        ATAWait450ns(ioBase);
    }

    // Check for errors
    uint8_t status = inb(ioBase + ATA_REG_STATUS);
    if (status & ATA_STATUS_ERR) {
        LOG(LOG_ERROR, "Error occurred during write operation.\n");
        return STATUS_FAILURE;
    }

    return STATUS_SUCCESS;
}

Status ATAReadSectors(BlockDevice* device, uint64_t sector, uint32_t count, void* buffer) {
    if (device == nullptr || buffer == nullptr || count == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    ATABlockDriverData* data = (ATABlockDriverData*)device->DriverData;
    if (data == nullptr) {
        return STATUS_FAILURE;
    }

    ATASelectDevice(device, data);
    uint16_t ioBase = data->IOBase;

    if (sector > 0x0FFFFFFF) {
        LOG(LOG_ERROR, "LBA28 addressing only supported currently.\n");
        return STATUS_UNSUPPORTED;
    }

    ATAPollStatus(ioBase, ATA_STATUS_BSY, ATA_BSY_TIMEOUT); // Wait for BSY to clear

    // Set up registers for reading
    ATASetCommandRegisters(ioBase, (uint32_t)sector, (uint8_t)count, data->Drive, ATA_CMD_READ_SECTORS);

    return ReadSectors(count, buffer, ioBase);
}

Status ATAWriteSectors(BlockDevice* device, uint64_t sector, uint32_t count, const void* buffer) {
    if (device == nullptr || buffer == nullptr || count == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    ATABlockDriverData* data = (ATABlockDriverData*)device->DriverData;
    if (data == nullptr) {
        return STATUS_FAILURE;
    }

    if (data->Type == ATAPI_DRIVE) {
        LOG(LOG_ERROR, "Attempted to write to an ATAPI device.\n");
        return STATUS_UNSUPPORTED; // Writing to ATAPI devices is not supported
    }

    ATASelectDevice(device, data);
    uint16_t ioBase = data->IOBase;

    if (sector > 0x0FFFFFFF) {
        LOG(LOG_ERROR, "LBA28 addressing only supported currently.\n");
        return STATUS_UNSUPPORTED;
    }

    ATAPollStatus(ioBase, ATA_STATUS_BSY, ATA_BSY_TIMEOUT); // Wait for BSY to clear

    // Set up registers for writing
    ATASetCommandRegisters(ioBase, (uint32_t)sector, (uint8_t)count, data->Drive, ATA_CMD_WRITE_SECTORS);

    return WriteSectors(count, buffer, ioBase, false);
}


Status ATAProbeDevice(uint16_t ioBase, uint8_t drive, ATABlockDriverData* blockDriverData, BlockDevice* outDevice)
{
    LOGF(LOG_DEBUG, "Probing %s drive on %s bus...\n",
         (drive == 0) ? "master" : "slave",
         (ioBase == ATA_PRIMARY_BUS) ? "primary" : "secondary");

    ATASelectDrive(ioBase, drive);

    // Issue IDENTIFY DEVICE first (ATA)
    outb(ioBase + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    uint8_t status = inb(ioBase + ATA_REG_STATUS);
    if (status == 0) {
        LOGF(LOG_DEBUG, "No device present at %s %s.\n",
             (ioBase == ATA_PRIMARY_BUS) ? "primary" : "secondary",
             (drive == 0) ? "master" : "slave");
        return STATUS_FAILURE;
    }

    // Wait for BSY clear
    uint32_t timeout = ATA_BSY_TIMEOUT;
    while (status & ATA_STATUS_BSY) {
        status = inb(ioBase + ATA_REG_STATUS);
        if (--timeout == 0) {
            LOGF(LOG_DEBUG, "Timeout waiting for %s %s.\n",
                 (ioBase == ATA_PRIMARY_BUS) ? "primary" : "secondary",
                 (drive == 0) ? "master" : "slave");
            return STATUS_FAILURE;
        }
    }

    uint16_t identifyData[256];
    for (int i = 0; i < 256; i++) {
        identifyData[i] = inw(ioBase + ATA_REG_DATA);
    }

    bool isHDD = false;
    bool supportsLBA48 = false;
    size_t totalSectors = 0;
    ATAParseIdentify(identifyData, &isHDD, &supportsLBA48, &totalSectors);

    if (!isHDD) {
        LOGF(LOG_DEBUG, "Device at %s %s did not look like a hard disk.\n",
             (ioBase == ATA_PRIMARY_BUS) ? "primary" : "secondary",
             (drive == 0) ? "master" : "slave");
        return STATUS_FAILURE;
    }

    LOGF(LOG_DEBUG, "Detected ATA hard disk at %s %s\n",
         (ioBase == ATA_PRIMARY_BUS) ? "primary" : "secondary",
         (drive == 0) ? "master" : "slave");
    PRINTF("\t* Supports LBA48: %s\n", supportsLBA48 ? "Yes" : "No");
    PRINTF("\t* Total Sectors: %u\n", (uint32_t)totalSectors);
    PRINTF("\t* Total Size: %u MB\n", (uint32_t)((totalSectors * ATA_SECTOR_SIZE) / MiB));

    // Fill out the BlockDevice structure
    outDevice->SectorSize = ATA_SECTOR_SIZE;
    outDevice->TotalSectors = totalSectors;
    outDevice->DriverData = blockDriverData;
    outDevice->Read = ATAReadSectors;
    outDevice->Write = ATAWriteSectors;

    blockDriverData->Bus = (ioBase == ATA_PRIMARY_BUS) ? ATA_BUS_PRIMARY : ATA_BUS_SECONDARY;
    blockDriverData->Drive = (drive == 0) ? ATA_DRIVE_MASTER : ATA_DRIVE_SLAVE;
    blockDriverData->IOBase = ioBase;
    blockDriverData->ControlBase = (blockDriverData->Bus == ATA_BUS_PRIMARY) ? ATA_PRIMARY_CONTROL : ATA_SECONDARY_CONTROL;
    blockDriverData->Type = ATA_DRIVE;

    return STATUS_SUCCESS;
}

#endif // BOREALOS_ENABLE_ATA