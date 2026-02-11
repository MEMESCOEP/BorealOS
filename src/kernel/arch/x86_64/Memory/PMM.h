#ifndef BOREALOS_PMM_H
#define BOREALOS_PMM_H

#include <Definitions.h>
#include "Boot/LimineDefinitions.h"

namespace Memory {
    class PMM {
    public:
        void Initialize();

    private:
        void ReserveRegion(void *startAddr, uint64_t size);
        void FreePages(uint64_t startAddr, uint32_t numPages);
        bool IsPageAllocated(uint64_t pageIndex);
        bool IsPageReserved(uint64_t pageIndex);
        uintptr_t AllocatePages(uint32_t numPages);
        STATUS TestPMM();

        limine_memmap_response* limineMemmapResponse;

        size_t frameCount;
        size_t usableFrames;
        size_t bitmapSize;

        uint8_t* allocatableBitmap;
        uint8_t* reservedBitmap;

        uint64_t higherHalfOffset;
        uint64_t bitmapBase;
    };
} // Memory

#endif //BOREALOS_PMM_H