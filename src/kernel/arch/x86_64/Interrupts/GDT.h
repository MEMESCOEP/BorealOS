#ifndef BOREALOS_GDT_H
#define BOREALOS_GDT_H

#include <Definitions.h>

namespace Interrupts {
    class GDT {
    private:
        struct PACKED GDTEntry {
            uint16_t LimitLow;
            uint16_t BaseLow;
            uint8_t BaseMiddle;
            uint8_t Access;
            uint8_t Granularity;
            uint8_t BaseHigh;
        };

        struct PACKED GDTPointer {
            uint16_t Limit;
            uint64_t Base;
        };

        static uint64_t entries[5]; // Null, Code, Data, TSS
        static GDTPointer gdtp;
        static void SetEntry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity);
        static void SetTSSDescriptor(int index, uint64_t base, uint32_t limit);

    public:
        static void Initialize();
        static GDTPointer* GetGDTPointer();
    };
} // Interrupts

#endif //BOREALOS_GDT_H