#include "PhysicalMemoryManager.h"

#include "Boot/MBParser.h"

typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    struct {
        uint64_t addr;
        uint64_t len;
        uint32_t type;
        uint32_t reserved;
    } entries[];
} PACKED MB2MemoryMap_t;

#define PMM_PAGE_MASK (~(PMM_PAGE_SIZE - 1))
#define PMM_ALIGN_DOWN(x, align) ((x) & PMM_PAGE_MASK)
#define PMM_ALIGN_UP(x, align) (((x) + (align) - 1) & PMM_PAGE_MASK)
#define PMM_MB2_TAG_TYPE_MMAP 6

Status PhysicalMemoryManagerInit(uint32_t InfoPtr, PhysicalMemoryManagerState *out) {
    if (!out) {
        return STATUS_FAILURE;
    }

    // Get the memory map from the multiboot info
    MB2MemoryMap_t* mmap = MBGetTag(InfoPtr, PMM_MB2_TAG_TYPE_MMAP);
    if (!mmap) {
        return STATUS_FAILURE;
    }

    uint32_t count = 0;
    struct {
        uint32_t addr;
        uint32_t len;
    } valid_regions[128];

    uint32_t total_memory = 0;

    for (uint8_t *entry_ptr = (uint8_t *)mmap->entries;
         entry_ptr < (uint8_t *)mmap + mmap->size;
         entry_ptr += mmap->entry_size) {
        struct multiboot_mmap_entry *entry = (struct multiboot_mmap_entry *)entry_ptr;

        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (entry->addr == 0 && entry->len < 0x100000) {
                // Skip memory below 1MB
                continue;
            }
            valid_regions[count].addr = entry->addr;
            valid_regions[count].len = entry->len;
            total_memory += entry->len;
            count++;
        }
    }

    // Now we need to find 2 regions for the allocation map and reserved map
    size_t needed_pages = (total_memory / PMM_PAGE_SIZE) / 8; // 1 bit per page
    uint32_t allocation_map_addr = 0;
    uint32_t reserved_map_addr = 0;

    for (uint32_t i = 0; i < count; i++) {
        if (valid_regions[i].len >= needed_pages * 2) {
            allocation_map_addr = valid_regions[i].addr;
            reserved_map_addr = valid_regions[i].addr + needed_pages;
            break;
        }
    }

    if (!allocation_map_addr || !reserved_map_addr) {
        return STATUS_FAILURE; // We couldn't find a suitable region
    }

    // Initialize the state
    out->TotalPages = total_memory / PMM_PAGE_SIZE;
    out->AllocationMap = (uint8_t *)allocation_map_addr;
    out->ReservedMap = (uint8_t *)reserved_map_addr;
    out->MapSize = needed_pages;

    // Now we need to mark these regions as reserved
    // Any region not marked in the valid regions is considered reserved
    // Or the region marked by allocation_map_addr/reserved_map_addr + their sizes
    for (uint32_t i = 0; i < out->TotalPages / 8; i++) {
        out->AllocationMap[i] = 0xFF; // Mark all as allocated
        out->ReservedMap[i] = 0xFF; // Mark all as reserved
    }

    for (uint32_t i = 0; i < count; i++) {
        uint32_t start_page = PMM_ALIGN_UP(valid_regions[i].addr, PMM_PAGE_SIZE) / PMM_PAGE_SIZE;
        uint32_t end_page = PMM_ALIGN_DOWN(valid_regions[i].addr + valid_regions[i].len, PMM_PAGE_SIZE) / PMM_PAGE_SIZE;

        for (uint32_t page = start_page; page < end_page; page++) {
            out->AllocationMap[page / 8] &= ~(1 << (page % 8)); // Mark as free
            out->ReservedMap[page / 8] &= ~(1 << (page % 8)); // Mark as not reserved
        }
    }

    // Now mark the allocation map and reserved map regions as reserved
    uint32_t alloc_start_page = allocation_map_addr / PMM_PAGE_SIZE;
    uint32_t alloc_end_page = (allocation_map_addr + needed_pages) / PMM_PAGE_SIZE;
    for (uint32_t page = alloc_start_page; page < alloc_end_page; page++) {
        out->ReservedMap[page / 8] |= (1 << (page % 8)); // Mark as reserved
        out->AllocationMap[page / 8] |= (1 << (page % 8)); // Mark as allocated
    }

    uint32_t resv_start_page = reserved_map_addr / PMM_PAGE_SIZE;
    uint32_t resv_end_page = (reserved_map_addr + needed_pages) / PMM_PAGE_SIZE;
    for (uint32_t page = resv_start_page; page < resv_end_page; page++) {
        out->ReservedMap[page / 8] |= (1 << (page % 8)); // Mark as reserved
        out->AllocationMap[page / 8] |= (1 << (page % 8)); // Mark as allocated
    }

    // Reserve the first 1MB of memory
    for (uint32_t page = 0; page < 256; page++) {
        out->ReservedMap[page / 8] |= (1 << (page % 8)); // Mark as reserved
        out->AllocationMap[page / 8] |= (1 << (page % 8)); // Mark as allocated
    }

    return STATUS_SUCCESS;
}

void * PhysicalMemoryManagerAllocatePage(PhysicalMemoryManagerState *state) {
    for (size_t i = 0; i < state->TotalPages; i++) {
        size_t byte_index = i / 8;
        size_t bit_index = i % 8;

        if (!(state->AllocationMap[byte_index] & (1 << bit_index)) && (!(state->ReservedMap[byte_index] & (1 << bit_index)))) {
            // Found a free page, mark it as allocated
            state->AllocationMap[byte_index] |= (1 << bit_index);
            return (void *)(i * PMM_PAGE_SIZE); // Return the address of the allocated page
        }
    }

    return nullptr; // No free pages available
}

Status PhysicalMemoryManagerFreePage(PhysicalMemoryManagerState *state, void *page) {
    size_t address = (size_t)page;
    if (address % PMM_PAGE_SIZE != 0) {
        return STATUS_FAILURE; // Page is not aligned to page size, cannot free
    }

    size_t page_index = address / PMM_PAGE_SIZE;
    size_t byte_index = page_index / 8;
    size_t bit_index = page_index % 8;

    if (state->ReservedMap[byte_index] & (1 << bit_index)) {
        return STATUS_FAILURE; // Attempting to free a reserved page, which is not allowed
    }
    if (!(state->AllocationMap[byte_index] & (1 << bit_index))) {
        return STATUS_FAILURE; // Page is not allocated, cannot free
    }

    state->AllocationMap[byte_index] &= ~(1 << bit_index); // Mark the page as free

    return STATUS_SUCCESS;
}

#include "Core/Kernel.h"

Status PhysicalMemoryManagerTest(PhysicalMemoryManagerState *state, KernelState *kernel) {
    kernel->Printf(kernel, "Testing Physical Memory Manager...\n");
    void* page1 = PhysicalMemoryManagerAllocatePage(state);
    if (!page1) {
        PANIC(kernel, "Failed to allocate page 1!\n");
    }

    void* page2 = PhysicalMemoryManagerAllocatePage(state);
    if (!page2) {
        PANIC(kernel, "Failed to allocate page 2!\n");
    }

    if (page1 == page2) {
        PANIC(kernel, "Allocated the same page twice!\n");
    }

    // Write some data to the pages
    volatile uint8_t* p1 = (uint8_t*)page1;
    volatile uint8_t* p2 = (uint8_t*)page2;
    for (size_t i = 0; i < PMM_PAGE_SIZE; i++) {
        p1[i] = 0xAA;
        p2[i] = 0x55;
    }

    bool success = true;
    for (size_t i = 0; i < PMM_PAGE_SIZE; i++) {
        if (p1[i] != 0xAA) {
            success = false;
            break;
        }
        if (p2[i] != 0x55) {
            success = false;
            break;
        }
    }
    if (!success) {
        PANIC(kernel, "Memory test failed! Data corruption detected!\n");
    }
    kernel->Printf(kernel, "Memory test passed! Freeing pages...\n");

    if (PhysicalMemoryManagerFreePage(state, page1) != STATUS_SUCCESS) {
        PANIC(kernel, "Failed to free page 1!\n");
    }

    if (PhysicalMemoryManagerFreePage(state, page2) != STATUS_SUCCESS) {
        PANIC(kernel, "Failed to free page 2!\n");
    }

    kernel->Printf(kernel, "Pages freed successfully.\n");
    return STATUS_SUCCESS;
}
