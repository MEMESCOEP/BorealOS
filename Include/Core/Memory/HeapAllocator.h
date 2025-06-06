#ifndef HEAPALLOCATOR_H
#define HEAPALLOCATOR_H

// This is the allocator that provides malloc & free functionality!
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <Core/Threading/Threading.h>

typedef struct HeapBlockHeader {
    uint32_t size;
    bool is_free; // true if the block is free, false if it is allocated
    struct HeapBlockHeader *next; // Pointer to the next block in the linked list
} HeapBlockHeader_t;

typedef struct {
    uintptr_t heap_start;
    uintptr_t heap_end;
    HeapBlockHeader_t *list;
    size_t total_size; // Total size of the heap
    size_t used_size;  // Total size of used memory
    spinlock_t lock; // Spinlock for thread safety
} HeapAllocator_t;

#endif //HEAPALLOCATOR_H
