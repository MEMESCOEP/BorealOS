#ifndef BOREALOS_CPIO_H
#define BOREALOS_CPIO_H

#include <Definitions.h>

#include "Core/Memory/HeapAllocator.h"

typedef struct FileSystem FileSystem;

/// Load a CPIO archive from the given memory address and size.
/// This calls the operations on the FileSystem to create files and directories as needed.
Status CPIOLoadArchive(void* baseAddress, size_t size, FileSystem* outFS);

Status CPIOFromModule(uint32_t MB2InfoPtr, HeapAllocatorState *heap, void** outBase, size_t* outSize);

#endif //BOREALOS_CPIO_H