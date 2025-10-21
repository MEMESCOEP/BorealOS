#ifndef BOREALOS_KERNEL_H
#define BOREALOS_KERNEL_H

#include <Definitions.h>
#include <Drivers/IO/Disk/Block.h>
#include <Drivers/IO/FS/VFS.h>

#include "Drivers/IO/Serial.h"
#include "Memory/PhysicalMemoryManager.h"
#include "Drivers/IO/PIC.h"
#include "Interrupts/IDT.h"
#include "Memory/HeapAllocator.h"
#include "Memory/Paging.h"
#include "Memory/VirtualMemoryManager.h"

typedef void (*LogFn)(int, const char*, ...);
typedef NORETURN void (*PanicFn)(const char*);
typedef void (*PrintfFn)(const char*, ...);

typedef struct KernelState {
    // Output functions
    LogFn Log;
    PanicFn Panic;
    PrintfFn Printf;

    // Subsystem states
    SerialPort Serial;
    PagingState Paging; // The root kernel paging structure
    VirtualMemoryManagerState VMM; // The root kernel virtual memory manager
    HeapAllocatorState Heap; // The kernel heap allocator
    VFSState VFS; // The kernel virtual file system
} KernelState;

// The global kernel state.
extern KernelState Kernel;

/// Initialize the kernel, and its subsystems.
Status KernelInit(uint32_t InfoPtr);

#endif //BOREALOS_KERNEL_H