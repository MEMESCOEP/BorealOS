#ifndef BOREALOS_KERNEL_H
#define BOREALOS_KERNEL_H

#include <Definitions.h>

#include "Drivers/IO/Serial.h"
#include "Memory/PhysicalMemoryManager.h"

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
} KernelState;

// These macros automatically include the file and line number in the log/panic messages
#define LOG(kernel, message) (kernel)->Log(kernel, __FILE__ ":" STR(__LINE__) " : " message)
#define PANIC(kernel, message) (kernel)->Panic(kernel, __FILE__ ":" STR(__LINE__) " : " message)

Status KernelLoad(uint32_t InfoPtr, KernelState *out);

#endif //BOREALOS_KERNEL_H