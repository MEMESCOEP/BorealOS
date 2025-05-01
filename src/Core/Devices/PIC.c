/* LIBRARIES */
#include <stdbool.h>
#include "Core/Graphics/Terminal.h"
#include "Core/IO/RegisterIO.h"
#include "Kernel.h"
#include "PIC.h"


/* FUNCTIONS */
// Prototypes so GCC stops complaining
void RemapPIC(int Offset1, int Offset2);
void DisablePIC();

// ACTUAL functions
void InitPIC()
{
    // Remap the PIC to prevent conflicts with IRQs 0 to 7.
    TerminalDrawString("[INFO] >> Remapping PIC (0x20, 0x28)...\n\r");
    RemapPIC(0x20, 0x28);

    // Disable the PIC until handlers are set up. This prevents any interrupts from causing problems
    // if their handlers haven't been set up yet.
    TerminalDrawString("\n\r[INFO] >> Masking all PIC interrupts...\n\r");
    DisablePIC();
}

// Acknowledge an interrupt by sending PIC_EOI to the master PIC chip's command register. If the IRQ is > 8, PIC_EOI
// will also be sent to the slave PIC chip's command register.
void PICSendEOI(uint8_t IRQ)
{
	if(IRQ >= 8)
    {
        OutB(PIC2_COMMAND, PIC_EOI);
    }

	OutB(PIC1_COMMAND, PIC_EOI);
}

// Remap the PIC chips to prevent IRQ conflicts with 0-7.
void RemapPIC(int Offset1, int Offset2)
{
    // Disable interrupts while the PIC is being initialized, otherwise a stray interrupt could cause a CPU exception.
    TerminalDrawString("\tPIC INTERRUPTS DISABLE\n\r");
    __asm__ volatile ("cli");

    // Starts the initialization sequence (in cascade mode)
    TerminalDrawString("\tPIC MASTER ICW1_INIT | ICW1_ICW4\n\r");
	OutB(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
	IOWait();

    TerminalDrawString("\tPIC SLAVE ICW1_INIT | ICW1_ICW4\n\r");
	OutB(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	IOWait();

    // ICW2: Master PIC vector offset
    TerminalDrawString("\tPIC MASTER VECTOR OFFSET\n\r");
	OutB(PIC1_DATA, Offset1);
	IOWait();

    // ICW2: Slave PIC vector offset
    TerminalDrawString("\tPIC SLAVE VECTOR OFFSET\n\r");
	OutB(PIC2_DATA, Offset2);
	IOWait();

    // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    TerminalDrawString("\tPIC MASTER SLAVE IRQ2\n\r");
	OutB(PIC1_DATA, 4);
	IOWait();
    
    // ICW3: tell Slave PIC its cascade identity (0000 0010)
    TerminalDrawString("\tPIC SLAVE CASCADE IDENTITY\n\r");
	OutB(PIC2_DATA, 2);
	IOWait();
	
    // ICW4: have the PICs use 8086 mode (and not 8080 mode)
    TerminalDrawString("\tPIC MASTER 8086 MODE\n\r");
	OutB(PIC1_DATA, ICW4_8086);
	IOWait();

    TerminalDrawString("\tPIC SLAVE ICW4_8086\n\r");
	OutB(PIC2_DATA, ICW4_8086);
	IOWait();

	// Unmask both PICs. This is equivalent to enabling every IRQ.
    TerminalDrawString("\tPIC UNMASK ALL IRQs\n\r");
	OutB(PIC1_DATA, 0);
	OutB(PIC2_DATA, 0);

    // Reenable interrupts.
    TerminalDrawString("\tPIC INTERRUPTS ENABLE\n\r");
    __asm__ volatile ("sti");
}

// "Disable" the PIC chips by masking every IRQ.
void DisablePIC()
{
    OutB(PIC1_DATA, 0xFF);
    OutB(PIC2_DATA, 0xFF);
}

// Disable an IRQ by setting the mask for the specific IRQ.
void SetIRQMask(uint8_t IRQLine)
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

// Enable an IRQ by clearing the mask for the specific IRQ.
void ClearIRQMask(uint8_t IRQLine)
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

    Value = InB(Port) & ~(1 << IRQLine);
    OutB(Port, Value);        
}

static uint16_t PICGetIRQReg(int OCW3)
{
    /* OCW3 to PIC CMD to get the register values.  PIC2 is chained, and
     * represents IRQs 8-15.  PIC1 is IRQs 0-7, with 2 being the chain */
    OutB(PIC1_COMMAND, OCW3);
    OutB(PIC2_COMMAND, OCW3);
    return (InB(PIC2_COMMAND) << 8) | InB(PIC1_COMMAND);
}

// Returns the combined value of the cascaded PICs irq request register.
uint16_t PICGetIRR()
{
    return PICGetIRQReg(PIC_READ_IRR);
}

// Returns the combined value of the cascaded PICs in-service register.
uint16_t PICGetISR()
{
    return PICGetIRQReg(PIC_READ_ISR);
}