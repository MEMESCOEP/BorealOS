#include "PIC.h"
#include "Utility/SerialOperations.h"

PICConfig KernelPIC = {};

Status PICInit(uint8_t masterOffset, uint8_t slaveOffset) {
    KernelPIC.MasterOffset = masterOffset;
    KernelPIC.SlaveOffset = slaveOffset;

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

    return STATUS_SUCCESS;
}

void PICDisable() {
    outb(PIC1_DATA, 0xFF); // Mask all interrupts by writing 0xFF to the data ports
    io_wait();
    outb(PIC2_DATA, 0xFF);
    io_wait();
}

void PICSetIRQMask(uint8_t irqLine) {
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
}

void PICClearIRQMask(uint8_t irq) {
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
}

void PICSendEOI(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI); // Send EOI to slave PIC
    }
    outb(PIC1_COMMAND, PIC_EOI); // Send EOI to master PIC
    io_wait();
}
