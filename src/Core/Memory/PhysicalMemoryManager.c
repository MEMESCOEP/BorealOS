#include <Core/Memory/PhysicalMemoryManager.h>

#include <Core/Graphics/Console.h>
#include <Core/Kernel/Panic.h>
#include <Core/Multiboot/MB2Parser.h>
#include <Drivers/IO/Serial.h>

#include "Utilities/MathUtils.h"
#include "Utilities/StrUtils.h"

extern void *_KernelEndMarker;

MemState_t physical_memory_manager_state = {
    .total_blocks = 0,
    .used_blocks = 0,
    .free_blocks = 0,
    .allocated_mask = NULL,
    .allocation_sizes = NULL,
    .lock = {0},
    .base_phys = 0,
};

void PhysicalMemoryManagerInit(void *MB2InfoPtr) {
    SendStringSerial(SERIAL_COM1,"Initializing Physical Memory Manager...\n\r");

    MB2MemoryMap_t *memoryMap = (MB2MemoryMap_t *)FindMB2Tag(MB2_TAG_MEMORYMAP, MB2InfoPtr);

    if (memoryMap == NULL) {
        SendStringSerial(SERIAL_COM1, "No memory map found!\n\r");
        KernelPanic(PHYSICAL_MEMORY_MANAGER_ERROR_MEMMAP_NULL, "Memory map is NULL! Kernel cannot continue.");
        return;
    }

    SendStringSerial(SERIAL_COM1, "Memory map found, initializing memory manager...\n\r");

    int count = 0;
    struct {
        uint64_t addr;
        uint64_t len;
    } usable_entries[PHYSICAL_MEMORY_MANAGER_MAXIMUM_MAP_ENTRIES];  // adjust size if expecting more than 16 large regions

    int32_t total_memory = 0;

    for (uint8_t *entry_ptr = (uint8_t *)memoryMap->entries;
         entry_ptr < ((uint8_t *)memoryMap) + memoryMap->size;
         entry_ptr += memoryMap->entry_size) {

        struct multiboot_mmap_entry *entry = (struct multiboot_mmap_entry *)entry_ptr;
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE){
            if (entry->len < PHYSICAL_MEMORY_MANAGER_MINIMUM_MAP_ENTRY_SIZE) {
                continue; // Skip entries that are too small
            }
            if (count >= PHYSICAL_MEMORY_MANAGER_MAXIMUM_MAP_ENTRIES) {
                SendStringSerial(SERIAL_COM1, "Too many usable memory entries found, increase usable_entries size.\n\r");
                KernelPanic(-1, "Too many usable memory entries found.");
                return;
            }

            SendStringSerial(SERIAL_COM1, "Found memory entry, i: ");
            char i[25];
            IntToStr(count, &i[0], 10);
            SendStringSerial(SERIAL_COM1, &i[0]);

            IntToStr(entry->addr, &i[0], 16);
            SendStringSerial(SERIAL_COM1, " with address: 0x");
            SendStringSerial(SERIAL_COM1, &i[0]);

            IntToStr(entry->len, &i[0], 16);
            SendStringSerial(SERIAL_COM1, " and length: 0x");
            SendStringSerial(SERIAL_COM1, &i[0]);
            SendStringSerial(SERIAL_COM1, "\n\r");

            total_memory += entry->len;

            usable_entries[count].addr = entry->addr;
            usable_entries[count].len  = entry->len;

            count += 1;
        }
    }

    char total_memory_str[25];
    int32_t total_memory_kb = total_memory / 1024;
    IntToStr(total_memory_kb, &total_memory_str[0], 10);
    SendStringSerial(SERIAL_COM1, "Total memory found: ");
    SendStringSerial(SERIAL_COM1, &total_memory_str[0]);
    SendStringSerial(SERIAL_COM1, " KB\n\r");

    uint64_t region_base = usable_entries[0].addr;
    uint64_t region_len  = usable_entries[0].len;

    // Compute how many 4 KB blocks fit, and how much metadata is needed
    uint64_t total_blocks = region_len / PHYSICAL_MEMORY_MANAGER_MINIMUM_SIZE;
    uint64_t bitmap_bytes = (total_blocks + 7) / 8;
    uint64_t sizes_bytes  = total_blocks * sizeof(uint32_t);
    uint64_t metadata_bytes = bitmap_bytes + sizes_bytes;

    while (region_len < metadata_bytes + PHYSICAL_MEMORY_MANAGER_MINIMUM_SIZE) {
        total_blocks--;
        bitmap_bytes = (total_blocks + 7) / 8;
        sizes_bytes  = total_blocks * sizeof(uint32_t);
        metadata_bytes = bitmap_bytes + sizes_bytes;
    }

    uint64_t adjusted_base = AlignUp((uint64_t)&_KernelEndMarker, PHYSICAL_MEMORY_MANAGER_MINIMUM_SIZE);

    uint32_t *sizes_ptr = (uint32_t *)(adjusted_base + bitmap_bytes);
    uint8_t *bitmap_ptr = (uint8_t *)adjusted_base;
    uintptr_t heap_base_phys = usable_entries[0].addr + (adjusted_base - region_base);
    if (heap_base_phys % PHYSICAL_MEMORY_MANAGER_MINIMUM_SIZE != 0) {
        heap_base_phys = AlignUp(heap_base_phys, PHYSICAL_MEMORY_MANAGER_MINIMUM_SIZE);
    }
    if (heap_base_phys + metadata_bytes > usable_entries[0].addr + usable_entries[0].len) {
        SendStringSerial(SERIAL_COM1, "Not enough space for metadata and minimum block size.\n\r");
        KernelPanic(-1, "Not enough space for metadata and minimum block size.");
        return;
    }

    physical_memory_manager_state.total_blocks     = (uint32_t)total_blocks;
    physical_memory_manager_state.free_blocks      = (uint32_t)total_blocks;
    physical_memory_manager_state.used_blocks      = 0;
    physical_memory_manager_state.allocated_mask   = bitmap_ptr;
    physical_memory_manager_state.allocation_sizes = sizes_ptr;
    physical_memory_manager_state.base_phys        = heap_base_phys;
    spinlock_init(&physical_memory_manager_state.lock);

    MemSet(bitmap_ptr, 0, bitmap_bytes);
    MemSet(sizes_ptr,  0, sizes_bytes);

    char buf[64];
    IntToStr((int32_t)total_blocks, buf, 10);
    SendStringSerial(SERIAL_COM1, "PMM: total blocks = ");
    SendStringSerial(SERIAL_COM1, buf);
    SendStringSerial(SERIAL_COM1, " (");
    IntToStr((int32_t)(total_blocks * PHYSICAL_MEMORY_MANAGER_MINIMUM_SIZE), buf, 10);
    SendStringSerial(SERIAL_COM1, buf);
    SendStringSerial(SERIAL_COM1, " bytes) initialized.\n\r");

    // Do some tests, allocate 100 ints, fill them with i * i, and print them out
    void *test_alloc = PhysicalMemoryManagerAlloc(100 * sizeof(int), PHYSICAL_MEMORY_MANAGER_MINIMUM_SIZE);
    if (test_alloc == NULL) {
        SendStringSerial(SERIAL_COM1, "PMM: Test allocation failed!\n\r");
        KernelPanic(-2, "Test allocation failed.");
        return;
    }
    SendStringSerial(SERIAL_COM1, "PMM: Test allocation successful at address: 0x");
    char addr_buf[25];
    IntToStr((int32_t)(uintptr_t)test_alloc, addr_buf, 16);
    SendStringSerial(SERIAL_COM1, addr_buf);
    SendStringSerial(SERIAL_COM1, "\n\r");
    for (int i = 0; i < 100; i++) {
        ((int *)test_alloc)[i] = i * i;
    }

    SendStringSerial(SERIAL_COM1, "PMM: Test allocation filled with squares:\n\r");

    for (int i = 0; i < 100; i++) {
        if (i * i != ((int*) test_alloc)[i]) {
            // Panic
            SendStringSerial(SERIAL_COM1, "PMM: Test allocation verification failed at index ");
            char index_buf[25];
            IntToStr(i, index_buf, 10);
            SendStringSerial(SERIAL_COM1, index_buf);
            SendStringSerial(SERIAL_COM1, ", expected ");
            IntToStr(i * i, index_buf, 10);
            SendStringSerial(SERIAL_COM1, index_buf);
            SendStringSerial(SERIAL_COM1, ", got ");
            IntToStr(((int*) test_alloc)[i], index_buf, 10);
            SendStringSerial(SERIAL_COM1, index_buf);
            SendStringSerial(SERIAL_COM1, "\n\r");
            KernelPanic(-2, "Test allocation verification failed.");
        }
    }

    SendStringSerial(SERIAL_COM1, "PMM: Test allocation verified successfully.\n\r");
    PhysicalMemoryManagerFree(test_alloc, 100 * sizeof(int));
    SendStringSerial(SERIAL_COM1, "PMM: Test allocation freed successfully.\n\r");
    SendStringSerial(SERIAL_COM1, "Physical Memory Manager initialized successfully. Have fun!!\n\r");
}

void * PhysicalMemoryManagerAlloc(uint32_t size, uint32_t align) {
    if (size == 0) {
        return NULL;
    }

    uint32_t blocks_needed = (size + PHYSICAL_MEMORY_MANAGER_MINIMUM_SIZE - 1) / PHYSICAL_MEMORY_MANAGER_MINIMUM_SIZE;

    spinlock_lock(&physical_memory_manager_state.lock);

    uint32_t total = physical_memory_manager_state.total_blocks;
    uint8_t* bitmap = physical_memory_manager_state.allocated_mask;
    uint32_t* sizes = physical_memory_manager_state.allocation_sizes;
    uintptr_t base = physical_memory_manager_state.base_phys;

    for (uint32_t idx = 0; idx + blocks_needed <= total; idx++) {
        uintptr_t candidate_phys = base + (uintptr_t)idx * PHYSICAL_MEMORY_MANAGER_MINIMUM_SIZE;
        if ((candidate_phys & (align - 1)) != 0) {
            continue;
        }
        uint32_t run = 0;
        for (uint32_t j = 0; j < blocks_needed; j++) {
            uint32_t byte_index = (idx + j) >> 3;
            uint32_t bit_index  = (idx + j) & 7;
            if (bitmap[byte_index] & (1 << bit_index)) {
                break;  // encountered an allocated block
            }
            run++;
        }
        if (run < blocks_needed) {
            continue;  // not enough free blocks here
        }
        for (uint32_t j = 0; j < blocks_needed; j++) {
            uint32_t byte_index = (idx + j) >> 3;
            uint32_t bit_index  = (idx + j) & 7;
            bitmap[byte_index] |= (1 << bit_index);

            sizes[idx + j] = (j == 0 ? (blocks_needed * PHYSICAL_MEMORY_MANAGER_MINIMUM_SIZE) : 0);
        }
        physical_memory_manager_state.used_blocks += blocks_needed;
        physical_memory_manager_state.free_blocks -= blocks_needed;
        spinlock_unlock(&physical_memory_manager_state.lock);
        return (void*)(uintptr_t)candidate_phys;
    }
    spinlock_unlock(&physical_memory_manager_state.lock);
    return NULL;
}

void PhysicalMemoryManagerFree(void *ptr, uint32_t size) {
    if (ptr == NULL || size == 0) {
        return;
    }

    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t base = physical_memory_manager_state.base_phys;
    if (addr < base || ((addr - base) % PHYSICAL_MEMORY_MANAGER_MINIMUM_SIZE) != 0) {
        // Invalid pointer (not aligned to block boundary)
        KernelPanic(-3, "PMM free: invalid or unaligned pointer");
        return;
    }

    uint32_t start_idx = (uint32_t)((addr - base) / PHYSICAL_MEMORY_MANAGER_MINIMUM_SIZE);
    if (start_idx >= physical_memory_manager_state.total_blocks) {
        KernelPanic(-4, "PMM free: pointer out of range");
        return;
    }

    spinlock_lock(&physical_memory_manager_state.lock);

    uint32_t alloc_size = physical_memory_manager_state.allocation_sizes[start_idx];
    if (alloc_size == 0 || alloc_size != AlignUp(size, PHYSICAL_MEMORY_MANAGER_MINIMUM_SIZE)) {
        spinlock_unlock(&physical_memory_manager_state.lock);
        KernelPanic(-5, "PMM free: size mismatch or double free");
        return;
    }

    uint32_t blocks_to_free = (alloc_size + PHYSICAL_MEMORY_MANAGER_MINIMUM_SIZE - 1) / PHYSICAL_MEMORY_MANAGER_MINIMUM_SIZE;
    for (uint32_t i = 0; i < blocks_to_free; i++) {
        uint32_t idx = start_idx + i;
        uint32_t byte_index = idx >> 3;
        uint32_t bit_index  = idx & 7;
        physical_memory_manager_state.allocated_mask[byte_index] &= (uint8_t)~(1 << bit_index);
        physical_memory_manager_state.allocation_sizes[idx] = 0;
    }

    physical_memory_manager_state.used_blocks -= blocks_to_free;
    physical_memory_manager_state.free_blocks += blocks_to_free;

    spinlock_unlock(&physical_memory_manager_state.lock);
}
