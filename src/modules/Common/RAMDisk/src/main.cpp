#include <Module.h>
#include "../Service.h"

MODULE(RAMDISK_MODULE_NAME, RAMDISK_MODULE_DESCRIPTION, RAMDISK_MODULE_VERSION, RAMDISK_MODULE_IMPORTANCE);
RELY_ON(EXTERNAL_MODULE(DISK_MODULE_NAME, VERSION(0,0,1)));

static Disk::Device* RAMDiskDevice = nullptr;
uint64_t RAMDiskSize = DEFAULT_RAMDISK_SIZE;

static STATUS Read(Disk::Device* device, uint64_t offset, uint64_t size, void* buffer) {
    // Make sure the buffer and device not null
    if (!buffer || !device) return STATUS::FAILURE;

    RAMDisk::RAMDiskData* data = (RAMDisk::RAMDiskData*)device->driverData;

    // Detect out of bounds and integer overflow conditions
    if (offset + size > data->size) return STATUS::FAILURE;
    if (offset + size < offset) return STATUS::FAILURE;

    // Read into the buffer
    memcpy(buffer, data->buffer + offset, size);
    return STATUS::SUCCESS;
}

static STATUS Write(Disk::Device* device, uint64_t offset, uint64_t size, const void* buffer) {
    // Make sure the buffer and device not null
    if (!buffer || !device) return STATUS::FAILURE;

    RAMDisk::RAMDiskData* data = (RAMDisk::RAMDiskData*)device->driverData;

    // Detect out of bounds and integer overflow conditions
    if (offset + size > data->size) return STATUS::FAILURE;
    if (offset + size < offset) return STATUS::FAILURE;

    // Write into the buffer
    memcpy(data->buffer + offset, buffer, size);
    return STATUS::SUCCESS;
}

// Flush does nothing for a RAM disk because the writes are fast and happen immediately
static STATUS Flush(Disk::Device* device) {
    (void)device;
    return STATUS::SUCCESS;
}

bool Test() {
    if (!RAMDiskDevice) {
        LOG_ERROR("RAM disk device is null!");
        return false;
    }

    uint8_t dummy = 0;
    bool allPassed = true;
    auto runTest = [&](const char* name, bool result) {
        LOG_DEBUG("%s: %s", name, result ? "PASSED" : "FAILED");
        if (!result) allPassed = false;
    };

    // Run the tests
    runTest("Out-of-bounds read",    RAMDiskDevice->Read(RAMDiskDevice,  RAMDiskSize, 1, &dummy) == STATUS::FAILURE);
    runTest("Out-of-bounds write",   RAMDiskDevice->Write(RAMDiskDevice, RAMDiskSize, 1, &dummy) == STATUS::FAILURE);
    runTest("Integer overflow read", RAMDiskDevice->Read(RAMDiskDevice,  UINT64_MAX,  1, &dummy) == STATUS::FAILURE);
    runTest("Null buffer read",      RAMDiskDevice->Read(RAMDiskDevice,  0,           1, nullptr) == STATUS::FAILURE);

    return allPassed;
}

COMPATIBLE_FUNC() {
    // TODO: Make sure the device has enough RAM to creater the RAM disk
    return true;
}

LOAD_FUNC() {
    // TODO: Get ram disk name and size from kernel parameters if they are passed
    LOG_INFO("Creating RAM disk \"%s\" with a size of %u64 KiB...", DEFAULT_RAMDISK_NAME, RAMDiskSize / Constants::KiB);

    Disk::DiskService* diskService = static_cast<Disk::DiskService*>(
        Core::ServiceManager::GetInstance()->GetService(DISK_SERVICE_NAME));

    if (diskService == nullptr) {
        LOG_ERROR("Failed to get disk service!");
        return STATUS::FAILURE;
    }

    // Create a buffer to act as the block device
    RAMDisk::RAMDiskData* data = new RAMDisk::RAMDiskData();
    data->buffer = new uint8_t[RAMDiskSize];
    data->size = RAMDiskSize;
    memset(data->buffer, 0, RAMDiskSize);

    // Create a disk device so it can be registered with the disk service
    Disk::Device* device = (Disk::Device*)new uint8_t[sizeof(Disk::Device)];
    memset(device, 0, sizeof(Disk::Device));
    strncpy((char*)device->name, DEFAULT_RAMDISK_NAME, 63);
    device->capacity   = RAMDiskSize;
    device->driverData = data;
    device->Read       = Read;
    device->Write      = Write;
    device->Flush      = Flush;

    if (diskService->RegisterDevice(device) != STATUS::SUCCESS) {
        LOG_ERROR("Failed to register RAM disk device!");
        delete[] data->buffer;
        delete data;
        delete[] (uint8_t*)device;
        return STATUS::FAILURE;
    }

    RAMDiskDevice = device;
    LOG_DEBUG("RAM disk \"%s\" is at address 0x%x64 and reserves up to address 0x%x64", (char*)device->name, device, device + device->capacity);
    LOG_DEBUG("RAM disk data buffer is at address 0x%x64", data->buffer);

    // Make sure the capacity is reported correctly
    if (RAMDiskDevice->capacity != RAMDiskSize) {
        LOG_ERROR("RAM disk device reported a capacity of %u64 KiB, but %u64 KiB was expected!", RAMDiskDevice->capacity / Constants::KiB, RAMDiskSize / Constants::KiB);
        return STATUS::FAILURE;
    }

    // Zero the entire RAM disk so it's created in a clean state
    memset(data->buffer, 0, RAMDiskSize);
    LOG_DEBUG("RAM disk zeroed (wrote %u64 byte(s))", RAMDiskSize);
    LOG_INFO("RAM disk registered successfully as \"%s\"", DEFAULT_RAMDISK_NAME);
    return Test() ? STATUS::SUCCESS : STATUS::FAILURE;
}