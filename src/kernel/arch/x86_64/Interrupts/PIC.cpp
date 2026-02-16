#include "PIC.h"

#include "../IO/Serial.h"

namespace Interrupts {
    PIC::PIC(uint8_t masterOffset, uint8_t slaveOffset) {
        this->_masterOffset = masterOffset;
        this->_slaveOffset = slaveOffset;
    }

    void PIC::Initialize() {
        // Initialise the PICs
        IO::Serial::outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
        IO::Serial::IOWait();

        IO::Serial::outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
        IO::Serial::IOWait();

        // Set vector offsets
        IO::Serial::outb(PIC1_DATA, this->_masterOffset);
        IO::Serial::IOWait();
        IO::Serial::outb(PIC2_DATA, this->_slaveOffset);
        IO::Serial::IOWait();

        // Tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
        IO::Serial::outb(PIC1_DATA, 4);
        IO::Serial::IOWait();

        // Tell Slave PIC its cascade identity (0000 0010)
        IO::Serial::outb(PIC2_DATA, 2);
        IO::Serial::IOWait();

        // Set both PICs to 8086/88 mode
        IO::Serial::outb(PIC1_DATA, ICW4_8086);
        IO::Serial::IOWait();
        IO::Serial::outb(PIC2_DATA, ICW4_8086);
        IO::Serial::IOWait();

        // Mask all IRQs initially
        IO::Serial::outb(PIC1_DATA, 0xFF);
        IO::Serial::IOWait();
        IO::Serial::outb(PIC2_DATA, 0xFF);
        IO::Serial::IOWait();

        PIC::ClearIRQMask(2); // Unmask the cascade IRQ line on the master PIC to allow communication with the slave PIC
    }

    void PIC::Disable() {
        // Mask all IRQs to disable the PICs
        IO::Serial::outb(PIC1_DATA, 0xFF);
        IO::Serial::IOWait();
        IO::Serial::outb(PIC2_DATA, 0xFF);
        IO::Serial::IOWait();
    }

    void PIC::SetIRQMask(uint8_t irq_line) {
        uint16_t port;
        uint8_t mask;

        if (irq_line < 8) {
            port = PIC1_DATA; // Master PIC
        } else {
            port = PIC2_DATA; // Slave PIC
            irq_line -= 8; // Adjust for slave PIC
        }

        mask = IO::Serial::inb(port) | (1 << irq_line); // Set the specific IRQ bit
        IO::Serial::outb(port, mask); // Write the new mask back to the PIC
        IO::Serial::IOWait();
    }

    void PIC::ClearIRQMask(uint8_t irq) {
        uint16_t port;
        uint8_t mask;

        if (irq < 8) {
            port = PIC1_DATA; // Master PIC
        } else {
            port = PIC2_DATA; // Slave PIC
            irq -= 8; // Adjust for slave PIC
        }

        mask = IO::Serial::inb(port) & ~(1 << irq); // Clear the specific IRQ bit
        IO::Serial::outb(port, mask); // Write the new mask back to the PIC
        IO::Serial::IOWait();
    }

    void PIC::SendEOI(uint8_t irq) {
        if (irq >= 8) {
            IO::Serial::outb(PIC2_COMMAND, PIC_EOI); // Send EOI to slave PIC
        }
        IO::Serial::outb(PIC1_COMMAND, PIC_EOI); // Send EOI to master PIC
        IO::Serial::IOWait();
    }
} // Interrupts