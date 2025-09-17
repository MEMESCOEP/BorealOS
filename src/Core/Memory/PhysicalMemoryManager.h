#ifndef BOREALOS_PHYSICALMEMORYMANAGER_H
#define BOREALOS_PHYSICALMEMORYMANAGER_H

#include <Definitions.h>

typedef struct PhysicalMemoryManagerState {
    uint8_t *AllocationMap; // Bitmap for tracking allocated pages
    uint8_t *ReservedMap; // Bitmap for tracking reserved pages (these can NEVER be freed)
    size_t TotalPages; // Total number of pages in the system
    size_t MapSize; // Size of the allocation and reserved maps in bytes
} PhysicalMemoryManagerState;

Status PhysicalMemoryManagerLoad(uint32_t InfoPtr, PhysicalMemoryManagerState *out);
void* PhysicalMemoryManagerAllocatePage(PhysicalMemoryManagerState *state);

Status PhysicalMemoryManagerFreePage(PhysicalMemoryManagerState *state, void *page);

typedef struct KernelState KernelState; // Forward declaration to avoid circular dependency
Status PhysicalMemoryManagerTest(PhysicalMemoryManagerState *state, KernelState *kernel);

#endif //BOREALOS_PHYSICALMEMORYMANAGER_H