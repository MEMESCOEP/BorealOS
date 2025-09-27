#ifndef BOREALOS_BLOCK_H
#define BOREALOS_BLOCK_H

#include <Definitions.h>

typedef struct BlockDevice BlockDevice;
typedef Status (*BlockReadFn)(BlockDevice* device, uint64_t sector, uint32_t count, void* buffer);
typedef Status (*BlockWriteFn)(BlockDevice* device, uint64_t sector, uint32_t count, const void* buffer);

typedef void* BlockDriverData;

/// Represents a block device, such as a hard drive or SSD.
/// This is API agnostic; specific implementations will fill in the function pointers.
/// You must also provide a pointer to the actual device when calling the read/write functions. This is to allow multiple devices to share the same function pointers.
typedef struct BlockDevice{
    uint32_t SectorSize; // Size of a sector in bytes (usually 512 or 4096)
    uint64_t TotalSectors; // Total number of sectors on the device
    BlockDriverData DriverData; // Pointer to driver-specific data, can be NULL

    /// Read sectors from the device. Returns STATUS_SUCCESS on success, or an error code on failure.
    /// The count is in sectors, not bytes.
    /// The buffer must be large enough to hold count * SectorSize bytes.
    /// If null, the device does not support reading.
    BlockReadFn Read; // Function pointer to read sectors

    /// Write sectors to the device. Returns STATUS_SUCCESS on success, or an error code on failure.
    /// The count is in sectors, not bytes.
    /// The buffer must be large enough to hold count * SectorSize bytes.
    /// If null, the device does not support writing.
    BlockWriteFn Write; // Function pointer to write sectors
} BlockDevice;

#endif //BOREALOS_BLOCK_H