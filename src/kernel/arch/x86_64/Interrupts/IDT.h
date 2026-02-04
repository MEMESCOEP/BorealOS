#ifndef BOREALOS_IDT_H
#define BOREALOS_IDT_H

#include <Definitions.h>

#include "PIC.h"

namespace Interrupts {
    class IDT {
    public:
        struct PACKED IDTEntry {
            uint16_t OffsetLow;
            uint16_t Selector;
            uint8_t IST;
            uint8_t TypeAttribute;
            uint16_t OffsetMiddle;
            uint32_t OffsetHigh;
            uint32_t Zero;
        };

        struct PACKED IDTPointer {
            uint16_t Limit;
            uint64_t Base;
        };

        explicit IDT(PIC* pic);

        void Initialize();
        void IRQHandler(uint8_t irq);
        void HandleException(uint32_t exceptionVector, uint32_t errorCode);
    private:
        PIC* _pic;
        IDTPointer _idtPointer = {0, 0};
        IDTEntry _idtEntries[256] = {};
        void (*_exceptionHandlers[32])(void) = { nullptr };
        void SetIDTEntry(uint8_t vector, uint64_t isr, uint8_t flags);
        bool _isTesting = false;
    };
} // Interrupts

#endif //BOREALOS_IDT_H