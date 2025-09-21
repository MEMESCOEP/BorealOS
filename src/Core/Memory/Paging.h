#ifndef BOREALOS_PAGING_H
#define BOREALOS_PAGING_H

#include <Definitions.h>

#define PAGE_PRESENT        0x001   // Entry is present
#define PAGE_WRITABLE       0x002   // 1 = writable, 0 = read-only
#define PAGE_USER           0x004   // 1 = user-mode accessible, 0 = supervisor only
#define PAGE_WRITE_THROUGH  0x008   // Write-through caching
#define PAGE_CACHE_DISABLE  0x010   // Disable caching
#define PAGE_ACCESSED       0x020   // Set by CPU on access
#define PAGE_DIRTY          0x040   // Only for PTE: set on write
#define PAGE_PAGE_SIZE      0x080   // Only for PDE: 1 = 4 MiB page
#define PAGE_GLOBAL         0x100   // Only if PGE enabled in CR4
#define PAGE_CUSTOM0        0x200   // Free for OS use
#define PAGE_CUSTOM1        0x400
#define PAGE_CUSTOM2        0x800

// 32 bit
typedef struct PagingState {
    uint32_t *PageDirectory;
    uint32_t **PageTable;
} PagingState;

/// Initialize paging structure.
/// Usage:
/// - Allocate a PagingState structure (can be on stack or heap, just make sure it does not go out of scope)
/// - Call PagingInit to initialize it
/// - Call PagingEnable to enable paging with this structure
/// - To disable paging, call PagingDisable, though this disables any paging, not just this structure
Status PagingInit(PagingState* state);

/// Map a single page (4 KiB) from virtual address to physical address with write, and or user permissions.
Status PagingMapPage(PagingState *state, void *virtualAddr, void *physicalAddr, bool writable, bool user);

/// Unmap a single page (4 KiB) at the given virtual address.
Status PagingUnmapPage(PagingState *state, void *virtualAddr);

/// Translate a virtual address to a physical address. Returns NULL if not mapped.
void *PagingTranslate(PagingState *state, void *virtualAddr);

/// Enable paging with the given paging structure.
/// This loads the page directory into CR3 and sets the PG bit in CR0.
/// Paging must be initialized with PagingInit before calling this.
void PagingEnable(PagingState* state);

/// Disable paging by clearing the PG bit in CR0.
void PagingDisable();

/// Test the paging implementation by mapping a page, writing to it, and verifying the contents.
Status PagingTest(PagingState* state);

#endif //BOREALOS_PAGING_H