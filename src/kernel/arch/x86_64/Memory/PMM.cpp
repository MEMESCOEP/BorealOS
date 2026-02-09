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

                if (validRegions[regionIndex].length + validRegions[regionIndex].addr > UINT64_MAX) {
                    LOG_WARNING("Bitmap region length is greater than %u64, it will be resized to %u64!", UINT64_MAX, UINT64_MAX - validRegions[regionIndex].addr);
                    validRegions[regionIndex].length = UINT64_MAX - validRegions[regionIndex].addr;
                }

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
        size_t bitmapEndPage = bitmapStartPage + BYTES_TO_PAGES(requiredMapStorageSize);
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

                PMM::ReserveRegion((void*)physStart, BYTES_TO_PAGES(physLength));
            }
        }

        LOG_INFO("Limine modules marked as reserved.");

        // Test the PMM if testing mode is enabled
        if (PMM::TestPMM() != STATUS::SUCCESS) PANIC("PMM test failed!");
    }

    // Mark a region of physical memory as reserved
    void PMM::ReserveRegion(void *startAddr, uint64_t size) {
        uint64_t startPage = (uint64_t)startAddr / Architecture::KernelPageSize;
        uint64_t endPage = startPage + size;
        
        // Mark the memory as reserved
        for (uint64_t page = startPage; page < endPage; page++) {
            (PMM::reservedBitmap + higherHalfOffset)[page / 8] |= (1 << (page % 8));
        }
    }

    // Allocate X pages of physical memory
    uintptr_t PMM::AllocatePages(uint32_t numPages) {
        if (numPages == 0) return 0;

        uint64_t totalPages = bitmapSize * 8;
        uint64_t runStart = 0;
        uint64_t runLength = 0;

        // Scan for a contiguous memory block that's large enough to hold the requested pages
        for (uint64_t page = 0; page < totalPages; page++) {
            uint64_t byteIndex = page / 8;
            uint8_t  bitIndex  = page % 8;

            bool allocated = (allocatableBitmap + higherHalfOffset)[byteIndex] & (1 << bitIndex);
            bool reserved = (reservedBitmap + higherHalfOffset)[byteIndex] & (1 << bitIndex);

            if (!reserved && !allocated) {
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

    // Test the PMM by allocating single and multi page blocks, writing to and reading from, and finally freeing them
    STATUS PMM::TestPMM() {
        if (SETTING_TEST_MODE != 1) return STATUS::SUCCESS;

        // First, try allocate a single page
        uintptr_t singleAlloc = PMM::AllocatePages(1);
        if (!singleAlloc) return STATUS::FAILURE;
        LOG_DEBUG("PMM single-page test allocation succeeded with address %p.", singleAlloc);

        // Allocate another page and make sure it's not the same address as the previous one
        uintptr_t singleAlloc2 = PMM::AllocatePages(1);
        if (!singleAlloc2 || singleAlloc2 == singleAlloc) return STATUS::FAILURE;
        LOG_DEBUG("PMM second single-page test allocation succeeded with address %p.", singleAlloc2);

        // Now try to allocate a continuous chunk
        uintptr_t multiAlloc = PMM::AllocatePages(4);
        if (!multiAlloc) return STATUS::FAILURE;
        LOG_DEBUG("PMM multi-page test allocation succeeded with address %p.", multiAlloc);

        // Try to allocate a massive 10MiB continuous chunk
        uintptr_t massiveMultiAlloc = PMM::AllocatePages(2560);
        if (!massiveMultiAlloc) return STATUS::FAILURE;
        LOG_DEBUG("PMM 10 MiB multi-page test allocation succeeded with address %p.", massiveMultiAlloc);

        // All of the allocation tests passed, now we need to write to the allocated areas
        uint8_t* memory = (uint8_t*)(multiAlloc + higherHalfOffset);
        uint64_t writeCount = 2560 * Architecture::KernelPageSize;
        for (uint64_t i = 0; i < writeCount; i++) {
            *memory = 0xAE;
            memory++;
        }

        // All tests passed!
        return STATUS::SUCCESS;
    }
} // Memory
