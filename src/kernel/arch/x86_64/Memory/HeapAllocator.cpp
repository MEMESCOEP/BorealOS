#include "HeapAllocator.h"
#include <Settings.h>
#include "../KernelData.h"

// ReSharper disable CppDFAMemoryLeak (we free it, but the static analysis doesn't understand that)

struct AllocationHeader {
    size_t totalSize;
    uint32_t magic;
};

// C++ new operator stuff:
static constexpr uint32_t ALLOC_MAGIC = 0xDEADC0DE;
static constexpr size_t HEADER_SIZE = (sizeof(AllocationHeader) + 15) & ~15; // Aligned to 16 bytes.

uintptr_t SizedNewAllocate(size_t bytes) {
    size_t actualBytes = bytes + HEADER_SIZE;

    Memory::HeapAllocator* heap = &Kernel<KernelData>::GetInstance()->ArchitectureData->HeapAllocator;
    uintptr_t rawAddr = heap->Allocate({.bytes = actualBytes});

    // The allocation might fail, so we need to catch that
    if (!rawAddr) return 0;

    auto* header = reinterpret_cast<AllocationHeader*>(rawAddr);
    header->totalSize = actualBytes;
    header->magic = ALLOC_MAGIC;

    return rawAddr + HEADER_SIZE;
}

void FreeWithHeader(void* ptr) {
    if (!ptr) return;

    uintptr_t rawAddr = reinterpret_cast<uintptr_t>(ptr) - HEADER_SIZE;
    auto* header = reinterpret_cast<AllocationHeader*>(rawAddr);

    if (header->magic != ALLOC_MAGIC) {
        PANIC("Kernel Heap Corruption: Invalid magic in allocation header!");
    }

    Memory::HeapAllocator* heap = &Kernel<KernelData>::GetInstance()->ArchitectureData->HeapAllocator;
    heap->Free(rawAddr, header->totalSize);
}

void* operator new(size_t size) {
    return reinterpret_cast<void*>(SizedNewAllocate(size));
}

void* operator new[](size_t size) {
    return reinterpret_cast<void*>(SizedNewAllocate(size));
}

void operator delete(void* ptr, size_t size) noexcept {
    FreeWithHeader(ptr);
}

void operator delete(void* ptr) noexcept {
    FreeWithHeader(ptr);
}

void operator delete[](void* ptr) noexcept {
    FreeWithHeader(ptr);
}

void operator delete[](void* ptr, size_t size) noexcept {
    FreeWithHeader(ptr);
}

namespace Memory {
    HeapAllocator::HeapAllocator(PMM *pmm, Paging *paging, Paging::PagingState* pagingState) : _pmm(pmm), _paging(paging), _pagingState(pagingState) {
        uintptr_t phys = _pmm->AllocatePages(ALIGN_UP(InitialHeapSize, (size_t)Architecture::KernelPageSize) / (Architecture::KernelPageSize));
        if (!phys) {
            PANIC("Failed to allocate physical memory for heap!");
        }

        _paging->MapPages(HeapStart, phys, InitialHeapSize / Architecture::KernelPageSize, PageFlags::ReadWrite);
        _tlsf = tlsf_create_with_pool(reinterpret_cast<void*>(HeapStart), InitialHeapSize);
    }

    void HeapAllocator::Initialize() {
#if SETTING_TEST_MODE
        if (this->Test() != STATUS::SUCCESS) {
            PANIC("Heap allocator failed self test!");
        }
#endif
    }

    uintptr_t HeapAllocator::Allocate(AllocateArgs args) {
        size_t bytes = args.bytes;
        if (bytes == 0) return 0;

        // Switch to this heap's paging state.
        _paging->SwitchToPageTable(_pagingState);

        uintptr_t addr = (uintptr_t)tlsf_malloc(_tlsf, bytes);
        if (addr && args.mode == AllocateMode::Zeroed) {
            memset(reinterpret_cast<void*>(addr), 0, bytes);
        }

        return addr;
    }

    void HeapAllocator::Free(uintptr_t address, size_t bytes) {
        if (bytes == 0) return;

        // Switch to this heap's paging state.
        _paging->SwitchToPageTable(_pagingState);
        tlsf_free(_tlsf, reinterpret_cast<void*>(address));
    }

    void * HeapAllocator::Allocate(size_t size) {
        return reinterpret_cast<void*>(Allocate({.bytes = size}));
    }

    void HeapAllocator::Free(void *ptr, size_t size) {
        Free(reinterpret_cast<uintptr_t>(ptr), size);
    }

    STATUS HeapAllocator::Test() {
        // We can do some basic tests to ensure the heap allocator is working correctly, but we can't do exhaustive tests since we don't have a way to check for memory corruption or leaks.
        uintptr_t addr1 = Allocate({.bytes = 64});
        uintptr_t addr2 = Allocate({.bytes = 128});
        uintptr_t addr3 = Allocate({.bytes = 4096});
        uintptr_t addr4 = Allocate({.bytes = 8192, .mode = AllocateMode::Zeroed});

        LOG_DEBUG("addr1: %p, addr2: %p, addr3: %p, addr4: %p", addr1, addr2, addr3, addr4);
        if (!addr1 || !addr2 || !addr3 || !addr4) {
            LOG_ERROR("Heap allocator test failed: failed to allocate memory!");
            return STATUS::FAILURE;
        }

        if (*reinterpret_cast<uint8_t *>(addr4) != 0) {
            LOG_ERROR("Heap allocator test failed: zeroed allocation is not zeroed!");
            return STATUS::FAILURE;
        }

        // Read the memory at the allocated addresses to ensure they are valid and writable.

        Free(addr1, 64);
        Free(addr2, 128);
        Free(addr3, 4096);
        Free(addr4, 8192);

        // Reallocate them as they *should* share the same pointers now.
        uintptr_t addr5 = Allocate({.bytes = 64});
        uintptr_t addr6 = Allocate({.bytes = 128});
        uintptr_t addr7 = Allocate({.bytes = 4096});
        uintptr_t addr8 = Allocate({.bytes = 8192});

        LOG_DEBUG("addr5: %p, addr6: %p, addr7: %p, addr8: %p", addr5, addr6, addr7, addr8);
        if (addr5 != addr1 || addr6 != addr2 || addr7 != addr3 || addr8 != addr4) {
            LOG_ERROR("Heap allocator test failed: did not reuse freed memory!");
            return STATUS::FAILURE;
        }

        Free(addr5, 64);
        Free(addr6, 128);
        Free(addr7, 4096);
        Free(addr8, 8192);

        uintptr_t testUnaligned = Allocate({.bytes = 48});
        if (!testUnaligned) {
            LOG_ERROR("Heap allocator test failed: failed to allocate unaligned size!");
            return STATUS::FAILURE;
        }
        Free(testUnaligned, 48);
        uintptr_t testUnaligned2 = Allocate({.bytes = 48});
        if (testUnaligned2 != testUnaligned) {
            LOG_ERROR("Heap allocator test failed: did not reuse freed unaligned size!");
            return STATUS::FAILURE;
        }
        Free(testUnaligned2, 48);

        uintptr_t testUnaligned3 = Allocate({.bytes = 836});
        if (!testUnaligned3) {
            LOG_ERROR("Heap allocator test failed: failed to allocate unaligned size!");
            return STATUS::FAILURE;
        }
        Free(testUnaligned3, 836);

        struct ComplexObject {
            uint64_t data;
            ComplexObject() : data(0xACE) {}
            ~ComplexObject() { data = 0; }
        };

        size_t count = 4;
        auto* objs = new ComplexObject[count];

        for (size_t i = 0; i < count; i++) {
            if (objs[i].data != 0xACE) PANIC("ComplexObject corrupted");
        }

        delete[] objs;

        return STATUS::SUCCESS;
    }
}
