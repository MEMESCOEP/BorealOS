#include <Definitions.h>
#include <Kernel.h>
#include "KernelData.h"
#include "Interrupts/GDT.h"
#include "IO/Serial.h"
#include "IO/SerialPort.h"

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
    LOG(LOG_LEVEL::INFO, "Loaded serial port.");

    // Gdt:
    Interrupts::GDT::initialize();
    LOG(LOG_LEVEL::INFO, "Initialized GDT.");
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
    Log(message);

    while (true) {
        asm ("hlt");
    }
}

void Core::Write(const char *message) {
    Kernel<KernelData>::GetInstance()->Log(message);
}

void Core::Log(LOG_LEVEL level, const char *message) {
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
    Kernel<KernelData>::GetInstance()->Log(message);
}

[[noreturn]] void Core::Panic(const char *message) {
    Kernel<KernelData>::GetInstance()->Panic(message);
}

template class Kernel<KernelData>; // Initialize the template class