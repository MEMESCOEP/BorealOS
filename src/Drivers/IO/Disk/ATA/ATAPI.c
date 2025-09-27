#include "ATA.h"
#include <Core/Kernel.h>
#include "Utility/SerialOperations.h"
#include "ATACommon.h"
#include <Drivers/IO/Disk/Block.h>

static Status ATAPISendPacket(uint16_t ioBase, const uint8_t packet[12], void* buffer, uint32_t size, bool read)
{
    // Wait until drive is ready (not busy)
    if (ATAPollStatus(ioBase, ATA_STATUS_RDY, ATA_BSY_TIMEOUT) != STATUS_SUCCESS) {
        return STATUS_TIMEOUT;
    }

    // Program expected transfer length
    outb(ioBase + ATA_REG_FEATURES, 0);
    outb(ioBase + ATA_REG_LBA1, (uint8_t)(size & 0xFF));
    outb(ioBase + ATA_REG_LBA2, (uint8_t)((size >> 8) & 0xFF));

    // Issue PACKET command
    outb(ioBase + ATA_REG_COMMAND, ATA_CMD_PACKET);
    ATAWait450ns(ioBase);

    // Wait for DRQ so we can send the 12-byte packet
    if (ATAPollStatus(ioBase, ATA_STATUS_DRQ, ATA_BSY_TIMEOUT) != STATUS_SUCCESS) {
        return STATUS_TIMEOUT;
    }

    // Send the packet as 6 words (little-endian)
    for (int i = 0; i < 6; i++) {
        uint16_t word = packet[2*i] | (packet[2*i+1] << 8);
        outw(ioBase + ATA_REG_DATA, word);
    }

    // Now wait for the device to enter the data phase
    if (size > 0) {
        if (ATAPollStatus(ioBase, ATA_STATUS_DRQ, ATA_BSY_TIMEOUT) != STATUS_SUCCESS) {
            return STATUS_TIMEOUT;
        }

        if (read) {
            for (uint32_t i = 0; i < size / 2; i++) {
                ((uint16_t*)buffer)[i] = inw(ioBase + ATA_REG_DATA);
            }
        } else {
            for (uint32_t i = 0; i < size / 2; i++) {
                outw(ioBase + ATA_REG_DATA, ((uint16_t*)buffer)[i]);
            }
        }
    }

    // Final status check
    uint8_t status = inb(ioBase + ATA_REG_STATUS);
    if (status & ATA_STATUS_ERR) {
        return STATUS_FAILURE;
    }

    return STATUS_SUCCESS;
}

static Status ATAPIReadCapacity(uint16_t ioBase, uint32_t* totalSectors, uint32_t* sectorSize) {
    uint8_t packet[12] = {0};
    packet[0] = ATA_CMD_READ_CAPACITY;

    uint8_t buffer[8];
    Status st = ATAPISendPacket(ioBase, packet, buffer, sizeof(buffer), true);
    if (st != STATUS_SUCCESS) {
        return st;
    }

    uint32_t lastLBA = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
    uint32_t secSize = (buffer[4] << 24) | (buffer[5] << 16) | (buffer[6] << 8) | buffer[7];
    *totalSectors = lastLBA + 1; // Last LBA is zero-based
    *sectorSize = secSize;

    return STATUS_SUCCESS;
}

Status ATAPIReadSectors(BlockDevice* device, uint64_t lba, uint32_t count, void* buffer) {
    ATABlockDriverData* data = (ATABlockDriverData*)device->DriverData;
    uint16_t ioBase = data->IOBase;

    for (uint32_t sectorIdx = 0; sectorIdx < count; sectorIdx++) {
        uint8_t packet[12] = {0};
        packet[0] = 0x28; // READ (10)
        packet[2] = (uint8_t)((lba >> 24) & 0xFF);
        packet[3] = (uint8_t)((lba >> 16) & 0xFF);
        packet[4] = (uint8_t)((lba >> 8) & 0xFF);
        packet[5] = (uint8_t)(lba & 0xFF);
        packet[8] = 1; // Transfer 1 block at a time

        uint8_t* bufPtr = (uint8_t*)buffer + sectorIdx * device->SectorSize;
        Status st = ATAPISendPacket(ioBase, packet, bufPtr, device->SectorSize, true);
        if (st != STATUS_SUCCESS) {
            return st;
        }
        lba++;
    }
    return STATUS_SUCCESS;
}

Status ATAPIProbeDevice(uint16_t ioBase, uint8_t drive, ATABlockDriverData* blockDriverData, BlockDevice* outDevice) {
    LOGF(LOG_DEBUG, "Probing %s drive on %s bus...\n",
         (drive == 0) ? "master" : "slave",
         (ioBase == ATA_PRIMARY_BUS) ? "primary" : "secondary");

    ATASelectDrive(ioBase, drive);

    outb(ioBase + ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET_DEVICE);
    ATAWait450ns(ioBase);

    uint8_t status = inb(ioBase + ATA_REG_STATUS);
    if (status == 0) {
        LOGF(LOG_DEBUG, "No device present at %s %s.\n",
             (ioBase == ATA_PRIMARY_BUS) ? "primary" : "secondary",
             (drive == 0) ? "master" : "slave");
        return STATUS_FAILURE;
    }

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
    (void)identifyData; // idk anymore im tired

    uint32_t totalSectors;
    uint32_t sectorSize;
    if (ATAPIReadCapacity(ioBase, &totalSectors, &sectorSize) != STATUS_SUCCESS) {
        // If READ CAPACITY fails, we can't use this device
        LOGF(LOG_DEBUG, "READ CAPACITY failed on %s %s.\n",
             (ioBase == ATA_PRIMARY_BUS) ? "primary" : "secondary",
             (drive == 0) ? "master" : "slave");
        return STATUS_FAILURE;
    }

    LOGF(LOG_DEBUG, "Detected ATAPI device at %s %s\n",
         (ioBase == ATA_PRIMARY_BUS) ? "primary" : "secondary",
         (drive == 0) ? "master" : "slave");
    PRINTF("\t* Sector Size: %u\n", sectorSize);
    PRINTF("\t* Total Sectors: %u\n", totalSectors);

    outDevice->SectorSize = sectorSize;
    outDevice->TotalSectors = totalSectors;
    outDevice->DriverData = blockDriverData;
    outDevice->Read = ATAPIReadSectors;
    outDevice->Write = nullptr; // usually not supported

    blockDriverData->Bus = (ioBase == ATA_PRIMARY_BUS) ? ATA_BUS_PRIMARY : ATA_BUS_SECONDARY;
    blockDriverData->Drive = (drive == 0) ? ATA_DRIVE_MASTER : ATA_DRIVE_SLAVE;
    blockDriverData->IOBase = ioBase;
    blockDriverData->ControlBase = (blockDriverData->Bus == ATA_BUS_PRIMARY) ? ATA_PRIMARY_CONTROL : ATA_SECONDARY_CONTROL;
    blockDriverData->Type = ATAPI_DRIVE;

    return STATUS_SUCCESS;
}