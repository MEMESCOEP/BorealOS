#include "DiskTracker.h"

#include <Definitions.h>

DiskTracker *trackerInstance = nullptr;

DiskTracker::DiskTracker() : _devices(16) {
    if (trackerInstance != nullptr) {
        PANIC("Multiple instances of DiskTracker detected.");
    }

    _service = {
        .GetDeviceByName = GetDeviceByName,
        .RegisterDevice = RegisterDevice,
        .UnregisterDevice = UnregisterDevice,
        .GetDevices = GetDevices
    };

    trackerInstance = this;
}

Disk::DiskService *DiskTracker::GetService() {
     return &_service;
}

Disk::Device * DiskTracker::GetDeviceByName(const char *name) {
    for (size_t i = 0; i < trackerInstance->_devices.Size(); i++) {
        if (strcmp(trackerInstance->_devices[i]->name, name) == 0) {
            return trackerInstance->_devices[i];
        }
    }

    return nullptr;
}

STATUS DiskTracker::RegisterDevice(Disk::Device *device) {
    if (GetDeviceByName(device->name) != nullptr) {
        return STATUS::FAILURE; // Device with this name is already registered
    }

    trackerInstance->_devices.Add(device);
    return STATUS::SUCCESS;
}

STATUS DiskTracker::UnregisterDevice(const char *name) {
    for (size_t i = 0; i < trackerInstance->_devices.Size(); i++) {
        if (strcmp(trackerInstance->_devices[i]->name, name) == 0) {
            trackerInstance->_devices.Remove(i);
            return STATUS::SUCCESS;
        }
    }

    return STATUS::FAILURE; // Device not found
}

size_t DiskTracker::GetDevices(Disk::Device **devices, size_t maxDevices) {
    size_t count = trackerInstance->_devices.Size();
    if (devices != nullptr) {
        size_t toCopy = count < maxDevices ? count : maxDevices;
        for (size_t i = 0; i < toCopy; i++) {
            devices[i] = trackerInstance->_devices[i];
        }
    }

    return count;
}
