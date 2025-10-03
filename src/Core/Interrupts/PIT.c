#include "PIT.h"

#include <Core/Interrupts/IDT.h>
#include <Utility/SerialOperations.h>

PITConfig KernelPITConfig = {
    .Frequency = 0,
    .Divisor = 0,
    .IsInitialized = false
};

PITState KernelPIT = {
    .Milliseconds = 0,
    .Ticks = 0,
    .TickTracker = 0,
    .IsInitialized = false
};

void pit_interrupt(uint8_t irqNumber, RegisterState* state) {
    (void)irqNumber, (void)state;
    if (!KernelPIT.IsInitialized || !KernelPITConfig.IsInitialized) {
        return; // PIT not initialized, ignore the interrupt
    }
    KernelPIT.Ticks++;
    KernelPIT.TickTracker++;

    if (KernelPIT.TickTracker >= KernelPITConfig.Frequency / 1000) {
        KernelPIT.TickTracker = 0;
        KernelPIT.Milliseconds++;
    }

    // The interrupt is automatically acknowledged by the PIC, so we don't need to do anything here
}

Status PITInit(uint32_t frequency) {
    KernelPITConfig.Frequency = frequency;
    KernelPITConfig.Divisor = PIT_FREQUENCY / frequency;
    if (KernelPITConfig.Divisor < PIT_DIVISOR_MIN || KernelPITConfig.Divisor > PIT_DIVISOR_MAX) {
        return STATUS_INVALID_PARAMETER;
    }

    KernelPITConfig.TimeIntervalMicroseconds = 1000000 / frequency;
    KernelPITConfig.IsInitialized = true;

    ASM ("cli"); // Disable interrupts while we set up

    // Set the PIT to mode 3 (square wave generator), binary mode, and access mode lobyte/hibyte
    outb(PIT_COMMAND_REGISTER, PIT_CHANNEL_0 | PIT_LOWBYTE_HIBYTE | PIT_MODE3 | PIT_BINARY);
    io_wait();
    outb(PIT_CHANNEL_0, (uint8_t)(KernelPITConfig.Divisor & 0xFF)); // Send the low byte of the divisor
    io_wait();
    outb(PIT_CHANNEL_0, (uint8_t)((KernelPITConfig.Divisor >> 8) & 0xFF)); // Send the high byte of the divisor
    io_wait();
    IDTSetIRQHandler(PIT_IRQ, pit_interrupt);
    PICClearIRQMask(PIT_IRQ); // Unmask the PIT IRQ line

    KernelPIT.IsInitialized = true;

    ASM ("sti"); // Re-enable interrupts
    return STATUS_SUCCESS;
}

void PITBusyWaitMicroseconds(uint32_t microseconds) {
    uint64_t start = KernelPIT.Ticks;
    while ((KernelPIT.Ticks - start) * (1000000 / KernelPITConfig.Frequency) < microseconds) {
        ASM("hlt");
    }
}
