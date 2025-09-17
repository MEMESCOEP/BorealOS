#include "IDT.h"

#include "Core/Kernel.h"

ALIGNED(16) extern void *isr_stub_table[];
IDTState global_idt;

const char* IDTExceptionStrings[] = {
    "<FATAL_CPU_EXCEPTION_0> Division By Zero",
    "<FATAL_CPU_EXCEPTION_1> Debug",
    "<FATAL_CPU_EXCEPTION_2> Non Maskable Interrupt",
    "<FATAL_CPU_EXCEPTION_3> Breakpoint",
    "<FATAL_CPU_EXCEPTION_4> Detected Overflow",
    "<FATAL_CPU_EXCEPTION_5> Out of Bounds",
    "<FATAL_CPU_EXCEPTION_6> Invalid Opcode",
    "<FATAL_CPU_EXCEPTION_7> No Coprocessor",
    "<FATAL_CPU_EXCEPTION_8> Double Fault",
    "<FATAL_CPU_EXCEPTION_9> Coprocessor Segment Overrun",
    "<FATAL_CPU_EXCEPTION_10> Bad TSS",
    "<FATAL_CPU_EXCEPTION_11> Segment Not Present",
    "<FATAL_CPU_EXCEPTION_12> Stack Fault",
    "<FATAL_CPU_EXCEPTION_13> General Protection Fault",
    "<FATAL_CPU_EXCEPTION_14> Page Fault",
    "<FATAL_CPU_EXCEPTION_15> Unknown Interrupt",
    "<FATAL_CPU_EXCEPTION_16> Coprocessor Fault",
    "<FATAL_CPU_EXCEPTION_17> Alignment Check failure (486+)",
    "<FATAL_CPU_EXCEPTION_18> Machine Check failure (Pentium/586+)",
    "<FATAL_CPU_EXCEPTION_19> Reserved",
    "<FATAL_CPU_EXCEPTION_20> Reserved",
    "<FATAL_CPU_EXCEPTION_21> Reserved",
    "<FATAL_CPU_EXCEPTION_22> Reserved",
    "<FATAL_CPU_EXCEPTION_23> Reserved",
    "<FATAL_CPU_EXCEPTION_24> Reserved",
    "<FATAL_CPU_EXCEPTION_25> Reserved",
    "<FATAL_CPU_EXCEPTION_26> Reserved",
    "<FATAL_CPU_EXCEPTION_27> Reserved",
    "<FATAL_CPU_EXCEPTION_28> Reserved",
    "<FATAL_CPU_EXCEPTION_29> Reserved",
    "<FATAL_CPU_EXCEPTION_30> Reserved",
    "<FATAL_CPU_EXCEPTION_31> Reserved",
};

void irq_handler(uint8_t irq) {
    if (global_idt.Kernel) {
        PICSendEOI(&global_idt.Kernel->PIC, irq);
        if (global_idt.IRQSet[irq - 0x20] && global_idt.ExceptionHandlers[irq - 0x20]) {
            global_idt.ExceptionHandlers[irq - 0x20](global_idt.Kernel, (uint32_t)irq);
            return;
        }

        // It wasn't handled, panic
        PANIC(global_idt.Kernel, "Unhandled IRQ!\n");
    }

    ASM ("cli");
    while (true) {
        ASM ("hlt");
    }
}

void exception_handler(uint32_t err) {
    if (global_idt.Kernel) {
        global_idt.Kernel->Printf(global_idt.Kernel, "Exception occurred! '%s'\n", IDTExceptionStrings[err]);
        if (global_idt.ExceptionHandlers[err]) {
            global_idt.ExceptionHandlers[err](global_idt.Kernel, err);
            return;
        }

        // It wasn't handled, panic
        PANIC(global_idt.Kernel, "Unhandled CPU Exception!\n");
    }

    // TODO: Do some fancier stuff, for now just halt
    ASM ("cli");
    while (true) {
        ASM ("hlt");
    }
}

static void TestingExceptionHandler(KernelState* kernel, uint32_t exceptionNumber) {
    kernel->Printf(kernel, "Exception handled successfully in testing mode. Exception number: %d\n", exceptionNumber);
    kernel->Printf(kernel, "Error string: %s\n", IDTExceptionStrings[exceptionNumber]);
}

Status IDTLoad(KernelState* kernel, IDTState *out) {
    ASM ("cli"); // Disable interrupts while loading IDT
    out = &global_idt; // Use the global IDT state

    out->Kernel = kernel;

    out->Descriptor.Base = (uint32_t)&out->Entries[0]; // Set the base address of the IDT entries
    out->Descriptor.Limit = sizeof(out->Entries) - 1; // Set the limit of the IDT

    for (uint8_t vec = 0; vec < 48; vec++) {
        IDTSetEntry(out, vec, isr_stub_table[vec], 0x8E);
        out->VectorSet[vec] = true;
    }

    for (uint8_t irq_handle = 0; irq_handle < 16; irq_handle++) {
        out->IRQSet[irq_handle] = false;
        out->ExceptionHandlers[irq_handle] = nullptr; // Initialize exception handlers to nullptr
    }

    for (uint8_t exc = 0; exc < 16; exc++) {
        out->ExceptionHandlers[exc] = nullptr; // Initialize exception handlers to nullptr
    }

    ASM ("lidt %0" : : "m"(out->Descriptor)); // Load IDT descriptor
    ASM ("sti");

    // Test the IDT by setting some exception handlers and triggering them
    out->Kernel->Printf(out->Kernel, "Testing IDT by triggering exceptions...\n");

    IDTSetExceptionHandler(out, 0, TestingExceptionHandler); // Division By Zero
    IDTSetExceptionHandler(out, 3, TestingExceptionHandler); // Breakpoint

    // Test out all the exceptions we set handlers for
    ASM ("int $0"); // Trigger Division By Zero
    ASM ("int $3"); // Trigger Breakpoint

    out->Kernel->Printf(out->Kernel, "IDT initialized and tested successfully.\n");

    out->ExceptionHandlers[0] = nullptr; // Remove the testing handler
    out->ExceptionHandlers[3] = nullptr; // Remove the testing handler

    return STATUS_SUCCESS;
}

void IDTSetEntry(IDTState *state, uint8_t vector, void *isr, uint8_t flags) {
    IDTEntry *entry = &state->Entries[vector];

    entry->IsrLow = (uint32_t)isr & 0xFFFF;
    entry->KernelCodeSegment = 0x08;
    entry->Reserved = 0;
    entry->Flags = flags;
    entry->IsrHigh = (uint32_t)isr >> 16;
}

void IDTSetExceptionHandler(IDTState *state, uint8_t exceptionNumber, ExceptionHandlerFn handler) {
    state->ExceptionHandlers[exceptionNumber] = handler;
}
