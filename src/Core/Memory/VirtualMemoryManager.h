#ifndef BOREALOS_VIRTUALMEMORYMANAGER_H
#define BOREALOS_VIRTUALMEMORYMANAGER_H

#include <Definitions.h>
#include "Paging.h"

typedef struct VirtualMemoryManagerMetadata VirtualMemoryManagerMetadata;

typedef struct VirtualMemoryManagerState {
    PagingState *Paging; // The paging structure this VMM manages
    KernelState *Kernel; // Reference to the kernel state for logging and memory allocation
    VirtualMemoryManagerMetadata *Metadata; // Metadata for tracking allocations
} VirtualMemoryManagerState;

Status VirtualMemoryManagerInit(VirtualMemoryManagerState *vmm, PagingState *paging, KernelState *kernel);

// Alloc, free, map, unmap functions
void* VirtualMemoryManagerAllocate(VirtualMemoryManagerState *vmm, size_t size, bool writable, bool user);
Status VirtualMemoryManagerFree(VirtualMemoryManagerState *vmm, void *addr, size_t size);
Status VirtualMemoryManagerMap(VirtualMemoryManagerState *vmm, void *virtualAddr, void *physicalAddr, bool writable, bool user);
Status VirtualMemoryManagerUnmap(VirtualMemoryManagerState *vmm, void *virtualAddr);
void* VirtualMemoryManagerTranslate(VirtualMemoryManagerState *vmm, void *virtualAddr);

#endif //BOREALOS_VIRTUALMEMORYMANAGER_H