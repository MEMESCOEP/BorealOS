#include "VirtualMemoryManager.h"

#include "PhysicalMemoryManager.h"
#include "Core/Kernel.h"

typedef struct VMMRegion {
    void* start; // Start address of the virtual memory region
    size_t size; // Size of said region
    uint32_t flags; // Flags for the region (writable, user, etc.)
    struct VMMRegion* next; // Pointer to the next VMMRegion in the linked list, or nullptr if this is the last region.
} VMMRegion;

// A simple max number of regions we can track. This is a limitation of this simple implementation.
#define VMM_REGION_POOL_SIZE 512

typedef struct VirtualMemoryManagerMetadata {
    VMMRegion* regions;
    VMMRegion regionPool[VMM_REGION_POOL_SIZE];
    VMMRegion* freeList;
} VirtualMemoryManagerMetadata;

static void InitRegionPool(VirtualMemoryManagerMetadata* meta) {
    // Initialize it all the next pointers to form a free list
    meta->freeList = &meta->regionPool[0];
    for (size_t i = 0; i < VMM_REGION_POOL_SIZE - 1; i++) {
        meta->regionPool[i].next = &meta->regionPool[i + 1];
    }
    meta->regionPool[VMM_REGION_POOL_SIZE - 1].next = nullptr;
}

static VMMRegion* AllocateRegionNode(VirtualMemoryManagerMetadata* meta) {
    // Pop from the free list
    if (!meta->freeList) return nullptr;
    VMMRegion* node = meta->freeList;
    meta->freeList = node->next;
    node->next = nullptr;
    return node;
}

static void FreeRegionNode(VirtualMemoryManagerMetadata* meta, VMMRegion* node) {
    // Push back to the free list
    node->next = meta->freeList;
    meta->freeList = node;
}

Status VirtualMemoryManagerInit(VirtualMemoryManagerState *vmm, PagingState *paging) {
    vmm->Paging = paging;

    // Calculate the amount of pages needed for metadata
    size_t metaSize = sizeof(VirtualMemoryManagerMetadata);
    size_t numPages = (metaSize + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;

    vmm->Metadata = (VirtualMemoryManagerMetadata*)PhysicalMemoryManagerAllocatePages(numPages);
    if (!vmm->Metadata) PANIC("Failed to allocate metadata!\n");

    // Now identity map the metadata, so we can access it
    for (size_t i = 0; i < numPages; i++) {
        void* vAddr = (void*)((size_t)vmm->Metadata + i * PMM_PAGE_SIZE);
        void* pAddr = (void*)((size_t)vmm->Metadata + i * PMM_PAGE_SIZE);

        // Map the metadata pages as writable, supervisor-only.
        if (PagingMapPage(vmm->Paging, vAddr, pAddr, true, false) != STATUS_SUCCESS) {
            PANIC("Failed to map metadata page!\n");
        }
    }

    // Initialize metadata
    vmm->Metadata->regions = nullptr;
    InitRegionPool(vmm->Metadata);
    return STATUS_SUCCESS;
}

void * VirtualMemoryManagerAllocate(VirtualMemoryManagerState *vmm, size_t size, bool writable, bool user) {
    size = ALIGN_UP(size, PMM_PAGE_SIZE);
    size_t numPages = size / PMM_PAGE_SIZE;

    void* vAddr = PhysicalMemoryManagerAllocatePages(numPages);
    if (!vAddr) return nullptr; // The allocation failed.

    // Identity map the allocated memory, so virtual == physical
    for (size_t i = 0; i < numPages; i++) {
        void* va = (void *)((size_t)vAddr + i * PMM_PAGE_SIZE);
        void* pa = (void *)((size_t)vAddr + i * PMM_PAGE_SIZE);
        if (PagingMapPage(vmm->Paging, va, pa, writable, user) != STATUS_SUCCESS) {
            PANIC("Failed to map page!\n");
        }
    }

    // Allocate region metadata
    VMMRegion* node = AllocateRegionNode(vmm->Metadata);
    if (!node) PANIC("No free region nodes!\n");

    node->start = vAddr;
    node->size = size;
    node->flags = (writable ? PAGE_WRITABLE : 0) | (user ? PAGE_USER : 0);

    // Insert at head
    node->next = vmm->Metadata->regions;
    vmm->Metadata->regions = node;

    return vAddr;
}

Status VirtualMemoryManagerFree(VirtualMemoryManagerState *vmm, void *addr, size_t size) {
    size = ALIGN_UP(size, PMM_PAGE_SIZE);
    size_t numPages = size / PMM_PAGE_SIZE;

    // Find the first region that matches in the linked list.
    VMMRegion* current = vmm->Metadata->regions;
    VMMRegion* previous = nullptr;
    while (current) {
        if (current->start == addr && current->size == size) break;
        previous = current;
        current = current->next;
    }

    if (!current) return STATUS_FAILURE;

    // Unmap pages
    for (size_t i = 0; i < numPages; i++) {
        void* va = (void *)((size_t)addr + i * PMM_PAGE_SIZE);
        if (PagingUnmapPage(vmm->Paging, va) != STATUS_SUCCESS) PANIC("Failed to unmap page!\n");
    }

    if (PhysicalMemoryManagerFreePages(addr, numPages) != STATUS_SUCCESS) PANIC("Failed to free physical pages!\n");

    // Remove from linked list
    if (previous) previous->next = current->next;
    else vmm->Metadata->regions = current->next;

    FreeRegionNode(vmm->Metadata, current);
    return STATUS_SUCCESS;
}

Status VirtualMemoryManagerTest(VirtualMemoryManagerState *vmm) {
    LOG(LOG_INFO, "Testing Virtual Memory Manager...\n");

    // Allocate an arbitrary block of memory
    size_t pages = 8;
    void* block = VirtualMemoryManagerAllocate(vmm, pages * PMM_PAGE_SIZE, true, false);
    if (!block) {
        PANIC("VMM Test: Allocation failed!\n");
    }

    PRINTF("\t* VMM Test: Allocated %z bytes at virtual address %p\n\n", pages * PMM_PAGE_SIZE, block);

    // Fill the block with data, and verify it's correct
    // We use volatile here to prevent the compiler from optimizing out our reads/writes
    volatile uint8_t* p = (uint8_t*)block;
    for (size_t i = 0; i < pages * PMM_PAGE_SIZE; i++) {
        p[i] = (uint8_t)(i & 0xFF);
    }

    for (size_t i = 0; i < pages * PMM_PAGE_SIZE; i++) {
        if (p[i] != (uint8_t)(i & 0xFF)) {
            PRINTF("VMM Test: Data mismatch at byte %z!\n", i);
            PANIC("VMM Test failed!\n");
        }
    }

    // Free the block
    if (VirtualMemoryManagerFree(vmm, block, pages * PMM_PAGE_SIZE) != STATUS_SUCCESS) {
        PANIC("VMM Test: Free failed!\n");
    }

    LOG(LOG_INFO, "Virtual Memory Manager test completed successfully.\n");
    return STATUS_SUCCESS;
}
