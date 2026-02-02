#ifndef BOREALOS_GDT_H
#define BOREALOS_GDT_H

#include <Definitions.h>

namespace Interrupts {
    class GDT {
    private:
        struct GDTPointer {
            uint16_t limit;
            uint64_t base;
        } __attribute__((packed));

        static uint64_t entries[3];
        static GDTPointer gdtp;
    public:
        static void initialize();
    };
} // Interrupts

#endif //BOREALOS_GDT_H