#ifndef BOREALOS_VIRTUALMEMORYMANAGER_H
#define BOREALOS_VIRTUALMEMORYMANAGER_H

#include <Definitions.h>
#include "Paging.h"

typedef struct VirtualMemoryManagerMetadata VirtualMemoryManagerMetadata;

typedef struct VirtualMemoryManagerState {
    PagingState *Paging; // The paging structure this VMM manages
    VirtualMemoryManagerMetadata *Metadata; // Metadata for tracking allocations
} VirtualMemoryManagerState;

/// Initialize the virtual memory manager with the given paging structure.
/// Virtual memory is the abstraction of physical memory, allowing for features like
/// - Isolation between processes
/// - Memory protection (read/write/execute permissions)
/// - Safer memory allocation (if a process closes unexpectedly, its memory can be reclaimed easily)
/// - Easier memory management (contiguous virtual memory can map to non-contiguous physical memory)
/// - Memory-mapped files and devices
/// - etc.
/// Usage:
/// - Allocate a VirtualMemoryManagerState structure (can be on stack or heap, just make sure it does not go out of scope)
/// - Call VirtualMemoryManagerInit to initialize it with a PagingState (this will bind the VMM to that paging structure)
/// - After this, the VMM can manage virtual memory allocations within that paging structure
Status VirtualMemoryManagerInit(VirtualMemoryManagerState *vmm, PagingState *paging);

/// Allocate a block of virtual memory of the given size (in bytes).
/// The memory will be mapped to physical pages as needed.
/// Returns a pointer to the allocated virtual memory, or NULL on failure.
void* VirtualMemoryManagerAllocate(VirtualMemoryManagerState *vmm, size_t size, bool writable, bool user);

/// Free a previously allocated block of virtual memory.
/// addr must be the address returned by VirtualMemoryManagerAllocate, and size must be the same size.
Status VirtualMemoryManagerFree(VirtualMemoryManagerState *vmm, void *addr, size_t size);

#endif //BOREALOS_VIRTUALMEMORYMANAGER_H