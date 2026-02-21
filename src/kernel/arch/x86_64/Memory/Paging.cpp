#include "Paging.h"
#include <Boot/LimineDefinitions.h>

#include "Kernel.h"
#include "../KernelData.h"
#include "../IO/FramebufferConsole.h"

namespace Memory {
    struct Paging::PagingState {
        PML4* pml4;
    };

    Paging::Paging(PMM *pmm) {
        physicalMemoryManager = pmm;
        kernelPagingState = nullptr;
        currentPagingState = nullptr;
        kernelHigherHalfOffset = hhdm_request.response->offset;
    }

    void Paging::Initialize() {
        size_t requiredPages = (sizeof(PagingState) + Architecture::KernelPageSize - 1) / Architecture::KernelPageSize;
        size_t requiredPagesForTables = 1; // We dynamically allocate other as necessary.
        size_t totalRequiredPages = requiredPages + requiredPagesForTables;
        uintptr_t stateMemory = physicalMemoryManager->AllocatePages(totalRequiredPages);
        if (!stateMemory) PANIC("Failed to allocate memory for Paging state!");

        // We have to make all allocated pages physical, since the CPU directly accesses memory using the MMU. The higher half offset will be added when we access the memory, but the addresses stored in the page tables need to be physical.
        kernelPagingState = reinterpret_cast<PagingState *>(stateMemory);
        auto* vmmState = reinterpret_cast<PagingState *>(stateMemory + kernelHigherHalfOffset); // make it accessible.
        vmmState->pml4 = reinterpret_cast<PML4 *>(stateMemory + Architecture::KernelPageSize); // The PML4 will be stored in the page immediately following the Paging state
        memset((reinterpret_cast<void *>(reinterpret_cast<uint64_t>(vmmState->pml4) + kernelHigherHalfOffset)), 0, sizeof(PML4)); // Clear the PML4

        uint64_t kernelElfOffset = 0XFFFFFFFF80000000;
        CopyExistingPageTableToNew(vmmState, kernelElfOffset, kernelHigherHalfOffset);

        LOG_DEBUG("Copied over kernel ELF mappings to the Paging's PML4. Now switching to the new page table...");
        LOG_DEBUG("We are now on our own page table! Paging at %p (virtual), with PML4 at %p (physical).", vmmState, vmmState->pml4);

        SwitchToKernelPageTable();
    }

    void Paging::MapPage(uint64_t virtualAddress, uint64_t physicalAddress, PageFlags flags) {
        MapPage(currentPagingState, physicalMemoryManager, virtualAddress, physicalAddress, flags, kernelHigherHalfOffset);
    }

    void Paging::UnmapPage(uint64_t virtualAddress) {
        UnmapPage(currentPagingState, physicalMemoryManager, virtualAddress, kernelHigherHalfOffset);
    }

    uint64_t Paging::GetPhysicalAddress(uint64_t virtualAddress) {
        uint32_t pml4Index, pdpIndex, pdIndex, ptIndex, pageOffset;
        ExtractPageTableIndices(virtualAddress, pml4Index, pdpIndex, pdIndex, ptIndex, pageOffset);

        // Apply the higher half offset to the Paging state so we can access it
        auto vmmState = reinterpret_cast<PagingState *>(reinterpret_cast<uint64_t>(currentPagingState) + kernelHigherHalfOffset);
        auto pml4 = reinterpret_cast<PML4 *>(reinterpret_cast<uint64_t>(vmmState->pml4) + kernelHigherHalfOffset);
        if (!(pml4->entries[pml4Index] & static_cast<uint64_t>(PageFlags::Present))) return 0;

        uintptr_t pdpPhysicalAddress = pml4->entries[pml4Index] & PointerMask;
        auto pdp = reinterpret_cast<PDP *>(reinterpret_cast<uint64_t>(pdpPhysicalAddress) + kernelHigherHalfOffset);
        if (!(pdp->entries[pdpIndex] & static_cast<uint64_t>(PageFlags::Present))) return 0;

        uintptr_t pdPhysicalAddress = pdp->entries[pdpIndex] & PointerMask;
        auto pd = reinterpret_cast<PD *>(reinterpret_cast<uint64_t>(pdPhysicalAddress) + kernelHigherHalfOffset);
        if (!(pd->entries[pdIndex] & static_cast<uint64_t>(PageFlags::Present))) return 0;

        uintptr_t ptPhysicalAddress = pd->entries[pdIndex] & PointerMask;
        auto pt = reinterpret_cast<PT *>(reinterpret_cast<uint64_t>(ptPhysicalAddress) + kernelHigherHalfOffset);
        if (!(pt->entries[ptIndex] & static_cast<uint64_t>(PageFlags::Present))) return 0;

        uintptr_t pagePhysicalAddress = pt->entries[ptIndex] & PointerMask;
        return pagePhysicalAddress | pageOffset;
    }

    void Paging::SwitchToKernelPageTable() {
        // We will get a page fault trying to access the pml4 field if we don't add the higher half offset, since the Paging state was allocated in physical memory.
        SwitchToPageTable(kernelPagingState);
    }

    Paging::PagingState * Paging::CreatePagingStateForProcess() const {
        // TODO: implement this, later when working on processes.
        PANIC("CreatePagingStateForProcess is not implemented yet!");
        return nullptr;
    }

    void Paging::SwitchToPageTable(PagingState *state) {
        auto newState = reinterpret_cast<PagingState *>(reinterpret_cast<uint64_t>(state) + kernelHigherHalfOffset); // Convert from phys to virt.
        if (!newState || !newState->pml4) PANIC("Invalid Paging state provided for page table switch!");
        if (newState == currentPagingState) return; // No need to switch if we're already on the desired page table

        auto newPml4PhysicalAddress = reinterpret_cast<uint64_t>(newState->pml4); // the pml4 is in physical memory, so this is already the physical address
        asm volatile("mov %0, %%cr3" : : "r"(newPml4PhysicalAddress) : "memory"); // Load the new page table into CR3
        asm volatile ("invlpg (%0)" : : "r"(0) : "memory"); // Invalidate the TLB to ensure the new page table is used immediately
        currentPagingState = state;
    }

    // This functions deeply copies the existing page table to a new one.

    void Paging::CopyExistingPageTableToNew(PagingState *vmmState, uint64_t offset, uint64_t higherHalfOffset) {
        uint64_t cr3;
        asm volatile("mov %%cr3, %0" : "=r"(cr3));

        // cr3 is limine's mapping.
        PML4 *liminePml4 = reinterpret_cast<PML4 *>(cr3 + higherHalfOffset);
        uintptr_t physPml4Addr = reinterpret_cast<uintptr_t>(vmmState->pml4);
        PML4 *newPml4 = reinterpret_cast<PML4 *>(physPml4Addr + higherHalfOffset);

        // Now we need to recursively copy over all the entries from limine.
        for (int i = 0; i < 512; i++) {
            if (liminePml4->entries[i] & static_cast<uint64_t>(PageFlags::Present)) {
                newPml4->entries[i] = liminePml4->entries[i];
                uint64_t nextLevelPhysicalAddress = liminePml4->entries[i] & PointerMask; // source
                uint64_t destPhysicalAddress = physicalMemoryManager->AllocatePages(1); // destination
                if (!destPhysicalAddress) PANIC("Failed to allocate memory for page table during deep copy!");
                DeepCopyPageTables(3, nextLevelPhysicalAddress, destPhysicalAddress, higherHalfOffset);
                newPml4->entries[i] = (newPml4->entries[i] & FlagsMask) | destPhysicalAddress; // Copy flags from limine's entry, but replace the address with the new one
            }
        }
    }

    void Paging::DeepCopyPageTables(uint32_t level, uintptr_t srcPhysical, uintptr_t dstPhysical,
                                    uint64_t higherHalfOffset) {
        auto srcTable = reinterpret_cast<uint64_t*>(srcPhysical + higherHalfOffset);
        auto dstTable = reinterpret_cast<uint64_t*>(dstPhysical + higherHalfOffset);

        for (int i = 0; i < 512; i++) {
            if (!(srcTable[i] & static_cast<uint64_t>(PageFlags::Present))) continue;

            // If the entry is a huge page, we can copy it directly, since huge pages are leaf entries.
            if ((level == 3 || level == 2) && (srcTable[i] & static_cast<uint64_t>(PageFlags::HugePage))) {
                dstTable[i] = srcTable[i];
                continue;
            }

            // If level is 1, we are at the PT level and can copy the entries directly
            if (level == 1) {
                dstTable[i] = srcTable[i];
                continue;
            }

            // Otherwise, allocate a new table for the next level
            uintptr_t newTablePhys = physicalMemoryManager->AllocatePages(1);
            memset(reinterpret_cast<void*>(newTablePhys + higherHalfOffset), 0, 4096);

            // Link the new table into the destination hierarchy
            uint64_t flags = srcTable[i] & FlagsMask;
            dstTable[i] = newTablePhys | flags;

            uintptr_t nextSrcPhys = srcTable[i] & PointerMask;
            uintptr_t nextDstPhys = newTablePhys;
            DeepCopyPageTables(level - 1, nextSrcPhys, nextDstPhys, higherHalfOffset);
        }
    }

    void Paging::MapPage(PagingState *vmmState, PMM *physicalMemoryManager, uint64_t virtualAddress, uint64_t physicalAddress, PageFlags flags, uint64_t
                         offset) {

        if (virtualAddress & (Architecture::KernelPageSize - 1)) PANIC("Virtual address is not page-aligned!");
        if (physicalAddress & (Architecture::KernelPageSize - 1)) PANIC("Physical address is not page-aligned!");

        uint64_t entryFlags = static_cast<uint64_t>(flags) | static_cast<uint64_t>(PageFlags::Present);
        auto directoryFlags = static_cast<uint64_t>(PageFlags::Present | PageFlags::ReadWrite | PageFlags::UserSupervisor);

        uint32_t pml4Index, pdpIndex, pdIndex, ptIndex, pageOffset;
        ExtractPageTableIndices(virtualAddress, pml4Index, pdpIndex, pdIndex, ptIndex, pageOffset);

        // Apply the higher half offset to the Paging state so we can access it
        vmmState = reinterpret_cast<PagingState *>(reinterpret_cast<uint64_t>(vmmState) + offset);
        PML4* pml4 = reinterpret_cast<PML4 *>(reinterpret_cast<uint64_t>(vmmState->pml4) + offset);
        if (!pml4->entries[pml4Index]) {
            // Allocate a new PDP table
            PDP* newPdp = reinterpret_cast<PDP *>(physicalMemoryManager->AllocatePages(1));
            if (!newPdp) PANIC("Failed to allocate memory for new PDP table!");
            memset(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(newPdp) + offset), 0, sizeof(PDP)); // Clear the new table
            pml4->entries[pml4Index] = reinterpret_cast<uint64_t>(newPdp) | directoryFlags;
        }

        uintptr_t pdpPhysicalAddress = pml4->entries[pml4Index] & PointerMask;
        PDP* pdp = reinterpret_cast<PDP *>(reinterpret_cast<uint64_t>(pdpPhysicalAddress) + offset);
        if (!pdp->entries[pdpIndex]) {
            // Allocate a new PD table
            PD* newPd = reinterpret_cast<PD *>(physicalMemoryManager->AllocatePages(1));
            if (!newPd) PANIC("Failed to allocate memory for new PD table!");
            memset(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(newPd) + offset), 0, sizeof(PD)); // Clear the new table
            pdp->entries[pdpIndex] = reinterpret_cast<uint64_t>(newPd) | directoryFlags;
        }

        uintptr_t pdPhysicalAddress = pdp->entries[pdpIndex] & PointerMask;
        PD* pd = reinterpret_cast<PD *>(reinterpret_cast<uint64_t>(pdPhysicalAddress) + offset);
        if (!pd->entries[pdIndex]) {
            // Allocate a new PT table
            PT* newPt = reinterpret_cast<PT *>(physicalMemoryManager->AllocatePages(1));
            if (!newPt) PANIC("Failed to allocate memory for new PT table!");
            memset(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(newPt) + offset), 0, sizeof(PT)); // Clear the new table
            pd->entries[pdIndex] = reinterpret_cast<uint64_t>(newPt) | directoryFlags;
        }

        uintptr_t ptPhysicalAddress = pd->entries[pdIndex] & PointerMask;
        PT* pt = reinterpret_cast<PT *>(reinterpret_cast<uint64_t>(ptPhysicalAddress) + offset);
        if (pt->entries[ptIndex] & static_cast<uint64_t>(PageFlags::Present)) {
            LOG_ERROR("Virtual address %p is already mapped to physical address %p with flags %p!", virtualAddress, pt->entries[ptIndex] & PointerMask, pt->entries[ptIndex] & FlagsMask);
            PANIC("Virtual address is already mapped!");
        }

        pt->entries[ptIndex] = (physicalAddress & PointerMask) | entryFlags;
        asm volatile("invlpg (%0)" ::"r"(virtualAddress) : "memory"); // Invalidate the TLB entry for this page to ensure the new mapping is used immediately
    }

    void Paging::UnmapPage(PagingState *vmmState, PMM *physicalMemoryManager, uint64_t virtualAddress, uint64_t offset) {
        // Reverse of the mapping process, but we also need to check if the page tables are present at each level and panic if we try to unmap an address that isn't mapped
        uint32_t pml4Index, pdpIndex, pdIndex, ptIndex, pageOffset;
        ExtractPageTableIndices(virtualAddress, pml4Index, pdpIndex, pdIndex, ptIndex, pageOffset);

        vmmState = reinterpret_cast<PagingState *>(reinterpret_cast<uint64_t>(vmmState) + offset);
        PML4* pml4 = reinterpret_cast<PML4 *>(reinterpret_cast<uint64_t>(vmmState->pml4) + offset);
        if (!(pml4->entries[pml4Index] & static_cast<uint64_t>(PageFlags::Present))) PANIC("Attempted to unmap a virtual address that is not mapped (PML4 entry not present)!");

        uintptr_t pdpPhysicalAddress = pml4->entries[pml4Index] & PointerMask;
        PDP* pdp = reinterpret_cast<PDP *>(reinterpret_cast<uint64_t>(pdpPhysicalAddress) + offset);
        if (!(pdp->entries[pdpIndex] & static_cast<uint64_t>(PageFlags::Present))) PANIC("Attempted to unmap a virtual address that is not mapped (PDP entry not present)!");

        uintptr_t pdPhysicalAddress = pdp->entries[pdpIndex] & PointerMask;
        PD* pd = reinterpret_cast<PD *>(reinterpret_cast<uint64_t>(pdPhysicalAddress) + offset);
        if (!(pd->entries[pdIndex] & static_cast<uint64_t>(PageFlags::Present))) PANIC("Attempted to unmap a virtual address that is not mapped (PD entry not present)!");

        uintptr_t ptPhysicalAddress = pd->entries[pdIndex] & PointerMask;
        PT* pt = reinterpret_cast<PT *>(reinterpret_cast<uint64_t>(ptPhysicalAddress) + offset);
        if (!(pt->entries[ptIndex] & static_cast<uint64_t>(PageFlags::Present))) PANIC("Attempted to unmap a virtual address that is not mapped (PT entry not present)!");

        pt->entries[ptIndex] = 0; // Clear the entry to unmap the page
        asm volatile("invlpg (%0)" ::"r"(virtualAddress) : "memory"); // Invalidate the TLB entry for this page to ensure the unmapping takes effect immediately

        // Check if PT is now empty, and if so, free it and clear the PD entry
        if (IsTableEmpty(reinterpret_cast<uint64_t*>(pt))) {
            physicalMemoryManager->FreePages(ptPhysicalAddress, 1);
            pd->entries[pdIndex] = 0;

            // Check if PD is now empty, and if so, free it and clear the PDP entry
            if (IsTableEmpty(reinterpret_cast<uint64_t*>(pd))) {
                physicalMemoryManager->FreePages(pdPhysicalAddress, 1);
                pdp->entries[pdpIndex] = 0;

                // Check if PDP is now empty, and if so, free it and clear the PML4 entry
                if (IsTableEmpty(reinterpret_cast<uint64_t*>(pdp))) {
                    physicalMemoryManager->FreePages(pdpPhysicalAddress, 1);
                    pml4->entries[pml4Index] = 0;
                }
            }
        }
    }

    bool Paging::IsTableEmpty(uint64_t *table) {
        for (int i = 0; i < 512; i++) {
            if (table[i] & static_cast<uint64_t>(PageFlags::Present)) return false;
        }
        return true;
    }

    // https://wiki.osdev.org/Page_Tables#Long_mode_(64-bit)_page_map
    // https://upload.wikimedia.org/wikipedia/commons/9/9b/X86_Paging_64bit.svg
    void Paging::ExtractPageTableIndices(uint64_t vaddr, uint32_t &pml4Index, uint32_t &pdpIndex, uint32_t &pdIndex,
        uint32_t &ptIndex, uint32_t &pageIndex) {
        pml4Index = (vaddr >> 39) & 0x1FF;
        pdpIndex = (vaddr >> 30) & 0x1FF;
        pdIndex = (vaddr >> 21) & 0x1FF;
        ptIndex = (vaddr >> 12) & 0x1FF;
        pageIndex = vaddr & 0xFFF; // lower 12 bits are the offset within the page
    }
} // Memory

