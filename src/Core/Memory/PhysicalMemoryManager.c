#include "PhysicalMemoryManager.h"

#include "Boot/MBParser.h"
#include "Core/Kernel.h"

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
#define PMM_MB2_TAG_TYPE_MMAP 6

PhysicalMemoryManagerState KernelPhysicalMemoryManager = {};

Status PhysicalMemoryManagerInit(uint32_t InfoPtr) {
    // Get the memory map from the multiboot info
    MB2MemoryMap_t* mmap = MBGetTag(InfoPtr, PMM_MB2_TAG_TYPE_MMAP);
    if (!mmap) {
        return STATUS_FAILURE;
    }

    uint32_t count = 0;

    // Collect all valid memory regions, skipping those below 1MB
    for (uint8_t *entry_ptr = (uint8_t *)mmap->entries;
         entry_ptr < (uint8_t *)mmap + mmap->size;
         entry_ptr += mmap->entry_size) {
        count++;
    }

    struct {
        uint64_t addr;
        uint64_t len;
    } validRegions[count];
    size_t totalMemory = 0;
    size_t validCount = 0;

    for (uint8_t *entry_ptr = (uint8_t *)mmap->entries;
         entry_ptr < (uint8_t *)mmap + mmap->size;
         entry_ptr += mmap->entry_size) {
        struct multiboot_mmap_entry *entry = (struct multiboot_mmap_entry *)entry_ptr;

        // We only care about available RAM regions
        if (entry->type != MULTIBOOT_MEMORY_AVAILABLE) {
            continue;
        }

        // Skip regions below 1MB
        if (entry->addr + entry->len <= 1 * MiB) {
            continue;
        }

        validRegions[validCount].addr = entry->addr;
        validRegions[validCount].len = entry->len;
        totalMemory += entry->len;
        validCount++;
    }

    // We need to find the space for our bitmaps, which will be 2 byte arrays, where each bit represents a page
    size_t totalPages = totalMemory / PMM_PAGE_SIZE;
    size_t bitmapSizeBytes = (totalPages + 7) / 8; // Round up to nearest byte
    bitmapSizeBytes = ALIGN_UP(bitmapSizeBytes, PMM_PAGE_SIZE); // Align to page size
    size_t necessaryBitmapSize = bitmapSizeBytes * 2; // We need 2 bitmaps: one for allocation, one for reservation
    void* bitmapMemory = nullptr; // size of necessaryBitmapSize

    for (size_t i = 0; i < validCount; i++) {
        size_t region_start = ALIGN_UP(validRegions[i].addr, PMM_PAGE_SIZE);
        size_t region_end = ALIGN_DOWN(validRegions[i].addr + validRegions[i].len, PMM_PAGE_SIZE);
        size_t region_size = (region_end > region_start) ? (region_end - region_start) : 0;

        if (region_size >= necessaryBitmapSize) {
            bitmapMemory = (void *)(uintptr_t)region_start;
            validRegions[i].addr = region_start + necessaryBitmapSize;
            validRegions[i].len = region_size - necessaryBitmapSize;
            if (validRegions[i].len + validRegions[i].addr > UINT32_MAX) {
                validRegions[i].len = UINT32_MAX - validRegions[i].addr;
            }
            break;
        }
    }

    if (!bitmapMemory) {
        return STATUS_FAILURE; // Could not find space for bitmaps
    }
    KernelPhysicalMemoryManager.TotalPages = totalPages;
    KernelPhysicalMemoryManager.AllocationMap = (uint8_t *)bitmapMemory;
    KernelPhysicalMemoryManager.ReservedMap = (uint8_t *)bitmapMemory + bitmapSizeBytes;

    // Set all bits to F (reserved) initially
    for (size_t i = 0; i < bitmapSizeBytes; i++) {
        KernelPhysicalMemoryManager.AllocationMap[i] = 0xFF;
        KernelPhysicalMemoryManager.ReservedMap[i] = 0xFF;
    }

    // Now mark all valid regions as free
    for (size_t i = 0; i < validCount; i++) {
        size_t region_start = ALIGN_UP(validRegions[i].addr, PMM_PAGE_SIZE);
        size_t region_end = ALIGN_DOWN(validRegions[i].addr + validRegions[i].len, PMM_PAGE_SIZE);
        if (region_start > UINT32_MAX) {
            // Region starts above 4GB, skip
            continue;
        }

        if (region_end > UINT32_MAX) {
            region_end = UINT32_MAX;
        }

        size_t start_page = region_start / PMM_PAGE_SIZE;
        size_t end_page = region_end / PMM_PAGE_SIZE;
        for (size_t page = start_page; page < end_page; page++) {
            KernelPhysicalMemoryManager.ReservedMap[page / 8] &= ~(1 << (page % 8)); // Mark as unreserved
        }
    }

    // Reserve the first 1MB of memory
    for (size_t page = 0; page < (1 * MiB) / PMM_PAGE_SIZE; page++) {
        KernelPhysicalMemoryManager.ReservedMap[page / 8] |= (1 << (page % 8)); // Mark as reserved
    }

    // Reserve the 2 bitmaps
    size_t bitmap_start_page = (size_t)(uintptr_t)bitmapMemory / PMM_PAGE_SIZE;
    size_t bitmap_end_page = bitmap_start_page + BYTES_TO_PAGES(necessaryBitmapSize);
    for (size_t page = bitmap_start_page; page < bitmap_end_page; page++) {
        KernelPhysicalMemoryManager.ReservedMap[page / 8] |= (1 << (page % 8)); // Mark as reserved
        KernelPhysicalMemoryManager.AllocationMap[page / 8] |= (1 << (page % 8)); // Mark as allocated
    }

    // Free up the allocation map for use (now that we've reserved necessary regions)
    for (size_t i = 0; i < bitmapSizeBytes; i++) {
        KernelPhysicalMemoryManager.AllocationMap[i] = 0x00;
    }

    struct multiboot_tag_module* initrdTag = (struct multiboot_tag_module*)MBGetTag(InfoPtr, MULTIBOOT_TAG_TYPE_MODULE);
    if (initrdTag) {
        PhysicalMemoryManagerReserveRegion((void*)(uintptr_t)initrdTag->mod_start, BYTES_TO_PAGES(initrdTag->mod_end - initrdTag->mod_start));
    }

    return STATUS_SUCCESS;
}

void PhysicalMemoryManagerReserveRegion(void *startAddr, size_t size) {
    // Reserve a region of physical memory
    size_t start_address = (size_t)startAddr;
    size_t start_page = start_address / PMM_PAGE_SIZE;
    size_t end_page = start_page + size;
    for (size_t page = start_page; page < end_page; page++) {
        KernelPhysicalMemoryManager.ReservedMap[page / 8] |= (1 << (page % 8)); // Mark as reserved
    }
}

uint64_t PhysicalMemoryManagerCountFreePages() {
    uint64_t freePages = 0;
    for (size_t i = 0; i < KernelPhysicalMemoryManager.TotalPages; i++) {
        size_t byte_index = i / 8;
        size_t bit_index = i % 8;

        // Check if the page is free
        if (!(KernelPhysicalMemoryManager.AllocationMap[byte_index] & (1 << bit_index)) && (!(KernelPhysicalMemoryManager.ReservedMap[byte_index] & (1 << bit_index)))) {
            freePages++;
        }
    }
    return freePages;
}

void * PhysicalMemoryManagerAllocatePage() {
    for (size_t i = 0; i < KernelPhysicalMemoryManager.TotalPages; i++) {
        size_t byte_index = i / 8;
        size_t bit_index = i % 8;

        // Check if the page is free and not reserved
        if (!(KernelPhysicalMemoryManager.AllocationMap[byte_index] & (1 << bit_index)) && (!(KernelPhysicalMemoryManager.ReservedMap[byte_index] & (1 << bit_index)))) {
            // Found a free page, mark it as allocated
            KernelPhysicalMemoryManager.AllocationMap[byte_index] |= (1 << bit_index);
            return (void *)(i * PMM_PAGE_SIZE); // Return the address of the allocated page
        }
    }

    LOGF(LOG_WARNING, "PhysicalMemoryManagerAllocatePage: Out of memory! No free pages available. Used %u out of %u pages.\n",
        KernelPhysicalMemoryManager.TotalPages - PhysicalMemoryManagerCountFreePages(),
        KernelPhysicalMemoryManager.TotalPages
    );

    return nullptr; // No free pages available
}

void * PhysicalMemoryManagerAllocatePages(size_t numPages) {
    void* firstPage = nullptr;
    size_t contiguousCount = 0;

    // Look for a sequence of free pages
    for (size_t i = 0; i < KernelPhysicalMemoryManager.TotalPages; i++) {
        size_t byte_index = i / 8;
        size_t bit_index = i % 8;

        if (!(KernelPhysicalMemoryManager.AllocationMap[byte_index] & (1 << bit_index)) && (!(KernelPhysicalMemoryManager.ReservedMap[byte_index] & (1 << bit_index)))) {
            // Found a free page
            if (contiguousCount == 0) {
                firstPage = (void *)(i * PMM_PAGE_SIZE);
            }
            contiguousCount++;

            // We have enough contiguous pages
            if (contiguousCount == numPages) {
                // Mark pages as allocated
                for (size_t j = 0; j < numPages; j++) {
                    size_t pageIndex = (size_t)firstPage / PMM_PAGE_SIZE + j;
                    size_t bIndex = pageIndex / 8;
                    size_t btIndex = pageIndex % 8;
                    KernelPhysicalMemoryManager.AllocationMap[bIndex] |= (1 << btIndex);
                }
                return firstPage;
            }
        } else {
            // Reset count if we hit an allocated or reserved page
            contiguousCount = 0;
            firstPage = nullptr;
        }
    }

    return nullptr; // Not enough contiguous pages available
}

Status PhysicalMemoryManagerFreePage(void *page) {
    size_t address = (size_t)page;
    if (address % PMM_PAGE_SIZE != 0) {
        return STATUS_FAILURE; // Page is not aligned to page size, cannot free
    }

    size_t page_index = address / PMM_PAGE_SIZE;
    size_t byte_index = page_index / 8;
    size_t bit_index = page_index % 8;

    // Check if the bit is reserved or not allocated
    if (KernelPhysicalMemoryManager.ReservedMap[byte_index] & (1 << bit_index)) {
        return STATUS_FAILURE; // Attempting to free a reserved page, which is not allowed
    }
    if (!(KernelPhysicalMemoryManager.AllocationMap[byte_index] & (1 << bit_index))) {
        return STATUS_FAILURE; // Page is not allocated, cannot free
    }

    KernelPhysicalMemoryManager.AllocationMap[byte_index] &= ~(1 << bit_index); // Mark the page as free

    return STATUS_SUCCESS;
}

Status PhysicalMemoryManagerFreePages(void *startPage, size_t numPages) {
    size_t address = (size_t)startPage;
    if (address % PMM_PAGE_SIZE != 0) {
        return STATUS_FAILURE; // Start page is not aligned to page size, cannot free
    }

    size_t start_page_index = address / PMM_PAGE_SIZE;
    size_t end_page_index = start_page_index + numPages;

    // For everything between start and end page, check if any are reserved or not allocated
    for (size_t i = start_page_index; i < end_page_index; i++) {
        size_t b_index = i / 8;
        size_t bt_index = i % 8;
        if (KernelPhysicalMemoryManager.ReservedMap[b_index] & (1 << bt_index)) {
            return STATUS_FAILURE; // Attempting to free a reserved page, which is not allowed
        }
        if (!(KernelPhysicalMemoryManager.AllocationMap[b_index] & (1 << bt_index))) {
            return STATUS_FAILURE; // Page is not allocated, cannot free
        }
    }

    // now mark them all as free
    for (size_t i = start_page_index; i < end_page_index; i++) {
        size_t b_index = i / 8;
        size_t bt_index = i % 8;
        KernelPhysicalMemoryManager.AllocationMap[b_index] &= ~(1 << bt_index); // Mark the page as free
    }

    return STATUS_SUCCESS;
}

Status PhysicalMemoryManagerTest(void) {
    LOG(LOG_INFO, "Testing Physical Memory Manager...\n");
  
    void* page1 = PhysicalMemoryManagerAllocatePage();
    if (!page1) {
        PANIC("Failed to allocate page 1!\n");
    }

    void* page2 = PhysicalMemoryManagerAllocatePage();
    if (!page2) {
        PANIC("Failed to allocate page 2!\n");
    }

    if (page1 == page2) {
        PANIC("Allocated the same page twice!\n");
    }

    // Write some data to the pages to verify they are usable
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
        PANIC("Memory test failed! Data corruption detected!\n");
    }
    LOG(LOG_INFO, "Memory test passed! Freeing pages...\n");

    if (PhysicalMemoryManagerFreePage(page1) != STATUS_SUCCESS) {
        PANIC("Failed to free page 1!\n");
    }

    if (PhysicalMemoryManagerFreePage(page2) != STATUS_SUCCESS) {
        PANIC("Failed to free page 2!\n");
    }

    PRINT("\t* Pages freed successfully.\n\n");
    return STATUS_SUCCESS;
}
