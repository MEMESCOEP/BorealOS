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
};


#endif //BOREALOS_TRACKER_H