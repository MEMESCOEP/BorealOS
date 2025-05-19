#include "PhysicalAllocator.h"

#include "Memory.h"
#include "../../Kernel.h"

PHYSICAL_MEMSTATE PhysicalMemoryState = {
    .TotalBlocks = 0,
    .FreeBlocks = 0,
    .BlockSize = 0,
    .AllocatedMask = NULL,
    .AllocationSizes = NULL,
    .HHDMOffset = 0,
    .Lock = {0},
};

void InitPhysicalMemoryAllocator() {
    SpinlockInit(&PhysicalMemoryState.Lock);

    struct limine_memmap_response *response = GetMEMMapResponse();

    // I would hav liked this to be in the memory.c file but it requires some special calculations so we're doing it here.
    struct limine_memmap_entry *selected_entry = NULL;
    for (uint64_t i = 0; i < response->entry_count; i++) {
        struct limine_memmap_entry *entry = response->entries[i];

        size_t entry_blocks = entry->length / PAGE_SIZE;
        size_t bitmap_size = (entry_blocks + 7) / 8;
        size_t allocation_sizes_size = entry_blocks * sizeof(uint32_t);
        size_t metadata_size = bitmap_size + allocation_sizes_size;

        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= metadata_size + PAGE_SIZE) {
            selected_entry = entry;
            break;
        }
    }

    if (!selected_entry) {
        KernelPanic(PHYSICALALLOCATOR_ERROR_NO_APPROPRIATE_MEMMAP_ENTRY, "Failed to find a suitable memory region for physical memory allocation. Do you have more than 2MB of memory?");
        return;
    }

    size_t total_blocks = (selected_entry->length / PAGE_SIZE);
    size_t bitmap_size = (total_blocks + 7) / 8;
    size_t allocation_sizes_size = total_blocks * sizeof(uint32_t);
    size_t metadata_size = bitmap_size + allocation_sizes_size;

    while (selected_entry->length < metadata_size + PAGE_SIZE) {
        total_blocks--;
        bitmap_size = (total_blocks + 7) / 8;
        allocation_sizes_size = total_blocks * sizeof(uint32_t);
        metadata_size = bitmap_size + allocation_sizes_size;
    }

    // Set up metadata and base address
    PhysicalMemoryState.AllocatedMask = (uint8_t*)(selected_entry->base);
    PhysicalMemoryState.AllocationSizes = (uint64_t*)((uintptr_t)PhysicalMemoryState.AllocatedMask + bitmap_size);
    PhysicalMemoryState.BlockSize = PAGE_SIZE;
    PhysicalMemoryState.TotalBlocks = total_blocks;
    PhysicalMemoryState.FreeBlocks = total_blocks;
    PhysicalMemoryState.HHDMOffset = selected_entry->base;
    selected_entry->type = LIMINE_MEMMAP_RESERVED;
    selected_entry->base += metadata_size;
    selected_entry->length -= metadata_size;
    MemSet(PhysicalMemoryState.AllocatedMask, 0, bitmap_size);
    MemSet(PhysicalMemoryState.AllocationSizes, 0, allocation_sizes_size);

    // We are now ready to allocate memory
}

void TestPhysicalMemoryAllocator() {
}
