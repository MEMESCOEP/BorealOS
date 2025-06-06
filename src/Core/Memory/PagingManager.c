#include <Core/Memory/PagingManager.h>

#include "Core/Graphics/Console.h"
#include "Core/Kernel/Panic.h"
#include "Drivers/IO/Serial.h"
#include "Utilities/MathUtils.h"

extern void *_KernelEndMarker;
extern void *_KernelStartMarker;

PageState_t page_state = {
    .page_directory = NULL,
    .page_table = NULL,
    .cr3_phys = 0
};

void enable_paging(void) {
    asm volatile (
        "mov %0, %%cr3\n\t"
        "mov %%cr0, %%eax\n\t"
        "or $0x80000000, %%eax\n\t"
        "mov %%eax, %%cr0\n\t"
        "jmp 1f\n\t"          // Far jump to flush pipeline
        "1:\n\t"
        :
        : "r"(page_state.cr3_phys)
        : "%eax"
    );
};

void PagingManagerInit(void* MB2InfoPtr) {
    SendStringSerial(SERIAL_COM1, "Initializing Paging Manager...\n\r");

    PageDirectoryEntry_t *page_directory = (PageDirectoryEntry_t *)PhysicalMemoryManagerAlloc(PAGE_SIZE, PAGE_SIZE);
    if (!page_directory) {
        LOG_KERNEL_MSG("Failed to allocate page directory.\n\r", ERROR);
        KernelPanic(PAGING_MANAGER_ERROR_FAILED_PMM_ALLOC_FAIL, "Failed to allocate page directory.");
    }

    // Map entire first 4MB for boot structures
    const size_t INITIAL_MAP_SIZE = 4 * 1024 * 1024;
    const size_t INITIAL_PAGE_COUNT = INITIAL_MAP_SIZE / PAGE_SIZE;

    PageTableEntry_t *initial_page_table = (PageTableEntry_t *)PhysicalMemoryManagerAlloc(PAGE_SIZE, PAGE_SIZE);
    if (!initial_page_table) {
        LOG_KERNEL_MSG("Failed to allocate initial page table.\n\r", ERROR);
        KernelPanic(PAGING_MANAGER_ERROR_FAILED_PMM_ALLOC_FAIL, "Failed to allocate initial page table.");
    }

    MemSet(page_directory, 0, PAGE_SIZE);
    MemSet(initial_page_table, 0, PAGE_SIZE);

    // Set up identity mapping for first 4MB
    for (size_t i = 0; i < INITIAL_PAGE_COUNT; i++) {
        uintptr_t phys_addr = i * PAGE_SIZE;
        initial_page_table[i].value = phys_addr | (PAGE_FLAGS_PRESENT | PAGE_FLAGS_RW);
    }

    // Map first page directory entry to our initial page table
    page_directory[0].value = ((uint32_t)initial_page_table >> 12) << 12;
    page_directory[0].value |= PAGE_FLAGS_PRESENT | PAGE_FLAGS_RW | PAGE_FLAGS_USER;

    page_state.page_directory = page_directory;
    page_state.cr3_phys = (uintptr_t)page_directory;

    // Identity map critical structures
    MapPage((uintptr_t)page_directory, (uintptr_t)page_directory, PAGE_FLAGS_PRESENT | PAGE_FLAGS_RW);
    MapPage((uintptr_t)initial_page_table, (uintptr_t)initial_page_table, PAGE_FLAGS_PRESENT | PAGE_FLAGS_RW);

    LOG_KERNEL_MSG("Enabling paging...\n\r", INFO);
    enable_paging();
    SendStringSerial(SERIAL_COM1, "Paging Manager initialized successfully.\n\r"); // we use Serial here because we enabled paging, which means the console's framebuffer is now protected physical memory.
}

void MapPage(uintptr_t virt, uintptr_t phys, uint64_t flags) {
    uint32_t pd_index = (virt >> 22) & 0x3FF;
    uint32_t pt_index = (virt >> 12) & 0x3FF;

    PageDirectoryEntry_t *pde_array = page_state.page_directory;

    if (!pde_array[pd_index].present) {
        PageTableEntry_t *new_pt = (PageTableEntry_t *)PhysicalMemoryManagerAlloc(PAGE_SIZE, PAGE_SIZE);
        if (!new_pt) {
            LOG_KERNEL_MSG("MapPage: failed to allocate a new page table.\n\r", ERROR);
            KernelPanic(PAGING_MANAGER_ERROR_FAILED_PMM_ALLOC_FAIL, "MapPage: PMM alloc fail");
        }

        MemSet(new_pt, 0, PAGE_SIZE);

        uint32_t new_pt_phys = (uint32_t)new_pt;
        pde_array[pd_index].value =
            ((new_pt_phys >> 12) << 12)
            | (uint32_t)( (flags & 0x7) | 1 );
    }

    uint32_t pt_phys = pde_array[pd_index].table_base << 12;
    PageTableEntry_t *pt_array = (PageTableEntry_t *)pt_phys;

    pt_array[pt_index].value = ( (phys & 0xFFFFF000) | (uint32_t)(flags & 0xFFF) );

    asm volatile ("invlpg (%0)" :: "r"(virt) : "memory");
}

uintptr_t GetPage(uintptr_t virt) {
    uint32_t pd_index = (virt >> 22) & 0x3FF;
    uint32_t pt_index = (virt >> 12) & 0x3FF;

    PageDirectoryEntry_t *pde_array = page_state.page_directory;

    if (!pde_array[pd_index].present) {
        return 0;
    }

    uint32_t pt_phys = pde_array[pd_index].table_base << 12;
    PageTableEntry_t *pt_array = (PageTableEntry_t *)pt_phys;

    if (!pt_array[pt_index].present) {
        return 0;
    }

    return ( (uintptr_t)(pt_array[pt_index].frame) << 12 );
}

void UnmapPage(uintptr_t virt) {
    uint32_t pd_index = (virt >> 22) & 0x3FF;
    uint32_t pt_index = (virt >> 12) & 0x3FF;

    PageDirectoryEntry_t *pde_array = page_state.page_directory;

    if (!pde_array[pd_index].present) {
        return;
    }

    uint32_t pt_phys = pde_array[pd_index].table_base << 12;
    PageTableEntry_t *pt_array = (PageTableEntry_t *)pt_phys;

    if (!pt_array[pt_index].present) {
        return;
    }

    pt_array[pt_index].value = 0;

    asm volatile ("invlpg (%0)" :: "r"(virt) : "memory");
}
