#ifndef PAGINGMANAGER_H
#define PAGINGMANAGER_H

#include "PhysicalMemoryManager.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGE_SIZE 0x1000 // Same as minimum memory allocation size in PhysicalMemoryManager.h
#define PAGE_ENTRIES 1024

#define PAGING_MANAGER_ERROR_FAILED_PMM_ALLOC_FAIL -10

#define PAGE_FLAGS_PRESENT        0x1  // Page is present
#define PAGE_FLAGS_RW             0x2  // Read/Write
#define PAGE_FLAGS_USER           0x4  // User-mode access
#define PAGE_FLAGS_WRITE_THROUGH  0x8  // Write-through caching

typedef union {
    uint32_t value;
    struct {
        uint32_t present        : 1;   // 1 = this PDE is valid
        uint32_t rw             : 1;   // 1 = writable, 0 = read-only
        uint32_t user           : 1;   // 1 = user-mode ok, 0 = supervisor only
        uint32_t write_through  : 1;   // write-through caching
        uint32_t cache_disable  : 1;   // disable caching on this page
        uint32_t accessed       : 1;   // set by CPU when PTE/PDE is used
        uint32_t reserved       : 1;   // for 4 KB pages, must be 0
        uint32_t page_size      : 1;   // 0 = references a page table; 1 = 4 MB page
        uint32_t global         : 1;   // ignored for PDE pointing to a page table
        uint32_t avail          : 3;   // available for kernel use
        uint32_t table_base     : 20;  // physical address of page-table (>>12)
    } __attribute__((packed));
} PageDirectoryEntry_t;

typedef union {
    uint32_t value;
    struct {
        uint32_t present        : 1;   // 1 = this PTE maps a physical page
        uint32_t rw             : 1;   // 1 = writable, 0 = read-only
        uint32_t user           : 1;   // 1 = user-mode ok, 0 = supervisor only
        uint32_t write_through  : 1;   // write-through caching
        uint32_t cache_disable  : 1;   // disable caching on this page
        uint32_t accessed       : 1;   // set by CPU when this PTE is used
        uint32_t dirty          : 1;   // set by CPU when the page is written to
        uint32_t pat            : 1;   // Page Attribute Table index (unused without PAT)
        uint32_t global         : 1;   // 1 = global across contexts (ignored if PGE = 0 in CR4)
        uint32_t avail          : 3;   // available for OS
        uint32_t frame          : 20;  // physical address of 4 KB page frame (>>12)
    } __attribute__((packed));
} PageTableEntry_t;

// 32 bit no PAE paging state so no PML4
typedef struct {
    PageDirectoryEntry_t *page_directory; // Pointer to the page directory
    PageTableEntry_t *page_table;
    uint32_t cr3_phys;
} PageState_t;

extern PageState_t page_state;

void PagingManagerInit(void* MB2InfoPtr);
void MapPage(uintptr_t virt, uintptr_t phys, uint64_t flags);
uintptr_t GetPage(uintptr_t virt);
void UnmapPage(uintptr_t virt);

#endif //PAGINGMANAGER_H
