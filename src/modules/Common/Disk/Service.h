#ifndef BOREALOS_SERVICE_H
#define BOREALOS_SERVICE_H

#define DISK_MODULE_NAME "Disk Service"
#define DISK_MODULE_DESCRIPTION "A service for disk drivers to provide a common API to interact with disks, so that other drivers (like filesystems) can interact with them"
#define DISK_MODULE_VERSION VERSION(0,0,1)
#define DISK_MODULE_IMPORTANCE Formats::DriverModule::Importance::Required

#define DISK_SERVICE_NAME "disk.service" // We define the service as a macro, so we can't make a mistake when registering/getting the service from the manager.

namespace Disk {
    typedef struct Device Device;
    typedef struct Partition Partition;

    typedef STATUS (*ReadDiskFunc)(Device *device, uint64_t offset, uint64_t size, void* buffer);
    typedef STATUS (*WriteDiskFunc)(Device *device, uint64_t offset, uint64_t size, const void* buffer);
    typedef STATUS (*FlushDiskFunc)(Device *device);

    /// Gets the partitions for a given device, if partitions is null then this function will return the number of partitions, otherwise it will fill the provided partitions array.
    typedef size_t (*GetPartitionsFunc)(Device *device, Partition *partitions, size_t maxPartitions);
    typedef STATUS (*CreatePartitionFunc)(Device *device, uint64_t offset, uint64_t size, const char* name);
    typedef STATUS (*DeletePartitionFunc)(Device *device, const char* name);

    /// A device is a physical disk, not to be confused with a partition.
    struct Device {
        void* driverData;
        const char name[64];
        uint64_t capacity;

        ReadDiskFunc Read;
        WriteDiskFunc Write;
        FlushDiskFunc Flush;

        GetPartitionsFunc GetPartitions;
        CreatePartitionFunc CreatePartition;
        DeletePartitionFunc DeletePartition;
    };

    typedef STATUS (*ReadPartitionFunc)(Partition *partition, uint64_t offset, uint64_t size, void* buffer);
    typedef STATUS (*WritePartitionFunc)(Partition *partition, uint64_t offset, uint64_t size, const void* buffer);
    typedef STATUS (*FlushPartitionFunc)(Partition *partition);

    struct Partition {
        void* driverData;
        const char name[64];
        uint64_t offset;
        uint64_t size;

        ReadPartitionFunc Read;
        WritePartitionFunc Write;
        FlushPartitionFunc Flush;
    };

    typedef Device* (*GetDeviceByNameFunc)(const char* name);
    typedef STATUS (*RegisterDeviceFunc)(Device *device);
    typedef STATUS (*UnregisterDeviceFunc)(const char* name);
    typedef size_t (*GetDevicesFunc)(Device **devices, size_t maxDevices);

    struct DiskService {
        GetDeviceByNameFunc GetDeviceByName;
        RegisterDeviceFunc RegisterDevice;
        UnregisterDeviceFunc UnregisterDevice;
        GetDevicesFunc GetDevices;
    };
}

#endif //BOREALOS_SERVICE_H