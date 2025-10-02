#ifndef BOREALOS_IDT_H
#define BOREALOS_IDT_H

#include <Definitions.h>

#include "Drivers/IO/PIC.h"

/*
push    eax
push    ebx
push    ecx
push    edx
push    esi
push    edi
push    ebp
*/
typedef struct RegisterState {
    uint32_t CR3, CR2, EBP, EDI, ESI, EDX, ECX, EBX, EAX;
} PACKED RegisterState;

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

typedef void (*ExceptionHandlerFn)(uint32_t exceptionNumber, RegisterState* state);
typedef void (*IRQHandlerFn)(uint8_t irqNumber, RegisterState* state);

typedef struct IDTState {
    IDTDescriptor Descriptor;
    IDTEntry Entries[256];
    bool VectorSet[256];
    bool IRQSet[16];
    ExceptionHandlerFn ExceptionHandlers[32]; // Handlers for CPU exceptions
    IRQHandlerFn IRQHandlers[16]; // Handlers for IRQs
} IDTState;

extern IDTState KernelIDT;
extern const char* IDTExceptionStrings[];

/// Initialise the IDT, set up default handlers, and load it into the CPU
Status IDTInit();

/// Set an entry in the IDT
void IDTSetEntry(uint8_t vector, void *isr, uint8_t flags);

/// Set a handler for a specific CPU exception
void IDTSetExceptionHandler(uint8_t exceptionNumber, ExceptionHandlerFn handler);

/// Set a handler for a specific IRQ
/// You must still enable the IRQ line in the PIC for it to be received
/// This function must be called with the IRQ number (0-15), not the vector number (32-47)
void IDTSetIRQHandler(uint8_t irqNumber, IRQHandlerFn handler);

#endif //BOREALOS_IDT_H