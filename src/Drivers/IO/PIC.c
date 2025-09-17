#include "PIC.h"
#include "Utility/SerialOperations.h"

Status PICLoad(uint8_t masterOffset, uint8_t slaveOffset, PICState *out) {
    out->Initialized = false;
    out->MasterOffset = masterOffset;
    out->SlaveOffset = slaveOffset;

    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC1_DATA, masterOffset);
    io_wait();
    outb(PIC2_DATA, slaveOffset);
    io_wait();

    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, 0xFF); // Mask all interrupts on master PIC
    io_wait();
    outb(PIC2_DATA, 0xFF); // Mask all interrupts on slave PIC
    io_wait();

    out->Initialized = true;
    return STATUS_SUCCESS;
}

void PICDisable(PICState *state) {
    outb(PIC1_DATA, 0xFF); // Mask all interrupts by writing 0xFF to the data ports
    io_wait();
    outb(PIC2_DATA, 0xFF);
    io_wait();

    (void)state; // Unused, but keep the signature consistent
}

void PICSetIRQMask(PICState *state, uint8_t irqLine) {
    uint16_t port;
    uint8_t mask;

    if (irqLine < 8) {
        port = PIC1_DATA; // Master PIC
    }
    else {
        port = PIC2_DATA; // Slave PIC
        irqLine -= 8; // Adjust for slave PIC
    }

    mask = inb(port) | (1 << irqLine); // Set the specific IRQ bit
    outb(port, mask); // Write the new mask back to the PIC
    io_wait();

    (void)state;
}

void PICClearIRQMask(PICState *state, uint8_t irq) {
    uint16_t port;
    uint8_t mask;

    if (irq < 8) {
        port = PIC1_DATA; // Master PIC
    }
    else {
        port = PIC2_DATA; // Slave PIC
        irq -= 8; // Adjust for slave PIC
    }
    mask = inb(port) & ~(1 << irq); // Clear the specific IRQ bit
    outb(port, mask); // Write the new mask back to the PIC
    io_wait();

    (void)state;
}

void PICSendEOI(PICState *state, uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI); // Send EOI to slave PIC
    }
    outb(PIC1_COMMAND, PIC_EOI); // Send EOI to master PIC
    io_wait();

    (void)state;
}
