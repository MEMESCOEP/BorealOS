#ifndef BOREALOS_PIC_H
#define BOREALOS_PIC_H

#include <Definitions.h>

namespace Interrupts {
    class PIC {
    public:
        static constexpr int PIC1 = 0x20;
        static constexpr int PIC2 = 0xA0;
        static constexpr int PIC1_COMMAND = PIC1;
        static constexpr int PIC1_DATA = PIC1 + 1;
        static constexpr int PIC2_COMMAND = PIC2;
        static constexpr int PIC2_DATA = PIC2 + 1;
        static constexpr int PIC_EOI = 0x20; // End of Interrupt command

        static constexpr int ICW1_ICW4 = 0x01; // ICW4 (ICW1 bit 0)
        static constexpr int ICW1_SINGLE = 0x02; // Single PIC mode (ICW1 bit 1)
        static constexpr int ICW1_INTERVAL4 = 0x04; // Call interval 4 (ICW1 bit 2)
        static constexpr int ICW1_LEVEL = 0x08; // Level triggered mode (ICW1 bit 3)
        static constexpr int ICW1_INIT = 0x10; // Initialization command (ICW1 bit 4)

        static constexpr int ICW4_8086 = 0x01; // 8086/88 mode (ICW4 bit 0)
        static constexpr int ICW4_AUTO = 0x02; // Auto EOI mode
        static constexpr int ICW4_BUF_MASTER = 0x0C; // Buffered mode,
        static constexpr int ICW4_BUF_SLAVE = 0x08; // Buffered mode, slave PIC
        static constexpr int ICW4_SFNM = 0x10; // Special fully nested mode

        explicit PIC(uint8_t masterOffset, uint8_t slaveOffset);

        void Initialize();
        void Disable();
        void SetIRQMask(uint8_t irq_line);
        void ClearIRQMask(uint8_t irq);
        void SendEOI(uint8_t irq);
    private:
        uint8_t _masterOffset;
        uint8_t _slaveOffset;
    };
} // Interrupts

#endif //BOREALOS_PIC_H