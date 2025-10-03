#ifndef BOREALOS_PIT_H
#define BOREALOS_PIT_H

#include <Definitions.h>

// This header is for the Programmable Interval Timer (PIT), which is used for system timing and generating periodic interrupts.
// This can be used for high precision tasks, unlike the RTC which is only for keeping track of time.

#define PIT_IRQ 0

#define PIT_CHANNEL_0 0x40 // Read/Write
#define PIT_CHANNEL_1 0x41 // Read/Write
#define PIT_CHANNEL_2 0x42 // Read/Write
#define PIT_COMMAND_REGISTER 0x43 // Write-only

#define PIT_LOWBYTE_HIBYTE 0x30 // Access mode: lobyte/hibyte
#define PIT_MODE0 0x00 // Mode 0: Interrupt on terminal count
#define PIT_MODE1 0x02 // Mode 1: Hardware re-triggerable one-shot
#define PIT_MODE2 0x04 // Mode 2: Rate generator
#define PIT_MODE3 0x06 // Mode 3: Square wave generator
#define PIT_MODE4 0x08 // Mode 4: Software triggered strobe
#define PIT_MODE5 0x0A // Mode 5: Hardware triggered strobe
#define PIT_BINARY 0x00 // Binary mode
#define PIT_BCD 0x01 // BCD mode (we won't use this)

#define PIT_FREQUENCY 1193180 // The PIT runs at 1.193180 MHz

// The least amount of time we can set the PIT to is approximately 0.838 microseconds (1 / 1,193,180 Hz)
// The maximum amount of time we can set the PIT to is approximately 54.925 seconds (65536 / 1,193,180 Hz)
#define PIT_DIVISOR_MIN 1
#define PIT_DIVISOR_MAX 65536

#define PIT_SECONDS_TO_MICROSECONDS(x) ((x) * 1000000)
#define PIT_MILLISECONDS_TO_MICROSECONDS(x) ((x) * 1000)
#define PIT_MICROSECONDS_TO_SECONDS(x) ((x) / 1000000)
#define PIT_MICROSECONDS_TO_MILLISECONDS(x) ((x) / 1000)

typedef struct PITConfig {
    uint32_t Frequency; // Frequency in Hz
    uint32_t TimeIntervalMicroseconds; // Time interval in microseconds
    uint32_t Divisor; // Divisor value for the PIT
    bool IsInitialized;
} PITConfig;

typedef struct PITState {
    uint64_t Milliseconds; // Number of milliseconds since the PIT was initialized
    uint64_t Ticks; // Number of ticks since the PIT was initialized
    uint32_t TickTracker; // Number of ticks since the last millisecond increment
    bool IsInitialized;
} PITState;

extern PITConfig KernelPITConfig;
extern PITState KernelPIT;

Status PITInit(uint32_t frequency);
void PITBusyWaitMicroseconds(uint32_t microseconds);

#endif //BOREALOS_PIT_H