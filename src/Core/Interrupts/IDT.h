#ifndef BOREALOS_IDT_H
#define BOREALOS_IDT_H

#include <Definitions.h>

#include "Drivers/IO/PIC.h"

typedef struct {
    uint16_t IsrLow;
    uint16_t KernelCodeSegment;
    uint8_t Reserved;
    uint8_t Flags;
    uint16_t IsrHigh;
} PACKED IDTEntry;

typedef struct {
    uint16_t Limit;
    uint32_t Base;
} PACKED IDTDescriptor;

typedef struct KernelState KernelState; // Forward declaration to avoid circular dependency
typedef void (*ExceptionHandlerFn)(struct KernelState*, uint32_t exceptionNumber);

typedef struct IDTState {
    IDTDescriptor Descriptor;
    IDTEntry Entries[256];
    bool VectorSet[256];
    bool IRQSet[16];
    ExceptionHandlerFn ExceptionHandlers[16]; // Handlers for CPU exceptions
    KernelState *Kernel; // Pointer to the kernel state for use in handlers
} IDTState;

extern const char* IDTExceptionStrings[];

Status IDTLoad(KernelState* kernel, IDTState **out);
void IDTSetEntry(IDTState *state, uint8_t vector, void *isr, uint8_t flags);
void IDTSetExceptionHandler(IDTState *state, uint8_t exceptionNumber, ExceptionHandlerFn handler);

#endif //BOREALOS_IDT_H