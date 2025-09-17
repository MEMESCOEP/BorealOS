#include "Paging.h"
#include "Core/Kernel.h"

Status PagingInit(PagingState *state, KernelState * kernel) {
    state->PageDirectory = (uint32_t *)PhysicalMemoryManagerAllocatePage(&kernel->PhysicalMemoryManager);
    if (!state->PageDirectory) {
        PANIC(kernel, "PagingInit: Failed to allocate page directory!\n");
    }
    for (size_t i = 0; i < 1024; i++) {
        state->PageDirectory[i] = 0;
    }

    state->PageTable = (uint32_t **)PhysicalMemoryManagerAllocatePage(&kernel->PhysicalMemoryManager);
    if (!state->PageTable) {
        PANIC(kernel, "PagingInit: Failed to allocate page table pointer array!\n");
    }

    for (size_t i = 0; i < 1024; i++) {
        state->PageTable[i] = nullptr; // Initialize pointers
    }

    state->PageTable[0] = (uint32_t *)PhysicalMemoryManagerAllocatePage(&kernel->PhysicalMemoryManager);
    if (!state->PageTable[0]) {
        PANIC(kernel, "PagingInit: Failed to allocate first page table!\n");
    }

    // Zero out the first page table
    for (size_t i = 0; i < 1024; i++) {
        state->PageTable[0][i] = 0;
    }

    // Identity map first 4 MB
    for (size_t i = 0; i < 1024; i++) {
        state->PageTable[0][i] = (i * 0x1000) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    state->PageDirectory[0] = ((uint32_t)state->PageTable[0]) | PAGE_PRESENT | PAGE_WRITABLE;

    return STATUS_SUCCESS;
}

Status PagingMapPage(PagingState *state, void *virtualAddr, void *physicalAddr, bool writable, bool user, KernelState *kernel) {
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
            state->PageTable[dirIndex] = (uint32_t *)PhysicalMemoryManagerAllocatePage(&kernel->PhysicalMemoryManager);
            if (!state->PageTable[dirIndex]) {
                PANIC(kernel, "PagingMapPage: Failed to allocate page table!\n");
            }

            // Zero out the new page table
            for (size_t i = 0; i < 1024; i++) state->PageTable[dirIndex][i] = 0;
        }

        // Update page directory to point to this page table
        state->PageDirectory[dirIndex] = ((uint32_t)state->PageTable[dirIndex]) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    // Map the page
    state->PageTable[dirIndex][tableIndex] = (pAddr & 0xFFFFF000) | flags;

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

void *PagingTranslate(PagingState *state, void *virtualAddr) {
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
    ASM("mov %0, %%cr3" : : "r"(state->PageDirectory));

    // Enable paging by setting the PG bit in CR0
    uint32_t cr0;
    ASM("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Set PG bit
    ASM("mov %0, %%cr0" : : "r"(cr0));
}

void PagingDisable() {
    ASM("cli");

    uint32_t cr0;
    ASM("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~0x80000000; // Clear PG bit
    ASM("mov %0, %%cr0" : : "r"(cr0));

    ASM("sti");
}

Status PagingTest(PagingState *state, KernelState *kernel) {
    // Allocate a physical page
    void *page = PhysicalMemoryManagerAllocatePage(&kernel->PhysicalMemoryManager);
    if (page == NULL) {
        PANIC(kernel, "PagingTest: Failed to allocate a physical page!\n");
    }

    void *address = (void *)(1024 * 8192); // 8MB, should be unmapped initially
    if (PagingTranslate(state, address) != NULL) {
        PANIC(kernel, "PagingTest: Address should not be mapped yet!\n");
    }

    PagingMapPage(state, address, page, true, false, kernel);

    // Write to the mapped page
    volatile uint32_t *ptr = (uint32_t *)address;
    *ptr = 0xDEADBEEF;
    if (*ptr != 0xDEADBEEF) {
        PANIC(kernel, "PagingTest: Failed to read/write to mapped page!\n");
    }

    kernel->Printf(kernel, "Successfully wrote %p to %p at real address %p!\n", *ptr, address, page);

    if (PagingTranslate(state, address) != page) {
        PANIC(kernel, "PagingTest: Translated address does not match physical page!\n");
    }

    if (PagingUnmapPage(state, address) != STATUS_SUCCESS) {
        PANIC(kernel, "PagingTest: Failed to unmap page!\n");
    }

    kernel->Printf(kernel, "Paging test completed successfully.\n");


    return STATUS_SUCCESS;
}
