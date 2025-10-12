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
} HeapBlock;

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

    heap->Data = block;
    return STATUS_SUCCESS;
}

void * HeapAllocate(HeapAllocatorState *heap, size_t size, size_t alignment) {
    HeapBlock* current = (HeapBlock*)heap->Data;
    if (!current) return nullptr;

    size = ALIGN_UP(size, 8); // Align size to 8 bytes

    if (alignment && (alignment & (alignment - 1)) != 0) {
        // Alignment is not a power of two
        return nullptr;
    }

    size += sizeof(HeapBlock); // Include header size
    HeapBlock* selectedBlock = nullptr;
    while (current) {
        if (current->free && current->size >= size) {
            // Check alignment
            size_t dataAddr = (size_t)current + sizeof(HeapBlock);
            size_t alignedAddr = ALIGN_UP(dataAddr, alignment);
            size_t padding = alignedAddr - dataAddr;
            if (current->size >= size + padding) {
                selectedBlock = current;
                break;
            }
        }
        current = current->next;
    }

    if (selectedBlock) {
        size_t dataAddr = (size_t)selectedBlock + sizeof(HeapBlock);
        size_t alignedAddr = ALIGN_UP(dataAddr, alignment);
        size_t padding = alignedAddr - dataAddr;

        // If we need padding, create a new block for the padding.
        if (padding > 0) {
            // We need to calculate with the header size in mind, otherwise we'll overwrite memory
            if (selectedBlock->size <= padding + sizeof(HeapBlock)) {
                // Not enough space for padding block
                return nullptr;
            }
            HeapBlock* paddingBlock = (HeapBlock*)((size_t)selectedBlock + sizeof(HeapBlock) + padding);
            paddingBlock->size = selectedBlock->size - padding - sizeof(HeapBlock);
            paddingBlock->free = true;
            paddingBlock->next = selectedBlock->next;
            selectedBlock->next = paddingBlock;
            selectedBlock->size = padding + sizeof(HeapBlock);
            selectedBlock = paddingBlock;
            size += padding; // Increase the size to account for the padding
        }

        size_t totalSizeNeeded = size; // This already includes the header size
        if (selectedBlock->size > totalSizeNeeded + sizeof(HeapBlock)) { // Split the block if it's significantly larger
            HeapBlock* remaining = (HeapBlock*)((size_t)selectedBlock + totalSizeNeeded);
            remaining->size = selectedBlock->size - totalSizeNeeded;
            remaining->free = true;
            remaining->next = selectedBlock->next;
            selectedBlock->next = remaining;
            selectedBlock->size = totalSizeNeeded;
        }

        selectedBlock->free = false;
        heap->UsedSize += selectedBlock->size;

        return (void*)((size_t)selectedBlock + sizeof(HeapBlock));
    }

    // No suitable block found, try to grow the heap
    size_t newSize = heap->TotalSize * 2; // Double the heap size
    if (newSize < size) newSize = size;
    newSize = ALIGN_UP(newSize, PMM_PAGE_SIZE);
    if (newSize > heap->MaxSize - heap->TotalSize) {
        newSize = heap->MaxSize - heap->TotalSize;
        if (newSize < size) return nullptr; // Cannot satisfy the allocation
    }

    // Request more memory from the VMM
    void* addr = VirtualMemoryManagerAllocate(heap->VMM, newSize, true, false);
    if (!addr) return nullptr;
    heap->TotalSize += newSize;
    HeapBlock* newBlock = (HeapBlock*)addr;
    newBlock->size = newSize;
    newBlock->free = true;
    newBlock->next = (HeapBlock*)heap->Data;
    heap->Data = newBlock;

    return HeapAllocate(heap, size - sizeof(HeapBlock), alignment); // Retry allocation
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
    // Get the block header
    if (!addr) return;
    HeapBlock* block = (HeapBlock*)((size_t)addr - sizeof(HeapBlock));
    if (block->free) {
        PANIC("Double free detected in HeapFree!\n");
    }

    block->free = true;
    heap->UsedSize -= block->size;

    HeapBlock* current = (HeapBlock*)heap->Data;
    while (current) {
        if (current->free && current->next && current->next->free) {
            size_t currentEnd = (size_t)current + current->size;
            size_t nextStart = (size_t)current->next;
            if (currentEnd == nextStart) {
                // Merge current and next
                current->size += current->next->size;
                current->next = current->next->next;
                continue; // stay on current to check further merges
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
