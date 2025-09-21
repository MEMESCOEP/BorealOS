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

Status VirtualMemoryManagerInit(VirtualMemoryManagerState *vmm, PagingState *paging) {
    vmm->Paging = paging;
    vmm->Metadata = nullptr; // Now we create the metadata structure

    // Allocate metadata structure
    vmm->Metadata = (VirtualMemoryManagerMetadata*)PhysicalMemoryManagerAllocatePage();
    if (!vmm->Metadata) {
        PANIC("VirtualMemoryManagerInit: Failed to allocate metadata structure!\n");
    }

    // Identity map the metadata structure
    if (PagingMapPage(paging, vmm->Metadata, vmm->Metadata, true, false) != STATUS_SUCCESS) {
        PANIC("VirtualMemoryManagerInit: Failed to map metadata structure!\n");
    }
    vmm->Metadata->regions = nullptr;

    return STATUS_SUCCESS;
}
