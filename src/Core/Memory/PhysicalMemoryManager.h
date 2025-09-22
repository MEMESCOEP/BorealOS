#ifndef BOREALOS_PHYSICALMEMORYMANAGER_H
#define BOREALOS_PHYSICALMEMORYMANAGER_H

#include <Definitions.h>

typedef struct PhysicalMemoryManagerState {
    uint8_t *AllocationMap; // Bitmap for tracking allocated pages
    uint8_t *ReservedMap; // Bitmap for tracking reserved pages (these can NEVER be freed)
    size_t TotalPages; // Total number of pages in the system
    size_t MapSize; // Size of the allocation and reserved maps in bytes
} PhysicalMemoryManagerState;

extern PhysicalMemoryManagerState KernelPhysicalMemoryManager;

#define PMM_PAGE_SIZE 4096

/// Initialize the physical memory manager for the system.
/// InfoPtr is a pointer to the multiboot2 information structure.
Status PhysicalMemoryManagerInit(uint32_t InfoPtr);

/// Allocate a single 4 KiB page of physical memory. Returns NULL if no pages are available.
void* PhysicalMemoryManagerAllocatePage();

/// Allocate multiple contiguous pages of physical memory. Returns NULL if not enough contiguous pages are available.
void* PhysicalMemoryManagerAllocatePages(size_t numPages);

/// Free a previously allocated page. Returns STATUS_FAILURE if the page is not valid or not allocated.
Status PhysicalMemoryManagerFreePage(void *page);

/// Free multiple contiguous pages starting at startPage. Returns STATUS_FAILURE if any page is not valid or not allocated.
Status PhysicalMemoryManagerFreePages(void *startPage, size_t numPages);

/// Test the physical memory manager by allocating, writing, and freeing pages.
Status PhysicalMemoryManagerTest();

#endif //BOREALOS_PHYSICALMEMORYMANAGER_H