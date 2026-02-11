#include "PMM.h"

#include <Settings.h>
#include "Kernel.h"
#include "../KernelData.h"

namespace Memory {
    void PMM::Initialize() {
        // Make sure a memory map and a higher hald offset are available
        if (!memmap_request.response) PANIC("Limine did not provide a memory map!");
        if (!hhdm_request.response) PANIC("Limine did not provide a higher-half offset!");

        limineMemmapResponse = memmap_request.response;
        higherHalfOffset = hhdm_request.response->offset;
        LOG_DEBUG("Limine memory map response contains %u64 entries, at %p.", limineMemmapResponse->entry_count, memmap_request.response);
        LOG_DEBUG("Higher half offset is %p.", hhdm_request.response->offset);

        struct {
            uint64_t addr;
            uint64_t length;
        } validRegions[limineMemmapResponse->entry_count];

        // Calculate the number of valid regions, usable frame count, usable memory, and bitmap size
        uint64_t validRegionCount = 0;
        uint64_t totalMemory = 0;

        for (uint64_t i = 0; i < limineMemmapResponse->entry_count; i++) {
            limine_memmap_entry* entry = limineMemmapResponse->entries[i];
            
            // Skip entries that are unusable or below 1MB
            if (entry->type != LIMINE_MEMMAP_USABLE) continue;
            if (entry->base + entry->length <= 1 * Constants::MiB) continue;

            // The entry is writable and abbove 1MB, so we can store it
            validRegions[validRegionCount].addr = entry->base;
            validRegions[validRegionCount].length = entry->length;
            totalMemory += entry->length;
            validRegionCount++;
        }

        usableFrames = totalMemory / Architecture::KernelPageSize;
        bitmapSize = (usableFrames + 7) / 8; // Round up to the nearest byte
        bitmapSize = ALIGN_UP(bitmapSize, Architecture::KernelPageSize);
        LOG_DEBUG("Memory map contains %u64 usable memory regions and %u64 usable frames, totaling %u64 bytes (%u64 KiB).", validRegionCount, usableFrames, totalMemory, totalMemory / Constants::KiB);
        LOG_DEBUG("Memory bitmap size is %u64 bytes.", bitmapSize);

        // Find space to store our two bitmaps
        uint64_t requiredMapStorageSize = bitmapSize * 2; // We need 2 bitmaps; one for reserved and one for allocatable
        void* bitmapMemory = nullptr;

        for (uint64_t regionIndex = 0; regionIndex < validRegionCount; regionIndex++) {
            uint64_t regionStart = ALIGN_UP(validRegions[regionIndex].addr, Architecture::KernelPageSize);
            uint64_t regionEnd = ALIGN_DOWN(validRegions[regionIndex].addr + validRegions[regionIndex].length, Architecture::KernelPageSize);
            uint64_t regionSize = (regionEnd > regionStart) ? (regionEnd - regionStart) : 0;
            
            // Is the region large enough?
            if (regionSize >= requiredMapStorageSize) {
                LOG_DEBUG("Memory region #%u64 (%p) is big enough to store the memory bitmaps (%u64 >= %u64).", regionIndex, regionStart, regionSize, requiredMapStorageSize);

                // Redefine the valid region we're using so that it excludes the bitmaps
                bitmapMemory = (void*)(uintptr_t)regionStart;
                validRegions[regionIndex].addr = regionStart + requiredMapStorageSize;
                validRegions[regionIndex].length = regionSize - requiredMapStorageSize;
                LOG_DEBUG("Bitmap memory starts at address %p; valid region address changed to %p with size %u64.", bitmapMemory, validRegions[regionIndex].addr, validRegions[regionIndex].length);
                break;
            }
        }

        // Panic if no space was found for the bitmaps
        if (!bitmapMemory) {
            PANIC("No valid memory region found to store PMM bitmaps!");
        }

        // Allocate the bitmaps
        allocatableBitmap = (uint8_t*)bitmapMemory;
        reservedBitmap = (uint8_t *)bitmapMemory + bitmapSize;
        LOG_DEBUG("Allocatable bitmap is at address %p, reserved bitmap is at address %p.", allocatableBitmap, reservedBitmap);

        // Mark all bits as reserved
        for(uint64_t byteIndex = 0; byteIndex < bitmapSize; byteIndex++) {
            (allocatableBitmap + higherHalfOffset)[byteIndex] = 0xFF;
            (reservedBitmap + higherHalfOffset)[byteIndex] = 0xFF;
        }

        // Mark all valid regions as free
        for(uint64_t regionIndex = 0; regionIndex < validRegionCount; regionIndex++) {
            uint64_t regionStart = ALIGN_UP(validRegions[regionIndex].addr, Architecture::KernelPageSize);
            uint64_t regionEnd = ALIGN_DOWN(validRegions[regionIndex].addr + validRegions[regionIndex].length, Architecture::KernelPageSize);
            uint64_t startPage = regionStart / Architecture::KernelPageSize;
            uint64_t endPage = regionEnd / Architecture::KernelPageSize;

            // Mark all bits in this page as free
            for (uint64_t page = startPage; page < endPage; page++) {
                (reservedBitmap + higherHalfOffset)[page / 8] &= ~(1 << (page % 8));
            }
        }

        // Mark the first 1MB of memory as reserved, critical data already lives there!
        for (uint64_t page = 0; page < (1 * Constants::MiB) / Architecture::KernelPageSize; page++) {
            (reservedBitmap + higherHalfOffset)[page / 8] |= (1 << (page % 8));
        }

        // Reserve the two bitmaps
        size_t bitmapStartPage = (size_t)(uintptr_t)bitmapMemory / Architecture::KernelPageSize;
        size_t bitmapEndPage = bitmapStartPage + (ALIGN_UP(requiredMapStorageSize, Architecture::KernelPageSize) / Architecture::KernelPageSize);
        for (size_t page = bitmapStartPage; page < bitmapEndPage; page++) {
            (reservedBitmap + higherHalfOffset)[page / 8] |= (1 << (page % 8)); // Mark as reserved
        }

        // Free up the allocation bitmap for use now that we know what's reserved
        for (uint64_t i = 0; i < bitmapSize; i++) {
            (allocatableBitmap + higherHalfOffset)[i] = 0x00;
        }

        LOG_INFO("Configured bitmaps.");

        // Reserve memory for limine modules, as they are loaded into memory and we don't want to hand them out
        if (module_request.response) {
            limine_module_response* modules = module_request.response;

            for (uint64_t i = 0; i < modules->module_count; i++) {
                limine_file* mod = modules->modules[i];
                uint64_t physStart = (uint64_t)mod->address;
                uint64_t physLength = mod->size;
                PMM::ReserveRegion((void*)physStart, (ALIGN_UP(physLength, Architecture::KernelPageSize) / Architecture::KernelPageSize));
            }
        }

        LOG_INFO("Limine modules marked as reserved.");

        // Test the PMM if testing mode is enabled
        #if SETTING_TEST_MODE
        if (PMM::TestPMM() != STATUS::SUCCESS) PANIC("PMM test failed!");
        #endif
    }

    // Mark a region of physical memory as reserved
    void PMM::ReserveRegion(void *startAddr, uint64_t size) {
        uint64_t startPage = (uint64_t)startAddr / Architecture::KernelPageSize;
        uint64_t endPage = startPage + size;
        
        // Mark the memory as reserved
        for (uint64_t page = startPage; page < endPage; page++) {
            (reservedBitmap + higherHalfOffset)[page / 8] |= (1 << (page % 8));
        }
    }

    // Allocate X pages of physical memory
    uintptr_t PMM::AllocatePages(uint32_t numPages) {
        if (numPages == 0) return 0;

        uint64_t totalPages = bitmapSize * 8;
        uint64_t runStart = 0;
        uint64_t runLength = 0;

        // Scan for a contiguous memory block that's large enough to hold the requested number of pages
        for (uint64_t page = 0; page < totalPages; page++) {
            uint64_t byteIndex = page / 8;
            uint8_t  bitIndex  = page % 8;

            if (!PMM::IsPageReserved(page) && !PMM::IsPageAllocated(page)) {
                if (runLength == 0) runStart = page;
                runLength++;

                // Stop searching early if we found a block that's big enough
                if (runLength == numPages) break;
            } else {
                runLength = 0;
            }
        }

        // Return NULL if no suitable block was found
        if (runLength < numPages)
            return 0;

        // Once we know that we have a big enough block, we can allocate it
        for (uint64_t page = runStart; page < runStart + numPages; page++) {
            uint64_t byteIndex = page / 8;
            uint8_t  bitIndex  = page % 8;
            (allocatableBitmap + higherHalfOffset)[byteIndex] |= (1 << bitIndex);
        }

        // Return the address of the allocation
        return runStart * Architecture::KernelPageSize;
    }

    // Free X pages, starting at an aligned address
    // NOTE: startAddr must be a physical address, it cannot have the higher half offset added to it
    void PMM::FreePages(uint64_t startAddr, uint32_t numPages) {
        // At least 1 page needs to be freed
        if (numPages <= 0) PANIC("At least one page needs to be freed!");

        // Make sure the starting address is aligned to the kernel page size
        if ((startAddr & (Architecture::KernelPageSize - 1)) != 0) PANIC("Cannot free unaligned pages!");

        uint64_t startPage = (uint64_t)startAddr / Architecture::KernelPageSize;
        uint64_t endPage = startPage + numPages;
        
        // Make sure the end page is within the managed memory range
        if (endPage > bitmapSize * 8) PANIC("End page is outside of managed memory range!");

        for (uint64_t page = startPage; page < endPage; page++) {
            uint64_t byteIndex = page / 8;
            uint8_t bitIndex = page % 8;

            // Panic if the current page is reserved or not allocated
            if (PMM::IsPageReserved(page)) PANIC("Invalid attempt to free reserved pages!");
            if (!PMM::IsPageAllocated(page)) PANIC("Invalid attempt to free unallocated pages!");

            // Mark the page as unallocated
            (allocatableBitmap + higherHalfOffset)[byteIndex] &= ~(1 << bitIndex);
        }
    }

    // Returns the allocation status of a page
    bool PMM::IsPageAllocated(uint64_t pageIndex) {
        uint64_t byteIndex = pageIndex / 8;
        uint8_t bitIndex = pageIndex % 8;
        return (allocatableBitmap + higherHalfOffset)[byteIndex] & (1 << bitIndex);
    }

    // Returns the reserved status of a page
    bool PMM::IsPageReserved(uint64_t pageIndex) {
        uint64_t byteIndex = pageIndex / 8;
        uint8_t bitIndex = pageIndex % 8;
        return (reservedBitmap + higherHalfOffset)[byteIndex] & (1 << bitIndex);
    }

    // Test the PMM to make sure allocation and freeing is propely working
    STATUS PMM::TestPMM() {
        // First, try allocate a single page
        LOG_DEBUG("Testing PMM single-page allocation...");
        uintptr_t singleAlloc = PMM::AllocatePages(1);
        if (!singleAlloc) return STATUS::FAILURE;
        LOG_DEBUG("PMM single-page test allocation succeeded with address %p.", singleAlloc);

        // Allocate another page and make sure it's not the same address as the previous one
        uintptr_t singleAlloc2 = PMM::AllocatePages(1);
        if (!singleAlloc2 || singleAlloc2 == singleAlloc) return STATUS::FAILURE;
        LOG_DEBUG("PMM second single-page test allocation succeeded with address %p.", singleAlloc2);

        // Now try to allocate a continuous chunk
        LOG_DEBUG("Testing PMM multi-page allocation...");
        uintptr_t multiAlloc = PMM::AllocatePages(4);
        if (!multiAlloc) return STATUS::FAILURE;
        LOG_DEBUG("PMM multi-page test allocation succeeded with address %p.", multiAlloc);

        // Try to allocate a massive 10MiB continuous chunk
        LOG_DEBUG("Testing PMM large allocation...");
        uintptr_t massiveMultiAlloc = PMM::AllocatePages(2560);
        if (!massiveMultiAlloc) return STATUS::FAILURE;
        LOG_DEBUG("PMM 10 MiB multi-page test allocation succeeded with address %p.", massiveMultiAlloc);

        // All of the allocation tests passed, now we need to try writing to and reading from the allocated areas
        uint8_t* memory = (uint8_t*)(massiveMultiAlloc + higherHalfOffset);
        uint64_t writeCount = 2560 * Architecture::KernelPageSize;
        for (uint64_t i = 0; i < writeCount; i++) {
            *memory = 0xAE;
            if (*memory != 0xAE) return STATUS::FAILURE;
            memory++;
        }

        // Test single-page freeing for 1 page by:
        //  Allocating a page and storing its address
        //  Freeing the page we just allocated
        //  Allocating a new page and checking if its address is the same as the first allocation
        LOG_DEBUG("Testing PMM single-page freeing...");
        uintptr_t singleFree = PMM::AllocatePages(1);
        uintptr_t allocAddr = singleFree;
        PMM::FreePages(singleFree, 1);
        
        singleFree = PMM::AllocatePages(1);
        if (singleFree != allocAddr) return STATUS::FAILURE;
        LOG_DEBUG("PMM single-page freeing works as expected.");

        // Do the same test for multiple pages
        LOG_DEBUG("Testing PMM multi-page freeing...");
        uintptr_t multiFree = PMM::AllocatePages(4);
        allocAddr = multiFree;
        PMM::FreePages(multiFree, 4);

        multiFree = PMM::AllocatePages(4);
        if(multiFree != allocAddr) return STATUS::FAILURE;
        LOG_DEBUG("PMM multi-page freeing works as expected.");

        // Now test for proper run length resetting by:
        //  Allocating 2 pages each for "A" and "B"
        //  Freeing "A"
        //  Allocating 4 pages for "C", it should allocate after "B"
        LOG_DEBUG("Testing PMM run length resetting..");
        uintptr_t rlrAllocA = PMM::AllocatePages(2);
        uintptr_t rlrAllocB = PMM::AllocatePages(2);
        PMM::FreePages(rlrAllocA, 2);

        uintptr_t rlrAllocC = PMM::AllocatePages(4);
        if (rlrAllocC <= rlrAllocB) return STATUS::FAILURE;
        LOG_DEBUG("PMM run length resetting works as expected.");

        // Make sure the allocator reuses the lowest suitable free range by:
        //  Allocating 2 pages each for "A" and "B"
        //  Freeing "A"
        //  Allocating 2 pages for "C", the PMM should allocate where "A" was
        LOG_DEBUG("Testing PMM lowest-address reuse...");
        uintptr_t lsfrAllocA = PMM::AllocatePages(2);
        uintptr_t lsfrAllocB = PMM::AllocatePages(2);
        PMM::FreePages(lsfrAllocA, 2);

        uintptr_t lsfrAllocC = PMM::AllocatePages(2);
        if (lsfrAllocC >= lsfrAllocB) return STATUS::FAILURE;
        LOG_DEBUG("The PMM can reuse the lowest suitable address range.");

        // Perform the same test but with 20MiB of data
        LOG_DEBUG("Testing PMM lowest-address reuse for large data...");
        uintptr_t blsfrAllocA = PMM::AllocatePages(5120);
        uintptr_t blsfrAllocB = PMM::AllocatePages(5120);
        PMM::FreePages(blsfrAllocA, 5120);

        uintptr_t blsfrAllocC = PMM::AllocatePages(5120);
        if (blsfrAllocC >= blsfrAllocB) return STATUS::FAILURE;
        LOG_DEBUG("The PMM can reuse the lowest suitable address range for large data.");

        // All tests passed, ts somehow works!!!
        // Now we'll have to free all the allocations we did
        LOG_DEBUG("Cleaning up test allocations...");
        
        // Alloc tests
        PMM::FreePages(singleAlloc, 1);
        PMM::FreePages(singleAlloc2, 1);
        PMM::FreePages(multiAlloc, 4);
        PMM::FreePages(massiveMultiAlloc, 2560);
        
        // Free tests
        PMM::FreePages(singleFree, 1);
        PMM::FreePages(multiFree, 4);

        // Run length reset test, A is already freed
        PMM::FreePages(rlrAllocB, 2);
        PMM::FreePages(rlrAllocC, 4);

        // Lowest suitable free range reusal test, A is already freed
        PMM::FreePages(lsfrAllocB, 2);
        PMM::FreePages(lsfrAllocC, 2);

        // Large lowest suitable free range reusal test, A is already freed
        PMM::FreePages(blsfrAllocB, 5120);
        PMM::FreePages(blsfrAllocC, 5120);
        return STATUS::SUCCESS;
    }
} // Memory
