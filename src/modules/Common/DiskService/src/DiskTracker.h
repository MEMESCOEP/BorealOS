#ifndef BOREALOS_TRACKER_H
#define BOREALOS_TRACKER_H

#include <Utility/List.h>

#include "../Service.h"

class DiskTracker {
public:
    DiskTracker();
    ~DiskTracker() = default;

    Disk::DiskService* GetService();

private:
    Disk::DiskService _service{};

    Utility::List<Disk::Device*> _devices;

    static Disk::Device *GetDeviceByName(const char* name);
    static STATUS RegisterDevice(Disk::Device* device);
    static STATUS UnregisterDevice(const char* name);
    static size_t GetDevices(Disk::Device** devices, size_t maxDevices);
    static size_t GetPartitions(Disk::Device *device, Disk::Partition *partitions, size_t maxPartitions);
    static STATUS CreatePartition(Disk::Device *device, uint64_t offset, uint64_t size, const char* name);
    static STATUS DeletePartition(Disk::Device *device, const char* name);
};


#endif //BOREALOS_TRACKER_H