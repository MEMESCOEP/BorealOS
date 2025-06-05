/*

This is an adaptation of Martin's custom pmm from the Limine based kernel.

*/

#ifndef PHYSICALMEMORYMANAGER_H
#define PHYSICALMEMORYMANAGER_H

#include <stdint.h>
#include <Core/Threading/Threading.h>

typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    struct {
        uint64_t addr;
        uint64_t len;
        uint32_t type;
        uint32_t reserved;
    } entries[];
} __attribute__((packed)) MB2MemoryMap_t;

typedef struct {
    uint32_t total_blocks;
    uint32_t used_blocks;
    uint32_t free_blocks;
    uint8_t* allocated_mask; // Bitmap to track allocated blocks
    uint32_t* allocation_sizes; // Array to track sizes of allocations
    spinlock_t lock; // Spinlock for potential thread safety (If we ever support multithreading)
    uintptr_t base_phys; // Base physical address for allocations
} MemState_t;

#define PHYSICAL_MEMORY_MANAGER_MINIMUM_SIZE 0x1000 // 4 KB (minimum page size)
#define PHYSICAL_MEMORY_MANAGER_MINIMUM_MAP_ENTRY_SIZE 0x100000 // 1 MB (minimum size for a usable memory map entry)
#define MB2_TAG_MEMORYMAP 6 // Multiboot2 tag for memory map (https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html (3.6.8))
#define PHYSICAL_MEMORY_MANAGER_MAXIMUM_MAP_ENTRIES 16

#define PHYSICAL_MEMORY_MANAGER_ERROR_MEMMAP_NULL -1

extern MemState_t physical_memory_manager_state;

void PhysicalMemoryManagerInit(void* MB2InfoPtr);

void* PhysicalMemoryManagerAlloc(uint32_t size, uint32_t align);
void PhysicalMemoryManagerFree(void* ptr, uint32_t size);

#endif //PHYSICALMEMORYMANAGER_H
