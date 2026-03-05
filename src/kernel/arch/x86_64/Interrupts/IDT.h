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

#pragma pack(push, 1)
        struct Registers {
            uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp, r8, r9, r10, r11, r12, r13, r14, r15;
        };
#pragma pack(pop)

        explicit IDT(InterruptController* ic);

        void Initialize();
        void RegisterExceptionHandler(uint8_t exceptionVector, void (*handler)(void));
        void RegisterIRQHandler(uint8_t irq, void (*handler)(void));
        void IRQHandler(uint8_t irq, Registers *registers);
        void HandleException(uint32_t exceptionVector, uint32_t errorCode, Registers *registers) const;
        void UnmaskIRQ(uint8_t uint8) const;
        void MaskIRQ(uint8_t uint8) const;
        void SetInterruptController(InterruptController* ic);

    private:
        InterruptController* _ic;
        IDTPointer _idtPointer = {0, 0};
        void (*_exceptionHandlers[32])(void) = { nullptr };
        void (*_irqHandlers[256])(void) = { nullptr };
        void SetIDTEntry(uint8_t vector, uint64_t isr, uint8_t flags);
        bool _isTesting = false;
    };
} // Interrupts

#endif //BOREALOS_IDT_H