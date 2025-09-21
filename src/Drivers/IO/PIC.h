#ifndef BOREALOS_PIC_H
#define BOREALOS_PIC_H

#include <Definitions.h>

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)
#define PIC_EOI 0x20 // End of Interrupt command

#define ICW1_ICW4 0x01 // ICW4 (ICW1 bit 0)
#define ICW1_SINGLE 0x02 // Single PIC mode (ICW1 bit 1)
#define ICW1_INTERVAL4 0x04 // Call interval 4 (ICW1 bit 2)
#define ICW1_LEVEL 0x08 // Level triggered mode (ICW1 bit 3)
#define ICW1_INIT 0x10 // Initialization command (ICW1 bit 4)

#define ICW4_8086 0x01 // 8086/88 mode (ICW4 bit 0)
#define ICW4_AUTO 0x02 // Auto EOI mode
#define ICW4_BUF_MASTER 0x0C // Buffered mode,
#define ICW4_BUF_SLAVE 0x08 // Buffered mode, slave PIC
#define ICW4_SFNM 0x10 // Special fully nested mode

typedef struct {
    uint8_t MasterOffset; // Offset for the master PIC
    uint8_t SlaveOffset;  // Offset for the slave PIC
} PICConfig;

extern PICConfig KernelPIC;

/// Initialize the PIC with the given vector offsets for master and slave.
/// Usually 0x20 and 0x28 are used to avoid conflicts with CPU exceptions.
Status PICInit(uint8_t masterOffset, uint8_t slaveOffset);

/// Disable both PICs by masking all IRQs.
void PICDisable();

/// Mask (disable) a specific IRQ line (0-15).
void PICSetIRQMask(uint8_t irqLine);

/// Unmask (enable) a specific IRQ line (0-15).
void PICClearIRQMask(uint8_t irq);

/// Send an End of Interrupt (EOI) signal for the given IRQ.
void PICSendEOI(uint8_t irq);

#endif //BOREALOS_PIC_H