#ifndef BOREALOS_KERNEL_H
#define BOREALOS_KERNEL_H

#include <Definitions.h>

#include "Drivers/IO/Serial.h"
#include "Memory/PhysicalMemoryManager.h"
#include "Drivers/IO/PIC.h"
#include "Interrupts/IDT.h"

typedef struct KernelState KernelState;
typedef void (*LogFn)(const KernelState*, const char*);
typedef NORETURN void (*PanicFn)(const KernelState*, const char*);
typedef void (*PrintfFn)(const KernelState*, const char*, ...);

typedef struct KernelState {
    // Output functions
    LogFn Log;
    PanicFn Panic;
    PrintfFn Printf;

    // Subsystem states
    SerialPort Serial;
    PhysicalMemoryManagerState PhysicalMemoryManager;
    PICState PIC;
    IDTState *IDT;
} KernelState;

Status KernelLoad(uint32_t InfoPtr, KernelState *out);

#endif //BOREALOS_KERNEL_H