#ifndef PHYSICALALLOCATOR_H
#define PHYSICALALLOCATOR_H

#include <stdint.h>
#include <stddef.h>

#include "../../Limine.h"
#include "../Threading/Threading.h"

#define PAGE_SIZE 0x1000 // 4 KB
#define MINIMUM_MEMMAP_SIZE 0x100000 // 1 MB

// Errors:
#define PHYSICALALLOCATOR_ERROR_NO_APPROPRIATE_MEMMAP_ENTRY -50
#define PHYSICALALLOCATOR_ERROR_NO_MEMORY -51
#define PHYSICALALLOCATOR_ERROR_INVALID_ADDRESS -52
#define PHYSICALALLOCATOR_ERROR_INVALID_SIZE -53
#define PHYSICALALLOCATOR_ERROR_NOT_ALIGNED -54
#define PHYSICALALLOCATOR_ERROR_OUT_OF_MEMORY -55

typedef struct {
    uint64_t TotalBlocks;
    uint64_t FreeBlocks;
    uint64_t BlockSize;

    uint8_t *AllocatedMask;
    uint64_t *AllocationSizes; // Keep track of how many blocks are allocated, so at index [malloc's return value] we'll have the size of the allocation
    uint64_t HHDMOffset;

    SPINLOCK Lock;
} PHYSICAL_MEMSTATE;

extern PHYSICAL_MEMSTATE PhysicalMemoryState;

void InitPhysicalMemoryAllocator();
void TestPhysicalMemoryAllocator();

void* AllocatePhysicalMemory(uint64_t Size);
void FreePhysicalMemory(void* Address);

#endif //PHYSICALALLOCATOR_H
