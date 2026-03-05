#ifndef BOREALOS_HEAPALLOCATOR_H
#define BOREALOS_HEAPALLOCATOR_H

#include <Definitions.h>

#include "Allocator.h"
#include "Kernel.h"
#include "Paging.h"
#include "PMM.h"

#include <tlsf.h> // We use the TLSF allocator, since its easy and efficient.

// TODO: Implement spin locks for the heap allocator when moving to multi core.
namespace Memory {
    class HeapAllocator : public Allocator {
    public:
        enum class AllocateMode {
            Normal, // Just allocate the memory, no special requirements
            Zeroed // Allocate the memory and zero it out
        };

        struct AllocateArgs {
            size_t bytes = 0;
            AllocateMode mode = AllocateMode::Normal;
        };

        explicit HeapAllocator(PMM* pmm, Paging* paging, Paging::PagingState* pagingState);
        ~HeapAllocator() override = default;
        void Initialize();

        [[nodiscard]] uintptr_t Allocate(AllocateArgs args);
        void Free(uintptr_t address, size_t bytes);

        void *Allocate(size_t size) override;
        void Free(void *ptr, size_t size) override;

        static constexpr size_t InitialHeapSize = 32 * Constants::MiB;
        static constexpr uintptr_t HeapStart = 0xFFFFFE0000000000; // We start the heap at a high virtual address to avoid conflicts with other kernel mappings, and to give us plenty of room to grow the heap upwards.
    private:
        PMM *_pmm;
        Paging *_paging;
        Paging::PagingState* _pagingState;
        tlsf_t _tlsf;

        [[nodiscard]] STATUS Test();
    };

    // This is a C like heap allocator.
} // Memory

#endif //BOREALOS_HEAPALLOCATOR_H