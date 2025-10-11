#include "IDT.h"

#include "Core/Kernel.h"

ALIGNED(16) extern void *isr_stub_table[];

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

IDTState KernelIDT = {};

void irq_handler(uint8_t irq, RegisterState *regs) {
    (void)regs; // Unused parameter
    if (KernelIDT.IRQSet[irq - 0x20] && KernelIDT.IRQHandlers[irq - 0x20]) {
        KernelIDT.IRQHandlers[irq - 0x20]((uint32_t)irq, regs);
    }
    PICSendEOI(irq);

    // It wasn't handled, that's fine, just don't do anything
}

void exception_handler(uint32_t err, RegisterState *regs) {
    (void)regs;
    LOGF(LOG_ERROR, "CPU exception occurred! '%s'\n", IDTExceptionStrings[err]);
    if (KernelIDT.ExceptionHandlers[err]) {
        KernelIDT.ExceptionHandlers[err](err, regs);
        return;
    }

    // It wasn't handled, panic
    // Print out all the registers
    PRINTF("\t* EAX: %p\n", (void*)regs->EAX);
    PRINTF("\t* EBX: %p\n", (void*)regs->EBX);
    PRINTF("\t* ECX: %p\n", (void*)regs->ECX);
    PRINTF("\t* EDX: %p\n", (void*)regs->EDX);
    PRINTF("\t* ESI: %p\n", (void*)regs->ESI);
    PRINTF("\t* EDI: %p\n", (void*)regs->EDI);
    PRINTF("\t* EBP: %p\n", (void*)regs->EBP);
    PRINTF("\t* CR2: %p\n", (void*)regs->CR2);
    PRINTF("\t* CR3: %p\n", (void*)regs->CR3);
    PRINTF("\t* Vector: %u\n\n", err);
    PANIC("Unhandled CPU Exception!\n");
}

void page_fault_handler(uint32_t vector, RegisterState* regs) {
    PRINT("Page fault occured. Panicking.\n");
    (void)vector, (void)regs;

    // Tell the paging system to log more info
    PagingLogPageFaultInfo(&Kernel.Paging, (void*)regs->CR2);

    PANIC("Page fault.");
}

static void TestingExceptionHandler(uint32_t exceptionNumber, RegisterState* regs) {
    (void)regs;
    LOGF(LOG_INFO, "Exception handled successfully in testing mode. Exception number: %d\n", exceptionNumber);
    PRINTF("\t* Error string: %s\n\n", IDTExceptionStrings[exceptionNumber]);
}

Status IDTInit() {
    KernelIDT.Descriptor.Base = (uint32_t)&KernelIDT.Entries[0]; // Set the base address of the IDT entries
    KernelIDT.Descriptor.Limit = sizeof(KernelIDT.Entries) - 1; // Set the limit of the IDT

    for (uint8_t vec = 0; vec < 48; vec++) {
        IDTSetEntry(vec, isr_stub_table[vec], 0x8E);
        KernelIDT.VectorSet[vec] = true;
    }

    for (uint8_t irq_handle = 0; irq_handle < 16; irq_handle++) {
        KernelIDT.IRQSet[irq_handle] = false;
        KernelIDT.ExceptionHandlers[irq_handle] = nullptr; // Initialize exception handlers to nullptr
    }

    ASM ("lidt %0" : : "m"(KernelIDT.Descriptor)); // Load IDT descriptor
    ASM ("sti");

    // Test the IDT by setting some exception handlers and triggering them
    LOG(LOG_INFO, "Testing IDT by triggering exceptions...\n");

    IDTSetExceptionHandler(0, TestingExceptionHandler); // Division By Zero
    IDTSetExceptionHandler(3, TestingExceptionHandler); // Breakpoint

    // Test out all the exceptions we set handlers for
    ASM ("int $0"); // Trigger Division By Zero
    ASM ("int $3"); // Trigger Breakpoint

    LOG(LOG_INFO, "IDT initialized and tested successfully.\n");

    KernelIDT.ExceptionHandlers[0] = nullptr; // Remove the testing handler
    KernelIDT.ExceptionHandlers[3] = nullptr; // Remove the testing handler
    IDTSetExceptionHandler(14, page_fault_handler); // Page Fault handler

    return STATUS_SUCCESS;
}

void IDTSetEntry(uint8_t vector, void *isr, uint8_t flags) {
    IDTEntry *entry = &KernelIDT.Entries[vector];

    entry->IsrLow = (uint32_t)isr & 0xFFFF;
    entry->KernelCodeSegment = 0x08;
    entry->Reserved = 0;
    entry->Flags = flags;
    entry->IsrHigh = (uint32_t)isr >> 16;
}

void IDTSetExceptionHandler(uint8_t exceptionNumber, ExceptionHandlerFn handler) {
    KernelIDT.ExceptionHandlers[exceptionNumber] = handler;
}

void IDTSetIRQHandler(uint8_t irqNumber, IRQHandlerFn handler) {
    KernelIDT.IRQSet[irqNumber] = true;
    KernelIDT.IRQHandlers[irqNumber] = handler;
}
