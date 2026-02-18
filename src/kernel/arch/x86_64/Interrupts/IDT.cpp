#include "IDT.h"

#include "Kernel.h"
#include "../KernelData.h"

static const char* ExceptionNames[] = {
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

extern "C" {
    extern void* ISRStubTable[];
}

namespace Interrupts {
    IDT::IDT(PIC *pic) {
        this->_pic = pic;
    }

    static IDT::IDTEntry _idtEntries[256] ALIGNED(16) = {}; // Static allocation for 256 IDT entries

    void IDT::Initialize() {
        asm volatile("cli"); // Disable interrupts while loading IDT

        _idtPointer.Base = reinterpret_cast<uint64_t>(&_idtEntries[0]);
        _idtPointer.Limit = sizeof(IDTEntry) * 256 - 1;

        for (uint8_t vec = 0; vec < 48; vec++) {
            SetIDTEntry(vec, reinterpret_cast<uint64_t>(((void**) &ISRStubTable)[vec]), 0x8E); // 0x8E = Present, DPL=0, Type=Interrupt Gate
        }

        // Override the stack for DF, GPF and PF to use the IST entry 1.
        _idtEntries[8].IST = 1; // Double Fault
        _idtEntries[13].IST = 1; // General Protection Fault
        _idtEntries[14].IST = 1; // Page Fault

        for (auto & _exceptionHandler : _exceptionHandlers) {
            _exceptionHandler = nullptr; // Initialize exception handlers to nullptr
        }

        for (auto & _irqHandler : _irqHandlers) {
            _irqHandler = nullptr; // Initialize IRQ handlers to nullptr
        }

        asm volatile("lidt %0" : : "m" (_idtPointer)); // Load the IDT
        asm volatile("sti"); // Enable interrupts after loading IDT

        _isTesting = true;
        asm volatile("int $0"); // Trigger Division By Zero
        asm volatile("int $3"); // Trigger Breakpoint
        _isTesting = false;
    }

    void IDT::RegisterExceptionHandler(uint8_t exceptionVector, void(*handler)()) {
        if (exceptionVector >= 32) {
            LOG_ERROR("Attempted to register an exception handler for vector %u8, which is not a CPU exception vector!", exceptionVector);
            return;
        }

        _exceptionHandlers[exceptionVector] = handler;
    }

    void IDT::RegisterIRQHandler(uint8_t irq, void(*handler)()) {
        if (irq >= 16) {
            LOG_ERROR("Attempted to register an IRQ handler for IRQ %u8, which is out of bounds!", irq);
            return;
        }

        _irqHandlers[irq] = handler;
    }

    void IDT::IRQHandler(uint8_t irq, Registers *registers) {
        if (irq < 16 && _irqHandlers[irq] != nullptr) {
            _irqHandlers[irq]();
        }

        _pic->SendEOI(irq);
    }

    void IDT::HandleException(uint32_t exceptionVector, uint32_t errorCode, Registers *registers) const {
        if (_isTesting) {
            LOG_INFO("IDT testing mode: Exception %s occurred with error code %u32",
                     (exceptionVector < 32) ? ExceptionNames[exceptionVector] : "Unknown", errorCode);
            return; // In testing mode, just return
        }

        LOG_ERROR("\n\r\n\r-- CPU Exception Occurred --\n\r");

        LOG_ERROR("CPU Exception: %s occurred with error code %u32",
                  (exceptionVector < 32) ? ExceptionNames[exceptionVector] : "Unknown", errorCode);

        if (exceptionVector == 14) {
            uint64_t cr2;
            asm volatile("mov %%cr2, %0" : "=r" (cr2));
            LOG_ERROR("Page fault at address: 0x%x64", cr2);
        }

        LOG_ERROR("Register state at time of exception:");
        LOG_ERROR("RAX: 0x%x64 RBX: 0x%x64 RCX: 0x%x64 RDX: 0x%x64", registers->rax, registers->rbx, registers->rcx, registers->rdx);
        LOG_ERROR("RSI: 0x%x64 RDI: 0x%x64 RBP: 0x%x64 R8: 0x%x64", registers->rsi, registers->rdi, registers->rbp, registers->r8);
        LOG_ERROR("R9: 0x%x64 R10: 0x%x64 R11: 0x%x64 R12: 0x%x64", registers->r9, registers->r10, registers->r11, registers->r12);
        LOG_ERROR("R13: 0x%x64 R14: 0x%x64 R15: 0x%x64", registers->r13, registers->r14, registers->r15);

        PANIC("Exception occurred");
    }

    void IDT::ClearIRQMask(uint8_t uint8) const {
        if (uint8 >= 16) {
            LOG_ERROR("Attempted to clear mask for IRQ %u8, which is out of bounds!", uint8);
            return;
        }

        _pic->ClearIRQMask(uint8);
    }

    void IDT::SetIDTEntry(uint8_t vector, uint64_t isr, uint8_t flags) {
        IDTEntry &entry = _idtEntries[vector];
        entry.OffsetLow = isr & 0xFFFF;
        entry.Selector = 0x08; // Kernel code segment
        entry.IST = 0; // No IST
        entry.TypeAttribute = flags;
        entry.OffsetMiddle = (isr >> 16) & 0xFFFF;
        entry.OffsetHigh = (isr >> 32) & 0xFFFFFFFF;
        entry.Zero = 0;
    }
} // Interrupts

extern "C" {
    void IRQHandler(uint8_t irq, Interrupts::IDT::Registers* regs) {
        Kernel<KernelData>::GetInstance()->ArchitectureData->Idt.IRQHandler(irq - 32, regs);
    }

    // Error code is the vector number, so for example, divide by zero is 0, page fault is 14, etc.
    // The error number is the error code pushed by the CPU for exceptions that push an error code.
    // So for vector 8, that would be 0 for double fault, for vector 14, that would be the page fault error code, etc.
    void ExceptionHandler(uint64_t exceptionVector, uint64_t errorCode, Interrupts::IDT::Registers* regs) {
        Kernel<KernelData>::GetInstance()->ArchitectureData->Idt.HandleException(exceptionVector, errorCode, regs);
    }
}