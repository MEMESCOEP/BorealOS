#include <Core/Graphics/Console.h>
#include <Core/IO/PIC.h>

// Remap the PIC chips to avoid interfering with CPU exceptions 0x00-0x1F
void PICInit(uint8_t masterOffset, uint8_t slaveOffset)
{
    LOG_KERNEL_MSG("\tNotifying master and slave PICs...\n\r", NONE);
    OutB(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    IoDelay();
    OutB(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
    IoDelay();

    LOG_KERNEL_MSG("\tSending new interrupt vector offsets to PICs...\n\r", NONE);
    OutB(PIC1_DATA, masterOffset);
    IoDelay();
    OutB(PIC2_DATA, slaveOffset);
    IoDelay();

    LOG_KERNEL_MSG("\tNotifying master of slave PIC's existence...\n\r", NONE);
    OutB(PIC1_DATA, 4); // Tell master PIC that there is a slave PIC at IRQ2
    IoDelay();

    LOG_KERNEL_MSG("\tSending slave PIC its cascade ID...\n\r", NONE);
    OutB(PIC2_DATA, 2); // Tell slave PIC its cascade identity
    IoDelay();

    LOG_KERNEL_MSG("\tSetting master and slave PICs to 8086 mode...\n\r", NONE);
    OutB(PIC1_DATA, ICW4_8086);
    IoDelay();
    OutB(PIC2_DATA, ICW4_8086);
    IoDelay();

    LOG_KERNEL_MSG("\tMasking all IRQs of master of slave PIC's...\n\r", NONE);
    OutB(PIC1_DATA, 0xFF); // Mask all IRQs on master PIC
    OutB(PIC2_DATA, 0xFF); // Mask all IRQs on slave PIC

    LOG_KERNEL_MSG("\tPIC remapping finished.\n\n\r", NONE);
}

// "Disable" the PICs by mmasking all of their interrupts
void PICDisable(void)
{
    OutB(PIC1_DATA, 0xFF); // Mask all IRQs on master PIC
    OutB(PIC2_DATA, 0xFF); // Mask all IRQs on slave PIC
}

// Enable an IRQ by clearing the mask for the specific IRQ
void PICClearIRQMask(uint8_t irq)
{
    uint16_t port;
    uint8_t mask;

    if (irq < 8)
    {
        port = PIC1_DATA;
    }
    else
    {
        port = PIC2_DATA;
        irq -= 8;
    }

    mask = InB(port) & ~(1 << irq);
    OutB(port, mask);
}

// Disable an IRQ by setting the mask for the specific IRQ.
void PICSetIRQMask(uint8_t IRQLine)
{
    uint16_t Port;
    uint8_t Value;

    if(IRQLine < 8)
    {
        Port = PIC1_DATA;
    }
    else
    {
        Port = PIC2_DATA;
        IRQLine -= 8;
    }

    Value = InB(Port) | (1 << IRQLine);
    OutB(Port, Value);        
}

// Let the PIC know that we've handled the interrupt
void PICSendEOI(uint8_t irq)
{
    if (irq >= 8)
    {
        OutB(PIC2_CMD, PIC_EOI);
    }
    
    OutB(PIC1_CMD, PIC_EOI);
}