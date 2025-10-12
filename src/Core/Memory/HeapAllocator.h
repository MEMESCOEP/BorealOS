#ifndef BOREALOS_HEAPALLOCATOR_H
#define BOREALOS_HEAPALLOCATOR_H

#include <Definitions.h>


typedef void* HeapAllocatorData;
typedef struct VirtualMemoryManagerState VirtualMemoryManagerState;

typedef struct HeapAllocatorState {
    size_t TotalSize; // Total size of the heap in bytes
    size_t MinSize; // Minimum size of the heap in bytes
    size_t MaxSize; // Maximum size of the heap in bytes
    size_t UsedSize; // Currently used size of the heap in bytes
    VirtualMemoryManagerState* VMM; // The VMM this heap is allocated from
    HeapAllocatorData Data; // Opaque pointer to allocator-specific data
} HeapAllocatorState;

/// Initialize the heap allocator with at least the given initial size.
/// The heap will grow as needed, up to maxSize.
Status HeapAllocatorInit(HeapAllocatorState* heap, size_t initialSize, size_t minSize, size_t maxSize, VirtualMemoryManagerState* vmm);

/// Allocate a block of memory from the heap.
/// Returns a pointer to the allocated memory, or NULL on failure.
/// You can pass in a required alignment (must be a power of two), or 0 for default alignment (8 bytes).
void* HeapAllocate(HeapAllocatorState* heap, size_t size, size_t alignment);

/// Allocate a block of memory from the heap with default alignment (8 bytes).
/// Returns a pointer to the allocated memory, or NULL on failure.
void* HeapAlloc(HeapAllocatorState* heap, size_t size);

/// Reallocate a previously allocated block of memory to a new size.
/// addr must be the address returned by HeapAllocate or HeapAlloc.
/// If the reallocation is successful, the contents of the old block are copied to the new block up to the minimum of the old and new sizes.
/// If the reallocation fails, NULL is returned and the original block remains valid.
/// If addr is NULL, this function behaves like HeapAlloc.
/// If size is 0, this function behaves like HeapFree and frees the block.
void* HeapRealloc(HeapAllocatorState* heap, void* addr, size_t size);

/// Free a previously allocated block of memory.
/// addr must be the address returned by HeapAllocate or HeapAlloc.
void HeapFree(HeapAllocatorState* heap, void* addr);

/// Perform a simple test of the heap allocator by allocating and freeing some memory.
Status HeapAllocatorTest(HeapAllocatorState* heap);

#endif //BOREALOS_HEAPALLOCATOR_H