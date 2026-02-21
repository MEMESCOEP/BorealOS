#ifndef BOREALOS_PAGING_H
#define BOREALOS_PAGING_H

#include <Definitions.h>
#include "PMM.h"

namespace Memory {
    enum class PageFlags : uint64_t {
        Present = 1 << 0,
        ReadWrite = 1 << 1,
        UserSupervisor = 1 << 2,
        WriteThrough = 1 << 3,
        CacheDisable = 1 << 4,
        Accessed = 1 << 5,
        Dirty = 1 << 6,
        HugePage = 1 << 7,
        Global = 1 << 8,
        NoExecute = 1ULL << 63
    };

    constexpr enum PageFlags operator|(PageFlags a, PageFlags b) {
        return static_cast<PageFlags>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
    }

    constexpr enum PageFlags operator|=(PageFlags& a, PageFlags b) {
        a = a | b;
        return a;
    }

    class Paging {
    public:
        struct PagingState;

        explicit Paging(PMM* pmm);

        /// Kernel virtual memory management initialization. To use the Paging for a process we must use a different function
        void Initialize();

        void MapPage(uint64_t virtualAddress, uint64_t physicalAddress, PageFlags flags);
        void UnmapPage(uint64_t virtualAddress);

        uint64_t GetPhysicalAddress(uint64_t virtualAddress);
        [[nodiscard]] bool IsMapped(uint64_t virtualAddress) { return GetPhysicalAddress(virtualAddress) != 0; }

        void SwitchToKernelPageTable();
        [[nodiscard]] PagingState* GetKernelPagingState() const { return kernelPagingState; }
        [[nodiscard]] PagingState* GetCurrentPagingState() const { return currentPagingState; }
        [[nodiscard]] PagingState* CreatePagingStateForProcess() const;
        void SwitchToPageTable(PagingState* newState);
    private:
        PMM* physicalMemoryManager;
        PagingState* kernelPagingState; // The Paging state for the kernel! When we are in kernel mode this will be used.
        uint64_t kernelHigherHalfOffset;
        PagingState* currentPagingState; // The currently active Paging state, this will be the same as kernelVmmState when we're in kernel mode

        void CopyExistingPageTableToNew(PagingState *vmmState, uint64_t offset, uint64_t higherHalfOffset);
        void DeepCopyPageTables(uint32_t level, uintptr_t srcPhysical, uintptr_t dstPhysical, uint64_t higherHalfOffset);

        static void MapPage(PagingState* vmmState, PMM *physicalMemoryManager, uint64_t virtualAddress, uint64_t physicalAddress, PageFlags flags, uint64_t
                            offset);
        static void UnmapPage(PagingState* vmmState, PMM *physicalMemoryManager, uint64_t virtualAddress, uint64_t offset);
        static bool IsTableEmpty(uint64_t* table);

        // Some helper functions to extract the indices from a virtual address
        static inline void ExtractPageTableIndices(uint64_t vaddr, uint32_t& pml4Index, uint32_t& pdpIndex, uint32_t& pdIndex, uint32_t& ptIndex, uint32_t& pageIndex);

        static constexpr uint64_t PointerMask = 0x000FFFFFFFFFF000; // Mask to get the address portion of a page table entry
        static constexpr uint64_t FlagsMask = 0xFFF0000000000FFF; // Mask to get the flags portion of a page table entry

        struct PT  { uint64_t entries[512]; } ALIGNED(0x1000);

        struct PD  { uint64_t entries[512]; } ALIGNED(0x1000);

        struct PDP { uint64_t entries[512]; } ALIGNED(0x1000);

        struct PML4{ uint64_t entries[512]; } ALIGNED(0x1000);
    };
} // Memory

#endif //BOREALOS_PAGING_H