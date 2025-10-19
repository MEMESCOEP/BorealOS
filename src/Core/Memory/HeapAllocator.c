#include "HeapAllocator.h"

#include "Memory.h"
#include "PhysicalMemoryManager.h"
#include "VirtualMemoryManager.h"
#include "Core/Kernel.h"

// This is a simple heap allocator that uses a virtual memory manager to allocate and free memory.
// It uses a linked list to track free and used blocks of memory.
// It can grow the heap as needed, up to a maximum size.

typedef struct HeapBlock {
    size_t size; // Size of the block, including the header
    bool free; // Whether the block is free or used
    struct HeapBlock* next; // Pointer to the next block in the linked list
    uint64_t magic;
} HeapBlock;

static inline int is_in_heap(HeapAllocatorState *heap, void *p) {
    uintptr_t base = (uintptr_t)heap->Data;
    uintptr_t v = (uintptr_t)p;
    return (v >= base) && (v < base + heap->TotalSize);
}

#define HEAP_MAGIC 0x4845504D45474701ULL  // "HEPMG..." example
#define HEAP_MAGIC_FREED 0xDEADBEEFDEADBEEFULL

Status HeapAllocatorInit(HeapAllocatorState *heap, size_t initialSize, size_t minSize, size_t maxSize,
    VirtualMemoryManagerState *vmm) {
    if (initialSize < minSize) initialSize = minSize;
    if (initialSize > maxSize) initialSize = maxSize;
    if (minSize > maxSize) return STATUS_INVALID_PARAMETER;

    heap->TotalSize = 0;
    heap->MinSize = minSize;
    heap->MaxSize = maxSize;
    heap->UsedSize = 0;
    heap->VMM = vmm;
    heap->Data = nullptr;

    initialSize = ALIGN_UP(initialSize, PMM_PAGE_SIZE);

    // Request initial memory from the VMM
    void* addr = VirtualMemoryManagerAllocate(vmm, initialSize, true, false);
    if (!addr) return STATUS_FAILURE;
    heap->TotalSize = initialSize;
    heap->UsedSize = 0; // We don't count the header as used

    HeapBlock* block = (HeapBlock*)addr;
    block->size = initialSize;
    block->free = true;
    block->next = nullptr;
    block->magic = HEAP_MAGIC;

    heap->Data = block;
    return STATUS_SUCCESS;
}

void * HeapAllocate(HeapAllocatorState *heap, size_t req_size, size_t alignment) {
    if (!heap) return nullptr;
    HeapBlock* current = (HeapBlock*)heap->Data;
    if (!current) return nullptr;

    // Sanitize inputs
    if (alignment == 0) alignment = 8;
    if (alignment & (alignment - 1)) return nullptr; // not power-of-two
    req_size = ALIGN_UP(req_size, 8);

    // Prevent overflow when adding header
    if (req_size > SIZE_MAX - sizeof(HeapBlock)) return nullptr;
    size_t total_req = req_size + sizeof(HeapBlock); // total bytes needed in a single block

    HeapBlock* selectedBlock = nullptr;
    while (current) {
        if (current->free && current->size >= total_req) {
            // compute alignment padding relative to data area
            uintptr_t dataAddr = (uintptr_t)current + sizeof(HeapBlock);
            uintptr_t alignedData = ALIGN_UP(dataAddr, alignment);
            size_t padding = (size_t)(alignedData - dataAddr);

            // ensure block has space for header and padding
            // current->size is total block size including header
            if (current->size >= total_req + padding) {
                selectedBlock = current;
                break;
            }
        }
        current = current->next;
    }

    if (selectedBlock) {
        // recompute safely
        uintptr_t selectedBase = (uintptr_t)selectedBlock;
        uintptr_t dataAddr = selectedBase + sizeof(HeapBlock);
        uintptr_t alignedData = ALIGN_UP(dataAddr, alignment);
        size_t padding = (size_t)(alignedData - dataAddr);

        // if padding needed, create a small leading block
        if (padding >= sizeof(HeapBlock)) {
            uintptr_t paddingBlockAddr = alignedData - sizeof(HeapBlock);
            HeapBlock* paddingBlock = (HeapBlock*)paddingBlockAddr;
            paddingBlock->size = selectedBlock->size - padding - sizeof(HeapBlock);
            paddingBlock->free = true;
            paddingBlock->next = selectedBlock->next;
            paddingBlock->magic = HEAP_MAGIC;

            selectedBlock->size = padding + sizeof(HeapBlock);
            selectedBlock->next = paddingBlock;
            selectedBlock = paddingBlock;
        }

        // split if large enough to leave a remaining free block
        if (selectedBlock->size > total_req + sizeof(HeapBlock)) {
            uintptr_t selBase = (uintptr_t)selectedBlock;
            uintptr_t remainingAddr = selBase + total_req;
            // bounds check
            if (remainingAddr + sizeof(HeapBlock) > selBase + selectedBlock->size) {
                // shouldn't happen due to check above, but be defensive
                return nullptr;
            }
            HeapBlock* remaining = (HeapBlock*)remainingAddr;
            remaining->size = selectedBlock->size - total_req;
            remaining->free = true;
            remaining->next = selectedBlock->next;
            remaining->magic = HEAP_MAGIC;

            selectedBlock->next = remaining;
            selectedBlock->size = total_req;
        }

        // allocate
        selectedBlock->free = false;
        heap->UsedSize += selectedBlock->size;
        selectedBlock->magic = HEAP_MAGIC;

        void* userAddr = (void*)((uintptr_t)selectedBlock + sizeof(HeapBlock));
        if (userAddr < (void*)(MiB * 1)) {
            PANIC("HeapAllocate: Allocated an address below 1MB!\n");
        }
        return userAddr;
    }

    // grow heap
    size_t newSize = heap->TotalSize ? heap->TotalSize * 2 : PMM_PAGE_SIZE;
    if (newSize < total_req) newSize = total_req;
    newSize = ALIGN_UP(newSize, PMM_PAGE_SIZE);

    if (newSize > heap->MaxSize - heap->TotalSize) {
        newSize = heap->MaxSize - heap->TotalSize;
        if (newSize < total_req) return nullptr;
    }

    void* addr = VirtualMemoryManagerAllocate(heap->VMM, newSize, true, false);
    if (!addr) return nullptr;

    heap->TotalSize += newSize;
    HeapBlock* newBlock = (HeapBlock*)addr;
    newBlock->size = newSize;
    newBlock->free = true;
    newBlock->next = (HeapBlock*)heap->Data;
    heap->Data = newBlock;

    // retry with a clean, well-defined requested user size
    return HeapAllocate(heap, req_size, alignment);
}

void * HeapAlloc(HeapAllocatorState *heap, size_t size) {
    // Just call HeapAllocate with default alignment of 8 bytes
    return HeapAllocate(heap, size, 8);
}

void * HeapRealloc(HeapAllocatorState *heap, void *addr, size_t size) {
    // First allocate a new block
    void* newBlock = HeapAlloc(heap, size);
    volatile HeapBlock* blockHeader = (HeapBlock*)((size_t)newBlock - sizeof(HeapBlock));
    (void)blockHeader;
    if (!newBlock) return nullptr;
    if (addr) {
        // Copy the old data to the new block
        HeapBlock* oldBlock = (HeapBlock*)((size_t)addr - sizeof(HeapBlock));
        size_t copySize = oldBlock->size - sizeof(HeapBlock);
        if (copySize > size) copySize = size;
        memcpy(newBlock, addr, copySize);
        // Free the old block
        HeapFree(heap, addr);
    }

    return newBlock;
}

void HeapFree(HeapAllocatorState *heap, void *addr) {
    if (!addr) return;

    // compute block pointer and basic validation
    uintptr_t user = (uintptr_t)addr;
    if (user < sizeof(HeapBlock)) PANIC("HeapFree: user pointer too small\n");
    HeapBlock *block = (HeapBlock*)(user - sizeof(HeapBlock));

    if (!is_in_heap(heap, block)) PANIC("HeapFree: pointer not in heap\n");

    // header sanity checks (add these fields to HeapBlock in debug builds)
    if (block->magic != HEAP_MAGIC)
        PANIC("HeapFree: header magic mismatch (corruption)\n");
    if (block->size < sizeof(HeapBlock) || block->size > heap->TotalSize)
        PANIC("HeapFree: invalid block size\n");

    if (block->free) PANIC("HeapFree: double free detected\n");

    // poison payload to expose use-after-free
    uintptr_t payload_sz = block->size - sizeof(HeapBlock);
    void *payload = (void*)((uintptr_t)block + sizeof(HeapBlock));
    memset(payload, 0xAB, payload_sz);

    // mark as freed and update metrics
    block->free = true;
    block->magic = HEAP_MAGIC_FREED;
    heap->UsedSize -= block->size;

    // Coalesce adjacent free blocks safely.
    // Walk list but always validate next pointer and bounds before deref.
    HeapBlock *current = (HeapBlock*)heap->Data;
    uintptr_t heap_base = (uintptr_t)heap->Data;
    uintptr_t heap_end = heap_base + heap->TotalSize;

    while (current) {
        // validate current header
        if (!is_in_heap(heap, current)) PANIC("HeapFree: current pointer out of heap\n");
        if (current->size < sizeof(HeapBlock) || (uintptr_t)current + current->size > heap_end) {
            PANIC("HeapFree: corrupted current->size\n");
        }

        HeapBlock *next = current->next;
        if (!next) {
            current = NULL;
            continue;
        }

        // validate next before using
        if (!is_in_heap(heap, next)) {
            PANIC("HeapFree: invalid next pointer detected (possible corruption)\n");
        }
        if (next->size < sizeof(HeapBlock) || (uintptr_t)next + next->size > heap_end) {
            PANIC("HeapFree: corrupted next->size\n");
        }

        // only merge if both free and physically adjacent
        if (current->free && next->free) {
            uintptr_t current_end = (uintptr_t)current + current->size;
            uintptr_t next_start = (uintptr_t)next;
            if (current_end == next_start) {
                // safe to merge
                // check for addition overflow
                size_t new_size = current->size + next->size;
                if (new_size < current->size) PANIC("HeapFree: size overflow on merge\n");
                current->size = new_size;
                current->next = next->next;
                // continue on current to try multi-merge
                continue;
            }
        }

        current = current->next;
    }
}

Status HeapAllocatorTest(HeapAllocatorState *heap) {
    // Allocate a few small blocks of memory and verify they are usable
    LOG(LOG_INFO, "Testing Heap Allocator...\n");
    size_t currentDataPtr = (size_t)heap->Data;
    size_t currentSize = heap->TotalSize;
    void* block1 = HeapAlloc(heap, 64);
    if (!block1) {
        PANIC("HeapAlloc failed for block1!\n");
    }
    void* block2 = HeapAlloc(heap, 128);
    if (!block2) {
        PANIC("HeapAlloc failed for block2!\n");
    }
    void* block3 = HeapAlloc(heap, 256);
    if (!block3) {
        PANIC("HeapAlloc failed for block3!\n");
    }

    if (block1 == block2 || block1 == block3 || block2 == block3) {
        PANIC("HeapAllocatorTest: Allocation returned the same memory address!\n");
    }

    // Write some data to the blocks to verify they are usable
    volatile uint8_t* p1 = (uint8_t*)block1;
    volatile uint8_t* p2 = (uint8_t*)block2;
    volatile uint8_t* p3 = (uint8_t*)block3;
    for (size_t i = 0; i < 64; i++) {
        p1[i] = 0xAA;
        p2[i] = 0x55;
        p3[i] = 0xFF;
    }
    bool success = true;
    for (size_t i = 0; i < 64; i++) {
        if (p1[i] != 0xAA) {
            success = false;
            break;
        }
        if (p2[i] != 0x55) {
            success = false;
            break;
        }
        if (p3[i] != 0xFF) {
            success = false;
            break;
        }
    }

    void* p4 = HeapRealloc(heap, block3, 512);
    if (!p4) {
        PANIC("HeapRealloc failed for block3!\n");
    }
    p3 = (uint8_t*)p4;
    for (size_t i = 256; i < 512; i++) {
        p3[i] = 0xEE;
    }
    for (size_t i = 0; i < 512; i++) {
        if (i < 64) {
            if (p3[i] != 0xFF) {
                success = false;
                break;
            }
        } else if (i >= 256) {
            if (p3[i] != 0xEE) {
                success = false;
                break;
            }
        }
    }

    if (!success) {
        PANIC("HeapAllocatorTest: Memory test failed! Data corruption detected!\n");
    }

    LOG(LOG_INFO, "Heap memory test passed! Freeing blocks...\n");
    HeapFree(heap, block1);
    HeapFree(heap, block2);
    HeapFree(heap, p4); // p4 is the reallocated block3

    // There should now only be 1 large free block
    HeapBlock* current = (HeapBlock*)heap->Data;
    size_t freeBlocks = 0;
    while (current) {
        if (current->free) freeBlocks++;
        current = current->next;
    }

    if (freeBlocks != 1 || (size_t)heap->Data != currentDataPtr || heap->TotalSize != currentSize) {
        // Something went wrong
        PANIC("HeapAllocatorTest: Memory leak detected or heap state invalid after freeing blocks!\n");
    }

    LOG(LOG_INFO, "Heap Allocator test completed successfully.\n");
    return STATUS_SUCCESS;
}