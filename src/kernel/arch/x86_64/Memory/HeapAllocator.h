#ifndef BOREALOS_HEAPALLOCATOR_H
#define BOREALOS_HEAPALLOCATOR_H

#include <Definitions.h>

#include "Kernel.h"
#include "Paging.h"
#include "PMM.h"

// TODO: Implement spin locks for the heap allocator when moving to multi core.

namespace Memory {
    class HeapAllocator {
    public:
        enum class AllocateMode {
            Normal, // Just allocate the memory, no special requirements
            Zeroed // Allocate the memory and zero it out
        };

        struct AllocateArgs {
            size_t bytes = 0;
            PageFlags flags = PageFlags::Present | PageFlags::ReadWrite;
            size_t alignment = 16;
            AllocateMode mode = AllocateMode::Normal;
        };

        explicit HeapAllocator(PMM* pmm, Paging* paging, Paging::PagingState* pagingState, size_t heapOffset = 2 * Constants::MiB);
        void Initialize();

        [[nodiscard]] uintptr_t Allocate(AllocateArgs args);
        void Free(uintptr_t address, size_t bytes);

    private:
        [[nodiscard]] static int32_t GetBinIndex(size_t size);

        PMM *physicalMemoryManager; // We need this to allocate and free physical pages when we need to grow or shrink the heap
        Paging *paging; // We need this to map and unmap pages
        static constexpr uintptr_t heapHigherHalfStart = 0xF8F8F000000;

        // This is our target paging state, this is which page tables we will be modifying when we map and unmap pages for the heap.
        // This will be the kernel Paging state when we're in kernel mode, or a process's if we're in user mode. This is set in KernelData and we can access it from there.
        Paging::PagingState *pagingState;

        // The offset of the start of the heap, this is the start of the virtual address range that the heap will use.
        // This just means applications will receive addresses starting from this offset.
        // It is 2 MiB by default because checking if an address is valid is usually done by checking if it's not null, so this ensures that all heap addresses are non-null.
        size_t heapOffset;

        struct Bin {
            uintptr_t startAddress; // The starting address of the memory region backing this bin, used for bounds checking when freeing.
            uintptr_t freeListHead; // Linked list of free blocks.
            size_t blockSize; // Size of blocks in this bin.
            size_t usedBlockCount; // Number of used blocks in this bin.
            size_t freeBlockCount; // Number of free blocks in this bin.
            PageFlags flags; // The flags that are used for the pages backing this bin.
            Bin* next; // Next bin in the list.
        };

        struct SizeClass {
            size_t classSize; // The size of blocks in this size class.
            Bin* bins;
        };

        static constexpr uint32_t sizeClassCount = 9;

        SizeClass sizeClasses[sizeClassCount] = {
            {16, nullptr},
            {32, nullptr},
            {64, nullptr},
            {128, nullptr},
            {256, nullptr},
            {512, nullptr},
            {1024, nullptr},
            {2048, nullptr},
            {Architecture::KernelPageSize, nullptr}
        };

        struct HugeAllocation {
            uintptr_t address; // The starting address of the allocation.
            size_t size; // The size of the allocation in bytes.
            size_t pageCount; // The number of pages backing this allocation.
            PageFlags flags; // The flags that are used for the pages backing this allocation.
            HugeAllocation* next; // Next huge allocation in the list.
        };

        HugeAllocation *hugeAllocationsHead;

        [[nodiscard]] Bin* GetBinForSize(size_t size, PageFlags flags) const;
        Bin* CreateBinForSize(size_t size, PageFlags flags);

        Bin* metadataBin = nullptr;

        uintptr_t AllocateFromBin(Bin* bin);
        uintptr_t AllocateHuge(size_t bytes, PageFlags flags);
        void FreeHuge(uintptr_t address, size_t bytes);

        uintptr_t AllocateMetadata();
        void FreeMetadata(uintptr_t address);

        void FreeBin(SizeClass &sizeClass, Bin *currentBin);

        [[nodiscard]] STATUS Test();
    };

    // This is a C like heap allocator.
} // Memory

#endif //BOREALOS_HEAPALLOCATOR_H