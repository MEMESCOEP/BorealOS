#include <Definitions.h>
#include <Kernel.h>
#include <stdarg.h>

#include "KernelData.h"
#include "Interrupts/GDT.h"
#include "Interrupts/TSS.h"
#include "IO/Serial.h"
#include "IO/SerialPort.h"
#include "IO/FramebufferConsole.h"
#include "Utility/StringFormatter.h"
#include "Utility/ANSI.h"
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
    LOG(LOG_LEVEL::INFO, "Loaded serial port COM1 (%p).", IO::Serial::COM1);

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

    // Console:
    ArchitectureData->Console.Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized framebuffer console.");

    // RTC:
    ArchitectureData->Rtc = Core::Time::RTC(&ArchitectureData->Idt);
    ArchitectureData->Rtc.Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized RTC.");

    // Physical Memory Manager:
    ArchitectureData->Pmm.Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized PMM.");

    // CPU:
    ArchitectureData->Cpu.Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized CPU.");

    // Paging:
    ArchitectureData->Paging = Memory::Paging(&ArchitectureData->Pmm);
    ArchitectureData->Paging.Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized paging.");

    // Heap:
    ArchitectureData->HeapAllocator = Memory::HeapAllocator(&ArchitectureData->Pmm, &ArchitectureData->Paging, ArchitectureData->Paging.GetKernelPagingState());
    ArchitectureData->HeapAllocator.Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized heap allocator.");

    // ACPI:
    ArchitectureData->Acpi.Initialize();
    LOG(LOG_LEVEL::INFO, "Initialized ACPI.");
  
    // Init ram fs:
    auto files = module_request.response->modules;
    if (!files || module_request.response->module_count == 0) {
        PANIC("Limine did not provide any modules, but we need at least one for the init ram filesystem!");
    }

    auto cpioArchive = files[0];
    ArchitectureData->InitRamFS = FileSystems::InitRamFileSystem(cpioArchive, &ArchitectureData->HeapAllocator);
    LOG_INFO("Initialized initramfs.");
}

template<typename T>
void Kernel<T>::Start() {
    PANIC("Kernel::Start not implemented");
}

template<typename T>
void Kernel<T>::Log(const char *message) {
    ArchitectureData->SerialPort.WriteString(message);
}

template<typename T>
[[noreturn]] void Kernel<T>::Panic(const char *message) {
    Log("[PANIC] ");
    Log(message);
    kernelData.Console.PrintString("[");
    kernelData.Console.PrintString(ANSI::Colors::Foreground::Red);
    kernelData.Console.PrintString(ANSI::EscapeCodes::TextDim);
    kernelData.Console.PrintString("PANIC\033[0m] ");
    kernelData.Console.PrintString(message);
    kernelData.Console.PrintString("\n\r");

    while (true) {
        asm ("hlt");
    }
}

void Core::Write(const char *message) {
    Kernel<KernelData>::GetInstance()->Log(message);
}

void Core::Log(LOG_LEVEL level, const char *fmt, ...) {
    kernelData.Console.PrintString("[");

    switch (level) {
        case LOG_LEVEL::INFO:
            kernel.Log("[INFO] ");
            kernelData.Console.PrintString(ANSI::Colors::Foreground::Green);
            kernelData.Console.PrintString("INFO");
            break;
        case LOG_LEVEL::WARNING:
            kernel.Log("[WARNING] ");
            kernelData.Console.PrintString(ANSI::Colors::Foreground::Yellow);
            kernelData.Console.PrintString("WARNING");
            break;
        case LOG_LEVEL::ERROR:
            kernel.Log("[ERROR] ");
            kernelData.Console.PrintString(ANSI::Colors::Foreground::Red);
            kernelData.Console.PrintString("ERROR");
            break;
        case LOG_LEVEL::DEBUG:
            kernel.Log("[DEBUG] ");
            kernelData.Console.PrintString(ANSI::Colors::Foreground::Cyan);
            kernelData.Console.PrintString("DEBUG");
            break;
    }

    kernelData.Console.PrintString("\033[0m] ");

    // Format the message
    char buffer[1025]; // +1 for null terminator
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

    kernel.Log(buffer);
    kernelData.Console.PrintString(buffer);
    kernelData.Console.PrintString("\r");
    if (truncated) {
        kernel.Log("...[TRUNCATED]");
        kernelData.Console.PrintString("...[TRUNCATED]\r");
    }
}

[[noreturn]] void Core::Panic(const char *message) {
    asm volatile ("cli"); // Disable interrupts to prevent any further damage or interference with the panic process.
    kernel.Panic(message);
}

template class Kernel<KernelData>; // Initialize the template class