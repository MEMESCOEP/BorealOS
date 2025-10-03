#include "ATACommon.h"
#include "ATA.h"
#include "ATAPI.h"

#include <Core/Kernel.h>
#include <Core/Memory/Memory.h>
#include "Utility/SerialOperations.h"

#if BOREALOS_ENABLE_ATA

ATAState KernelATA = {};

// The suggestion is to read the Status register FIFTEEN TIMES, and only pay attention to the value returned by the last one -- after selecting a new master or slave device. The point being that you can assume an IO port read takes at least 30ns, so doing the first fourteen creates a 420ns delay -- which allows the drive time to push the correct voltages onto the bus.
void ATAWait450ns(uint16_t ioBase) {
    for (int i = 0; i < 15; i++) {
        inb(ioBase + ATA_REG_STATUS);
    }
}

void ATASelectDrive(uint16_t ioBase, uint8_t drive) {
    outb(ioBase + ATA_REG_HDDEVSEL, 0xA0 | (drive << 4));
    ATAWait450ns(ioBase);
}

Status ATAPollStatus(uint16_t ioBase, uint8_t waitMask, uint32_t timeoutLimit) {
    uint8_t status = inb(ioBase + ATA_REG_STATUS);
    uint32_t timeout = timeoutLimit;
    while ((status & waitMask) != waitMask) {
        status = inb(ioBase + ATA_REG_STATUS);
        if (--timeout == 0) return STATUS_TIMEOUT;
    }
    return STATUS_SUCCESS;
}

void ATASetCommandRegisters(uint16_t ioBase, uint32_t lba, uint8_t sectorCount, ATADrive drive, uint8_t command) {
    outb(ioBase + ATA_REG_FEATURES, 0);
    outb(ioBase + ATA_REG_SECCOUNT0, sectorCount);
    outb(ioBase + ATA_REG_LBA0, (uint8_t)(lba & 0xFF));
    outb(ioBase + ATA_REG_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    outb(ioBase + ATA_REG_LBA2, (uint8_t)((lba >> 16) & 0xFF));
    outb(ioBase + ATA_REG_HDDEVSEL, 0xE0 | ((drive & 0x1) << 4) | ((lba >> 24) & 0x0F));
    outb(ioBase + ATA_REG_COMMAND, command);
    ATAWait450ns(ioBase);
}

void ATASelectDevice(BlockDevice *device, ATABlockDriverData *data) {
    if (device != KernelATA.CurrentDevice || data->Bus != KernelATA.Bus || data->Drive != KernelATA.Drive) {
        // Only reselect if necessary.
        LOGF(LOG_DEBUG, "Selecting %s drive on %s bus...\n",
             (data->Drive == ATA_DRIVE_MASTER) ? "master" : "slave",
             (data->Bus == ATA_BUS_PRIMARY) ? "primary" : "secondary");

        ATASelectDrive(data->IOBase, data->Drive);
        KernelATA.CurrentDevice = device;
        KernelATA.Bus = data->Bus;
        KernelATA.Drive = data->Drive;
    }
}

/*
uint16_t 0: is useful if the device is not a hard disk.
uint16_t 83: Bit 10 is set if the drive supports LBA48 mode.
uint16_t 88: The bits in the low byte tell you the supported UDMA modes, the upper byte tells you which UDMA mode is active. If the active mode is not the highest supported mode, you may want to figure out why. Notes: The returned uint16_t should look like this in binary: 0000001 00000001. Each bit corresponds to a single mode. E.g. if the decimal number is 257, that means that only UDMA mode 1 is supported and running (the binary number above) if the binary number is 515, the binary looks like this, 00000010 00000011, that means that UDMA modes 1 and 2 are supported, and 2 is running. This is true for every mode. If it does not look like that, e.g 00000001 00000011, as stated above, you may want to find out why. The formula for finding out the decimal number is 257 * 2 ^ position + 2 ^position - 1.
uint16_t 93 from a master drive on the bus: Bit 11 is supposed to be set if the drive detects an 80 conductor cable. Notes: if the bit is set then 80 conductor cable is present and UDMA modes > 2 can be used; if bit is clear then there may or may not be an 80 conductor cable and UDMA modes > 2 shouldn't be used but might work fine. Because this bit is "master device only", if there is a slave device and no master there is no way information about cable type (and would have to assume UDMA modes > 2 can't be used).
uint16_t 60 & 61 taken as a uint32_t contain the total number of 28 bit LBA addressable sectors on the drive. (If non-zero, the drive supports LBA28.)
uint16_t 100 through 103 taken as a uint64_t contain the total number of 48 bit addressable sectors on the drive. (Probably also proof that LBA48 is supported.)
 */
void ATAParseIdentify(uint16_t buffer[256], bool* isHDD, bool* supportsLBA48, size_t* totalSectors) {
    // Check if it's an ATA device
    if (buffer[0] == 0) {
        *isHDD = false;
        return;
    }
    *isHDD = true;

    // Check if it supports LBA48
    *supportsLBA48 = (buffer[83] & (1 << 10)) != 0;

    // Get total sectors
    if (*supportsLBA48) {
        *totalSectors = ((uint64_t)buffer[103] << 48) | ((uint64_t)buffer[102] << 32) | ((uint64_t)buffer[101] << 16) | buffer[100];
    } else {
        *totalSectors = buffer[60] | ((uint32_t)buffer[61] << 16);
    }
}

Status ATAInit(void) {
    LOG(LOG_INFO, "Initializing ATA driver...\n");
    uint16_t buses[2] = {ATA_PRIMARY_BUS, ATA_SECONDARY_BUS};

    for (int busIndex = 0; busIndex < 2; busIndex++) {
        uint16_t ioBase = buses[busIndex];

        for (int drive = 0; drive < 2; drive++) {
            if (ATAProbeDevice(ioBase, drive, &KernelATA.DeviceData[KernelATA.DeviceCount], &KernelATA.Devices[KernelATA.DeviceCount]) == STATUS_SUCCESS) {
                // Successfully detected a device. Increment device count.
                KernelATA.DeviceCount++;
            }
            else if (ATAPIProbeDevice(ioBase, drive, &KernelATA.DeviceData[KernelATA.DeviceCount], &KernelATA.Devices[KernelATA.DeviceCount]) == STATUS_SUCCESS) {
                // Successfully detected an ATAPI device. Increment device count.
                KernelATA.DeviceCount++;
            }
        }
    }

    if (KernelATA.DeviceCount == 0) {
        LOG(LOG_WARNING, "No ATA drives detected.\n");
    } else {
        LOGF(LOG_INFO, "Total ATA drives detected: %d\n", KernelATA.DeviceCount);
    }

    return STATUS_SUCCESS;
}

#endif // BOREALOS_ENABLE_ATA