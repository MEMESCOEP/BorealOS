#ifndef BOREALOS_RAMDISK_DRIVER_H
#define BOREALOS_RAMDISK_DRIVER_H

#include <Core/ServiceManager.h>
#include <Utility/StringFormatter.h>
#include "../../../modules/Common/DiskService/Service.h"

#define RAMDISK_MODULE_NAME "RAM Disk Driver"
#define RAMDISK_MODULE_DESCRIPTION "Creates a RAM disk and presents it as a standard block device."
#define RAMDISK_MODULE_VERSION VERSION(0,0,1)
#define RAMDISK_MODULE_IMPORTANCE Formats::DriverModule::Importance::Required

#define DEFAULT_RAMDISK_SECTOR_SIZE 4096
#define DEFAULT_RAMDISK_SIZE        4 * Constants::MiB
#define DEFAULT_RAMDISK_NAME        "ram0"

namespace RAMDisk {
    struct RAMDiskData {
        uint8_t* buffer;
        uint64_t size;
    };

    // Creates and registers a RAM disk
    Disk::Device* Create(const char* name, uint64_t size, Disk::DiskService* diskService);

    // Destroys and unregisters a RAM disk device
    void Destroy(Disk::Device* RAMDiskDevice, Disk::DiskService* diskService);
}

#endif //BOREALOS_RAMDISK_DRIVER_H