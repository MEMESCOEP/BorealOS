#include "HeapAllocator.h"
#include <Settings.h>
#include "../KernelData.h"

struct AllocationHeader {
    size_t totalSize;
    uint32_t magic;
};

static constexpr uint32_t ALLOC_MAGIC = 0xDEADC0DE;
static constexpr size_t HEADER_SIZE = (sizeof(AllocationHeader) + 15) & ~15; // Aligned to 16 bytes.

uintptr_t SizedNewAllocate(size_t bytes) {
    size_t actualBytes = bytes + HEADER_SIZE;

    Memory::HeapAllocator* heap = &Kernel<KernelData>::GetInstance()->ArchitectureData->HeapAllocator;
    uintptr_t rawAddr = heap->Allocate({.bytes = actualBytes});

    auto* header = reinterpret_cast<AllocationHeader*>(rawAddr);
    header->totalSize = actualBytes;
    header->magic = ALLOC_MAGIC;

    return rawAddr + HEADER_SIZE;
}

void FreeWithHeader(void* ptr) {
    if (!ptr) return;

    uintptr_t rawAddr = reinterpret_cast<uintptr_t>(ptr) - HEADER_SIZE;
    auto* header = reinterpret_cast<AllocationHeader*>(rawAddr);

    if (header->magic != ALLOC_MAGIC) {
        PANIC("Kernel Heap Corruption: Invalid magic in allocation header!");
    }

    Memory::HeapAllocator* heap = &Kernel<KernelData>::GetInstance()->ArchitectureData->HeapAllocator;
    heap->Free(rawAddr, header->totalSize);
}

void* operator new(size_t size) {
    return reinterpret_cast<void*>(SizedNewAllocate(size));
}

void* operator new[](size_t size) {
    return reinterpret_cast<void*>(SizedNewAllocate(size));
}

void operator delete(void* ptr, size_t size) noexcept {
    FreeWithHeader(ptr);
}

void operator delete(void* ptr) noexcept {
    FreeWithHeader(ptr);
}

void operator delete[](void* ptr) noexcept {
    FreeWithHeader(ptr);
}

void operator delete[](void* ptr, size_t size) noexcept {
    FreeWithHeader(ptr);
}

// Placement new (required by the C++ standard library/headers)
inline void* operator new(size_t, void* p) noexcept {
    return p;
}

namespace Memory {
    HeapAllocator::HeapAllocator(PMM *pmm, Paging *paging, Paging::PagingState* pagingState, size_t heapOffset) : physicalMemoryManager(
            pmm), paging(paging), pagingState(pagingState), heapOffset(heapOffset),
        hugeAllocationsHead(nullptr) {

    }

    void HeapAllocator::Initialize() {
#if SETTING_TEST_MODE
        if (this->Test() != STATUS::SUCCESS) {
            PANIC("Heap allocator failed self test!");
        }
#endif
    }

    uintptr_t HeapAllocator::Allocate(AllocateArgs args) {
        size_t bytes = args.bytes;
        PageFlags flags = args.flags;
        size_t alignment = args.alignment;
        AllocateMode mode = args.mode;

        if (bytes == 0) return 0;

        if (bytes <= 16) bytes = 16;

        if (alignment == 0) alignment = 16;
        if (alignment & (alignment - 1)) PANIC("Alignment must be a power of 2!");

        if (bytes > Architecture::KernelPageSize) {
            // For large allocations, we will just allocate whole pages and map them directly, this is simpler and more efficient than trying to fit them into bins.
            return AllocateHuge(bytes, flags);
        }

        auto* bin = GetBinForSize(bytes, flags);
        if (!bin) {
            bin = CreateBinForSize(bytes, flags);
            if (!bin) PANIC("Failed to create a heap bin for allocation!");
        }

        uintptr_t allocationAddress = AllocateFromBin(bin);
        if (!allocationAddress) PANIC("Failed to allocate memory from heap bin!");

        if (mode == AllocateMode::Zeroed) {
            memset(reinterpret_cast<void*>(allocationAddress), 0, bytes);
        }

        return allocationAddress;
    }

    void HeapAllocator::Free(uintptr_t address, size_t bytes) {
        if (bytes == 0) return;

        if (bytes > Architecture::KernelPageSize) {
            FreeHuge(address, bytes);
            return;
        }

        if (bytes <= 16) bytes = 16;
        int32_t bin = GetBinIndex(bytes);
        if (bin < 0) PANIC("Invalid size for heap free!");

        // We need to find the bin that this address belongs to, we can do this by iterating through the bins for this size class and checking if the address is within the range of the bins.
        auto& sizeClass = sizeClasses[bin];
        auto* currentBin = sizeClass.bins;
        while (currentBin) {
            uintptr_t binStart = currentBin->startAddress; // The start of the bin is the address of the first block in the free list, since all blocks in the bin are contiguous
            uintptr_t binEnd = binStart + (currentBin->blockSize * (currentBin->usedBlockCount + currentBin->freeBlockCount)); // The end of the bin is the start plus the total size of all blocks in the bin
            if (address >= binStart && address < binEnd) {
                // This is the bin we need to free the block to, we can just add the block back to the free list
                *reinterpret_cast<uintptr_t *>(address) = currentBin->freeListHead; // Add the block back to the free list
                currentBin->freeListHead = address;
                currentBin->usedBlockCount--;
                currentBin->freeBlockCount++;

                if (currentBin->usedBlockCount == 0) {
                    FreeBin(sizeClass, currentBin);
                }
                return;
            }

            currentBin = currentBin->next;
        }

        LOG_ERROR("Attempted to free an address that was not allocated by the heap allocator: %p (size: %u64)", address, bytes);
        PANIC("Invalid free in heap allocator!");
    }

    // Remove the pedantic warning about range cases being a gnu extension.
    void HeapAllocator::FreeBin(HeapAllocator::SizeClass &sizeClass, HeapAllocator::Bin *currentBin) {
        // Free the page backing this bin, and the metadata for the bin.
        uintptr_t pageAddress = paging->GetPhysicalAddress(currentBin->startAddress);
        if (!pageAddress) PANIC("Failed to get physical address for heap bin page during free!");
        size_t pageCount = (currentBin->blockSize * (currentBin->usedBlockCount + currentBin->freeBlockCount) + Architecture::KernelPageSize - 1) / Architecture::KernelPageSize;
        for (size_t i = 0; i < pageCount; i++) {
            paging->UnmapPage(currentBin->startAddress + i * Architecture::KernelPageSize);
        }
        physicalMemoryManager->FreePages(pageAddress, pageCount);

        // Remove the bin from the size class's bin list and free the metadata for the bin.
        if (sizeClass.bins == currentBin) {
            sizeClass.bins = currentBin->next;
        } else {
            auto* prevBin = sizeClass.bins;
            while (prevBin && prevBin->next != currentBin) {
                prevBin = prevBin->next;
            }
            if (prevBin) {
                prevBin->next = currentBin->next;
            } else {
                PANIC("Failed to find previous bin in bin list during free!");
            }
        }

        FreeMetadata(reinterpret_cast<uintptr_t>(currentBin));
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    int32_t HeapAllocator::GetBinIndex(size_t size) {
        switch (size) {
            case 1 ... 16:
                return 0;
            case 17 ... 32:
                return 1;
            case 33 ... 64:
                return 2;
            case 65 ... 128:
                return 3;
            case 129 ... 256:
                return 4;
            case 257 ... 512:
                return 5;
            case 513 ... 1024:
                return 6;
            case 1025 ... 2048:
                return 7;
        }

        if (size > Architecture::KernelPageSize)
            return -1;

        return sizeClassCount - 1; // Kernel page size is the highest size class.
    }
#pragma GCC diagnostic pop

    HeapAllocator::Bin *HeapAllocator::GetBinForSize(size_t size, PageFlags flags) const {
        int32_t binIndex = GetBinIndex(size);
        if (binIndex < 0) return nullptr;

        auto& sizeClass = sizeClasses[binIndex];
        auto* bin = sizeClass.bins;
        while (bin) {
            if (bin->flags == flags) return bin;
            bin = bin->next;
        }

        return nullptr;
    }

    HeapAllocator::Bin *HeapAllocator::CreateBinForSize(size_t size, PageFlags flags) {
        int32_t binIndex = GetBinIndex(size);
        if (binIndex < 0) return nullptr;

        auto& sizeClass = sizeClasses[binIndex];
        size = sizeClass.classSize;
        auto* lastBin = sizeClass.bins;
        while (lastBin && lastBin->next) {
            lastBin = lastBin->next;
        }

        auto newBin = reinterpret_cast<Bin *>(AllocateMetadata());
        newBin->freeListHead = 0;
        newBin->startAddress = 0;
        newBin->blockSize = sizeClass.classSize;
        newBin->usedBlockCount = 0;
        newBin->freeBlockCount = 0;
        newBin->flags = flags;
        newBin->next = nullptr;

        if (lastBin) {
            lastBin->next = newBin;
        } else {
            sizeClass.bins = newBin;
        }

        // Allocate a page for this bin to use.
        uintptr_t pageAddress = physicalMemoryManager->AllocatePages(1);
        if (!pageAddress) PANIC("Failed to allocate memory for new heap bin!");
        uintptr_t virtualAddress = heapOffset + pageAddress; // Map the page into the heap's virtual address space
        paging->MapPage(virtualAddress, pageAddress, flags | PageFlags::Present);
        newBin->startAddress = virtualAddress;

        // Add all blocks in this page to the bin's free list
        size_t blockCount = Architecture::KernelPageSize / sizeClass.classSize;
        for (size_t i = 0; i < blockCount; i++) {
            uintptr_t blockAddress = virtualAddress + i * sizeClass.classSize;
            *reinterpret_cast<uintptr_t *>(blockAddress) = newBin->freeListHead;
            newBin->freeListHead = blockAddress;
            newBin->freeBlockCount++;
        }

        return newBin;
    }

    uintptr_t HeapAllocator::AllocateFromBin(Bin *bin) {
        // If the bin has no free blocks, we need to create a new bin for this size class and flags
        if (bin->freeBlockCount == 0) {
            auto newBin = CreateBinForSize(bin->blockSize, bin->flags);
            if (!newBin) PANIC("Failed to create a new heap bin for allocation!");
            newBin->next = bin->next;
            bin->next = newBin;
            bin = newBin;
        }

        // Pop a block from the free list
        uintptr_t blockAddress = bin->freeListHead;
        bin->freeListHead = *reinterpret_cast<uintptr_t *>(blockAddress); // Update the head of the free list to the next free block
        bin->usedBlockCount++;
        bin->freeBlockCount--;

        return blockAddress;
    }

    uintptr_t HeapAllocator::AllocateHuge(size_t bytes, PageFlags flags) {
        size_t pageCount = (bytes + Architecture::KernelPageSize - 1) / Architecture::KernelPageSize;
        uintptr_t allocationAddress = physicalMemoryManager->AllocatePages(pageCount);
        if (!allocationAddress) PANIC("Failed to allocate memory for huge heap allocation!");
        uintptr_t virtualAddress = heapOffset + allocationAddress; // Map the pages into the heap's virtual address space
        for (size_t i = 0; i < pageCount; i++) {
            paging->MapPage(virtualAddress + i * Architecture::KernelPageSize, allocationAddress + i * Architecture::KernelPageSize, flags | PageFlags::Present);
        }

        auto* newAllocation = reinterpret_cast<HugeAllocation *>(AllocateMetadata());
        newAllocation->address = virtualAddress;
        newAllocation->size = bytes;
        newAllocation->pageCount = pageCount;
        newAllocation->flags = flags | PageFlags::Present;
        newAllocation->next = hugeAllocationsHead;
        hugeAllocationsHead = newAllocation;

        return virtualAddress;
    }

    void HeapAllocator::FreeHuge(uintptr_t address, size_t bytes) {
        HugeAllocation *current = hugeAllocationsHead;
        HugeAllocation *prev = nullptr;
        while (current) {
            if (current->address == address) {
                // Found the allocation to free
                if (prev) {
                    prev->next = current->next;
                } else {
                    hugeAllocationsHead = current->next;
                }

                // Unmap the pages and free the physical memory
                physicalMemoryManager->FreePages(paging->GetPhysicalAddress(current->address), current->pageCount);

                for (size_t i = 0; i < current->pageCount; i++) {
                    paging->UnmapPage(current->address + i * Architecture::KernelPageSize);
                }

                FreeMetadata(reinterpret_cast<uintptr_t>(current));
                return;
            }
            prev = current;
            current = current->next;
        }

        PANIC("Attempted to free a huge heap allocation that was not allocated!");
    }

    uintptr_t HeapAllocator::AllocateMetadata() {
        Bin* current = metadataBin;
        while (current) {
            if (current->freeBlockCount > 0) {
                return AllocateFromBin(current);
            }
            current = current->next;
        }

        // If we couldn't find a bin with a large enough block, we need to create a new bin for this size.
        size_t newBlockSize = 64;
        uintptr_t pageAddress = physicalMemoryManager->AllocatePages(1);
        if (!pageAddress) PANIC("Failed to allocate memory for heap metadata!");
        uintptr_t virtualAddress = heapHigherHalfStart + pageAddress; // Map the page into the heap's virtual address space
        paging->MapPage(virtualAddress, pageAddress, PageFlags::Present | PageFlags::ReadWrite);

        auto newBin = reinterpret_cast<Bin *>(virtualAddress);
        newBin->freeListHead = 0;
        newBin->startAddress = 0;
        newBin->blockSize = newBlockSize;
        newBin->usedBlockCount = 0;
        newBin->freeBlockCount = (Architecture::KernelPageSize / newBlockSize) - 1; // We will use one block for the Bin metadata itself, so we subtract one from the free block count.
        newBin->flags = PageFlags::Present | PageFlags::ReadWrite;
        newBin->next = metadataBin;

        uintptr_t blockAddress = virtualAddress + sizeof(Bin); // The blocks start immediately after the Bin metadata
        newBin->startAddress = blockAddress;
        for (size_t i = 0; i < newBin->freeBlockCount; i++) {
            uintptr_t currentBlockAddress = blockAddress + i * newBlockSize;
            *reinterpret_cast<uintptr_t *>(currentBlockAddress) = newBin->freeListHead;
            newBin->freeListHead = currentBlockAddress;
        }

        metadataBin = newBin;
        return AllocateFromBin(newBin);
    }

    void HeapAllocator::FreeMetadata(uintptr_t address) {
        if (!metadataBin) PANIC("No metadata bin available to free metadata!");
        *reinterpret_cast<uintptr_t *>(address) = metadataBin->freeListHead;
        metadataBin->freeListHead = address;
        metadataBin->usedBlockCount--;
        metadataBin->freeBlockCount++;

        if (metadataBin->usedBlockCount == 0) {
            Bin* next = metadataBin->next;
            // If the metadata bin is now empty, we can free the page backing it and remove it from the metadata bin list.
            uintptr_t pageAddress = paging->GetPhysicalAddress(metadataBin->startAddress - sizeof(Bin));
            if (!pageAddress) PANIC("Failed to get physical address for metadata bin page during free!");
            paging->UnmapPage(metadataBin->startAddress);
            physicalMemoryManager->FreePages(pageAddress, 1);

            metadataBin = next;
        }
    }

    STATUS HeapAllocator::Test() {
        // We can do some basic tests to ensure the heap allocator is working correctly, but we can't do exhaustive tests since we don't have a way to check for memory corruption or leaks.
        uintptr_t addr1 = Allocate({.bytes = 64});
        uintptr_t addr2 = Allocate({.bytes = 128});
        uintptr_t addr3 = Allocate({.bytes = 4096});
        uintptr_t addr4 = Allocate({.bytes = 8192, .mode = AllocateMode::Zeroed});

        LOG_DEBUG("addr1: %p, addr2: %p, addr3: %p, addr4: %p", addr1, addr2, addr3, addr4);
        if (!addr1 || !addr2 || !addr3 || !addr4) {
            LOG_ERROR("Heap allocator test failed: failed to allocate memory!");
            return STATUS::FAILURE;
        }

        if (*reinterpret_cast<uint8_t *>(addr4) != 0) {
            LOG_ERROR("Heap allocator test failed: zeroed allocation is not zeroed!");
            return STATUS::FAILURE;
        }

        // Read the memory at the allocated addresses to ensure they are valid and writable.

        Free(addr1, 64);
        Free(addr2, 128);
        Free(addr3, 4096);
        Free(addr4, 8192);

        if (metadataBin) {
            LOG_ERROR("Heap allocator test failed: metadata bin is not empty after freeing all allocations!");
            return STATUS::FAILURE;
        }

        // Reallocate them as they *should* share the same pointers now.
        uintptr_t addr5 = Allocate({.bytes = 64});
        uintptr_t addr6 = Allocate({.bytes = 128});
        uintptr_t addr7 = Allocate({.bytes = 4096});
        uintptr_t addr8 = Allocate({.bytes = 8192});

        LOG_DEBUG("addr5: %p, addr6: %p, addr7: %p, addr8: %p", addr5, addr6, addr7, addr8);
        if (addr5 != addr1 || addr6 != addr2 || addr7 != addr3 || addr8 != addr4) {
            LOG_ERROR("Heap allocator test failed: did not reuse freed memory!");
            return STATUS::FAILURE;
        }

        Free(addr5, 64);
        Free(addr6, 128);
        Free(addr7, 4096);
        Free(addr8, 8192);

        uintptr_t testUnaligned = Allocate({.bytes = 48});
        if (!testUnaligned) {
            LOG_ERROR("Heap allocator test failed: failed to allocate unaligned size!");
            return STATUS::FAILURE;
        }
        Free(testUnaligned, 48);
        uintptr_t testUnaligned2 = Allocate({.bytes = 48});
        if (testUnaligned2 != testUnaligned) {
            LOG_ERROR("Heap allocator test failed: did not reuse freed unaligned size!");
            return STATUS::FAILURE;
        }
        Free(testUnaligned2, 48);

        uintptr_t testUnaligned3 = Allocate({.bytes = 836});
        if (!testUnaligned3) {
            LOG_ERROR("Heap allocator test failed: failed to allocate unaligned size!");
            return STATUS::FAILURE;
        }
        Free(testUnaligned3, 836);

        struct ComplexObject {
            uint64_t data;
            ComplexObject() : data(0xACE) {}
            ~ComplexObject() { data = 0; }
        };

        size_t count = 4;
        auto* objs = new ComplexObject[count];

        for (size_t i = 0; i < count; i++) {
            if (objs[i].data != 0xACE) PANIC("ComplexObject corrupted");
        }

        delete[] objs;

        return STATUS::SUCCESS;
    }
}
