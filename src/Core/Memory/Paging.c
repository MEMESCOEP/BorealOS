#include <Definitions.h>
#include "Paging.h"
#include <Drivers/CPU.h>
#include "Core/Kernel.h"
#include "PhysicalMemoryManager.h"

Status PagingInit(PagingState *state) {
    state->PageDirectory = (uint32_t *)PhysicalMemoryManagerAllocatePage();
    if (!state->PageDirectory) {
        PANIC("PagingInit: Failed to allocate page directory!\n");
    }
    for (size_t i = 0; i < 1024; i++) {
        state->PageDirectory[i] = 0;
    }

    state->PageTable = (uint32_t **)PhysicalMemoryManagerAllocatePage();
    if (!state->PageTable) {
        PANIC("PagingInit: Failed to allocate page table pointer array!\n");
    }

    for (size_t i = 0; i < 1024; i++) {
        state->PageTable[i] = nullptr; // Initialize pointers
    }

    state->PageTable[0] = (uint32_t *)PhysicalMemoryManagerAllocatePage();
    if (!state->PageTable[0]) {
        PANIC("PagingInit: Failed to allocate first page table!\n");
    }

    // Zero out the first page table
    for (size_t i = 0; i < 1024; i++) {
        state->PageTable[0][i] = 0;
    }

    // Identity map first 8 MB
    for (size_t i = 0; i < 1024; i++) {
        state->PageTable[0][i] = (i * PMM_PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    state->PageTable[1] = (uint32_t *)PhysicalMemoryManagerAllocatePage();
    if (!state->PageTable[1]) {
        PANIC("PagingInit: Failed to allocate second page table!\n");
    }
    for (size_t i = 0; i < 1024; i++) {
        state->PageTable[1][i] = ((i + 1024) * PMM_PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    state->PageDirectory[0] = ((uint32_t)state->PageTable[0]) | PAGE_PRESENT | PAGE_WRITABLE;
    state->PageDirectory[1] = ((uint32_t)state->PageTable[1]) | PAGE_PRESENT | PAGE_WRITABLE;

    return STATUS_SUCCESS;
}

Status PagingMapPage(PagingState *state, void *virtualAddr, void *physicalAddr, bool writable, bool user, uint32_t extraFlags) {
    uint32_t vAddr = (uint32_t)virtualAddr;
    uint32_t pAddr = (uint32_t)physicalAddr;

    size_t dirIndex = (vAddr >> 22) & 0x3FF;
    size_t tableIndex = (vAddr >> 12) & 0x3FF;

    uint32_t flags = PAGE_PRESENT;
    if (writable) flags |= PAGE_WRITABLE;
    if (user) flags |= PAGE_USER;

    // Allocate page table if it does not exist
    if (!(state->PageDirectory[dirIndex] & PAGE_PRESENT)) {
        if (!state->PageTable[dirIndex]) {
            state->PageTable[dirIndex] = (uint32_t *)PhysicalMemoryManagerAllocatePage();
            if (!state->PageTable[dirIndex]) {
                PANIC("PagingMapPage: Failed to allocate page table!\n");
            }

            // Zero out the new page table
            for (size_t i = 0; i < 1024; i++) state->PageTable[dirIndex][i] = 0;
        }

        // Update page directory to point to this page table
        state->PageDirectory[dirIndex] = ((uint32_t)state->PageTable[dirIndex]) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    // Map the page
    state->PageTable[dirIndex][tableIndex] = (pAddr & 0xFFFFF000) | flags | (extraFlags & 0xFFF);

    // Flush TLB entry
    ASM("invlpg (%0)" ::"r"(vAddr) : "memory");

    return STATUS_SUCCESS;
}

Status PagingUnmapPage(PagingState *state, void *virtualAddr) {
    uint32_t vAddr = (uint32_t)virtualAddr;
    size_t dirIndex = (vAddr >> 22) & 0x3FF;
    size_t tableIndex = (vAddr >> 12) & 0x3FF;

    if (!(state->PageDirectory[dirIndex] & PAGE_PRESENT)) {
        return STATUS_FAILURE;
    }

    if (!(state->PageTable[dirIndex][tableIndex] & PAGE_PRESENT)) {
        return STATUS_FAILURE;
    }

    // Clear entry
    state->PageTable[dirIndex][tableIndex] = 0;

    // Flush TLB entry
    ASM("invlpg (%0)" ::"r"(vAddr) : "memory");

    return STATUS_SUCCESS;
}

void* PagingTranslate(PagingState *state, void *virtualAddr) {
    uint32_t vAddr = (uint32_t)virtualAddr;
    size_t dirIndex = (vAddr >> 22) & 0x3FF;
    size_t tableIndex = (vAddr >> 12) & 0x3FF;
    uint32_t offset = vAddr & 0xFFF;

    if (!(state->PageDirectory[dirIndex] & PAGE_PRESENT)) {
        return NULL;
    }

    uint32_t entry = state->PageTable[dirIndex][tableIndex];
    if (!(entry & PAGE_PRESENT)) {
        return NULL;
    }

    return (void *)((entry & 0xFFFFF000) | offset);
}

void PagingEnable(PagingState *state) {
    // Load the page directory into CR3
    CPUWriteCR3((uint32_t)(uintptr_t)state->PageDirectory);
    CPUEnablePaging();
}

void PagingDisable() {
    ASM("cli");

    uint32_t cr0;
    ASM("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~0x80000000; // Clear PG bit
    ASM("mov %0, %%cr0" : : "r"(cr0));

    ASM("sti");
}

Status PagingTest(PagingState *state) {
    // Allocate a physical page
    void *page = PhysicalMemoryManagerAllocatePage();
    if (!page) {
        PANIC("PagingTest: Failed to allocate a physical page!\n");
    }

    void *address = (void *)(GiB * 3);
    if (PagingTranslate(state, address) != NULL) {
        PANIC("PagingTest: Address should not be mapped yet!\n");
    }

    PagingMapPage(state, address, page, true, false, 0);

    // Write to the mapped page
    volatile uint32_t *ptr = (uint32_t *)address;
    *ptr = 0xDEADBEEF;
    if (*ptr != 0xDEADBEEF) {
        PANIC("PagingTest: Failed to read/write to mapped page!\n");
    }

    LOGF(LOG_INFO, "Successfully wrote %p to %p at real address %p!\n", *ptr, address, page);

    if (PagingTranslate(state, address) != page) {
        PANIC("PagingTest: Translated address does not match physical page!\n");
    }

    if (PagingUnmapPage(state, address) != STATUS_SUCCESS) {
        PANIC("PagingTest: Failed to unmap page!\n");
    }

    LOG(LOG_INFO, "Paging test completed successfully.\n");


    return STATUS_SUCCESS;
}

void PagingLogPageFaultInfo(PagingState *state, void* faultingAddress) { // faulting address is the CR2 here
    uint32_t addr = (uint32_t)faultingAddress;
    size_t dirIndex = (addr >> 22) & 0x3FF;
    size_t tableIndex = (addr >> 12) & 0x3FF;

    PRINT("Page Fault Info:\n");
    PRINTF("\t* Faulting address: %p\n", faultingAddress);
    PRINTF("\t* Page Directory Index: %u\n", (uint32_t)dirIndex);
    PRINTF("\t* Page Table Index: %u\n", (uint32_t)tableIndex);

    if (!(state->PageDirectory[dirIndex] & PAGE_PRESENT)) {
        PRINT("\t* Page Directory Entry not present.\n");

        // Find the nearest present entry before this one
        for (int32_t i = dirIndex - 1; i >= 0; i--) {
            if (state->PageDirectory[i] & PAGE_PRESENT) {
                PRINTF("\t* Nearest present Page Directory Entry before faulting address is at index %u\n", (uint32_t)i);
                PRINTF("\t* Entry value: %p\n", (void*)state->PageDirectory[i]);
                break;
            }
        }
        return;
    }

    uint32_t entry = state->PageTable[dirIndex][tableIndex];
    if (!(entry & PAGE_PRESENT)) {
        PRINT("\t* Page Table Entry not present.\n");

        // Find the nearest present entry before this one
        for (int32_t i = tableIndex - 1; i >= 0; i--) {
            if (state->PageTable[dirIndex][i] & PAGE_PRESENT) {
                PRINTF("\t* Nearest present Page Table Entry before faulting address is at index %u\n", (uint32_t)i);
                PRINTF("\t* Entry value: %p\n", (void*)state->PageTable[dirIndex][i]);
                break;
            }
        }
        return;
    }

    PRINTF("\t* Page Table Entry: %p\n", entry);
    PRINTF("\t* Mapped to physical address: %p\n", (void *)(entry & 0xFFFFF000));
    PRINTF("\t* Entry flags: 0x%x\n", entry & 0xFFF);
}