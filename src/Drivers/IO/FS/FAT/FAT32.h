#ifndef BOREALOS_FAT32_H
#define BOREALOS_FAT32_H

#include <Definitions.h>
#include <Drivers/IO/Disk/Block.h>
#include <Drivers/IO/FS/FileSystem.h>

/// Initialize the FAT32 filesystem on the given block device.
/// This initializes the filesystem structure and prepares it for use.
/// To for example mount it, use the FS mount function.
/// Returns STATUS_SUCCESS on success, or an error code on failure.
// Status FAT32Init(BlockDevice* device, FileSystem** outFS);


#endif //BOREALOS_FAT32_H