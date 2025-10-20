#ifndef BOREALOS_RAMDISK_H
#define BOREALOS_RAMDISK_H

#include <Definitions.h>
#include <Core/Memory/HeapAllocator.h>
#include <Drivers/IO/FS/FileSystem.h>

#define RAMDISK_FILESYSTEM_NAME "ramdisk"
Status RamDiskInit(HeapAllocatorState *Heap, FileSystem** outFS);

#endif //BOREALOS_RAMDISK_H