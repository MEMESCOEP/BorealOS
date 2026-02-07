#include <Definitions.h>
#include <Kernel.h>
#include <stdarg.h>

#include "KernelData.h"
#include "Interrupts/GDT.h"
#include "Interrupts/TSS.h"
#include "IO/Serial.h"
#include "IO/SerialPort.h"
#include "Utility/StringFormatter.h"
#include "Memory/PMM.h"

Kernel<KernelData> kernel;
KernelData kernelData;

template<typename T>
Kernel<T> *Kernel<T>::GetInstance() {
    return &kernel;
}

template<typename T>
void Kernel<T>::Initialize() {
    ArchitectureData = &kernelData;

    // Serial:
    ArchitectureData->SerialPort = IO::SerialPort(IO::Serial::COM1);
    ArchitectureData->SerialPort.Initialize();
    ArchitectureData->SerialPort.WriteString("\n\n");
    LOG(LOG_LEVEL::INFO, "Loaded serial port. %p", IO::Serial::COM1);

    // Tss & Gdt:
    Interrupts::GDT::Initialize(); // This loads the GDT and the TSS into the GDT
    LOG(LOG_LEVEL::INFO, "Initialized GDT & TSS. (GDT at %p, TSS at %p)",
        Interrupts::GDT::GetGDTPointer(),
        Interrupts::TSS::GetTSSStruct());

    // PIC:
    ArchitectureData->Pic = Interrupts::PIC(0x20, 0x28);
    ArchitectureData->Pic.Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized PIC.");

    // IDT:
    ArchitectureData->Idt = Interrupts::IDT(&ArchitectureData->Pic);
    ArchitectureData->Idt.Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized IDT.");

    // Physical Memory Manager:
    ArchitectureData->Pmm.Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized PMM.");
}

template<typename T>
void Kernel<T>::Start() {
    uint8_t t = 'A';
    uint16_t u16 = 24;
    int32_t i32 = -123456;
    uint64_t u64 = 1234567890123456;
    LOG(LOG_LEVEL::INFO, "Kernel started! Testing logging:");
    LOG(LOG_LEVEL::DEBUG, "Char: %c, UInt16: %u16, Int32: %i32, UInt64: %u64", t, u16, i32, u64);

    PANIC("Kernel::Start not implemented");
}

template<typename T>
void Kernel<T>::Log(const char *message) {
    ArchitectureData->SerialPort.WriteString(message);
}

template<typename T>
[[noreturn]] void Kernel<T>::Panic(const char *message) {
    Log(message);

    while (true) {
        asm ("hlt");
    }
}

void Core::Write(const char *message) {
    Kernel<KernelData>::GetInstance()->Log(message);
}

void Core::Log(LOG_LEVEL level, const char *fmt, ...) {
    switch (level) {
        case LOG_LEVEL::INFO:
            Kernel<KernelData>::GetInstance()->Log("[INFO] ");
            break;
        case LOG_LEVEL::WARNING:
            Kernel<KernelData>::GetInstance()->Log("[WARNING] ");
            break;
        case LOG_LEVEL::ERROR:
            Kernel<KernelData>::GetInstance()->Log("[ERROR] ");
            break;
        case LOG_LEVEL::DEBUG:
            Kernel<KernelData>::GetInstance()->Log("[DEBUG] ");
            break;
    }

    // Format the message
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    auto len = Utility::StringFormatter::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    bool truncated = false;
    if (len > 1024) {
        len = 1024;
        truncated = true;
    }
    buffer[len] = '\0';

    Kernel<KernelData>::GetInstance()->Log(buffer);
    if (truncated) {
        Kernel<KernelData>::GetInstance()->Log("...[TRUNCATED]");
    }
}

[[noreturn]] void Core::Panic(const char *message) {
    Kernel<KernelData>::GetInstance()->Panic(message);
}

template class Kernel<KernelData>; // Initialize the template class