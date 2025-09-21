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

typedef void (*ExceptionHandlerFn)(uint32_t exceptionNumber);

typedef struct IDTState {
    IDTDescriptor Descriptor;
    IDTEntry Entries[256];
    bool VectorSet[256];
    bool IRQSet[16];
    ExceptionHandlerFn ExceptionHandlers[16]; // Handlers for CPU exceptions
} IDTState;

extern IDTState KernelIDT;
extern const char* IDTExceptionStrings[];

/// Initialise the IDT, set up default handlers, and load it into the CPU
Status IDTInit();

/// Set an entry in the IDT
void IDTSetEntry(uint8_t vector, void *isr, uint8_t flags);

/// Set a handler for a specific CPU exception
void IDTSetExceptionHandler(uint8_t exceptionNumber, ExceptionHandlerFn handler);

#endif //BOREALOS_IDT_H