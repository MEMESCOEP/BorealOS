#include "VirtualMemoryManager.h"

#include "PhysicalMemoryManager.h"
#include "Core/Kernel.h"

typedef struct VMMRegion {
    void* start;
    size_t size;
    uint32_t flags;
    struct VMMRegion* next;
} VMMRegion;

typedef struct VirtualMemoryManagerMetadata {
    VMMRegion* regions;
} VirtualMemoryManagerMetadata;

Status VirtualMemoryManagerInit(VirtualMemoryManagerState *vmm, PagingState *paging, KernelState *kernel) {
    vmm->Kernel = kernel;
    vmm->Paging = paging;
    vmm->Metadata = nullptr; // Now we create the metadata structure

    // Allocate metadata structure
    vmm->Metadata = (VirtualMemoryManagerMetadata*)PhysicalMemoryManagerAllocatePage(&kernel->PhysicalMemoryManager);
    if (!vmm->Metadata) {
        PANIC(kernel, "VirtualMemoryManagerInit: Failed to allocate metadata structure!\n");
    }

    // Identity map the metadata structure
    if (PagingMapPage(paging, vmm->Metadata, vmm->Metadata, true, false, kernel) != STATUS_SUCCESS) {
        PANIC(kernel, "VirtualMemoryManagerInit: Failed to map metadata structure!\n");
    }
    vmm->Metadata->regions = nullptr;

    return STATUS_SUCCESS;
}
