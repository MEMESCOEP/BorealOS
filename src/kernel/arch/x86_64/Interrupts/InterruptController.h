#ifndef BOREALOS_INTERRUPTCONTROLLER_H
#define BOREALOS_INTERRUPTCONTROLLER_H

namespace Interrupts {
    class InterruptController {
    public:
        virtual void Initialize() = 0;
        virtual void SendEOI(uint8_t vector) = 0;
        virtual void MaskIRQ(uint8_t irqNum) = 0;
        virtual void UnmaskIRQ(uint8_t irqNum) = 0;
        virtual ~InterruptController() = default;
    };
}

#endif //BOREALOS_INTERRUPTCONTROLLER_H