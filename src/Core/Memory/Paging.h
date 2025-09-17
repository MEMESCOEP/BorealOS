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

typedef struct KernelState KernelState; // Forward declaration to avoid circular dependency
Status PagingInit(PagingState* state, KernelState * kernel);

Status PagingMapPage(PagingState *state, void *virtualAddr, void *physicalAddr, bool writable, bool user, KernelState *kernel);
Status PagingUnmapPage(PagingState *state, void *virtualAddr);
void *PagingTranslate(PagingState *state, void *virtualAddr);
void PagingEnable(PagingState* state);
void PagingDisable();
Status PagingTest(PagingState* state, KernelState* kernel);

#endif //BOREALOS_PAGING_H