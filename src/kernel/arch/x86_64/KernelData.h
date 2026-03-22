#ifndef BOREALOS_KERNELDATA_H
#define BOREALOS_KERNELDATA_H

#include "Interrupts/APIC.h"
#include "Interrupts/GDT.h"
#include "Interrupts/IDT.h"
#include "Interrupts/PIC.h"
#include "IO/Serial.h"
#include "IO/SerialPort.h"
#include "IO/FramebufferConsole.h"
#include "Memory/Paging.h"
#include "Memory/PMM.h"
#include "Core/CPU.h"
#include "Core/Time/RTC.h"
#include "FileSystems/InitRam.h"
#include "Memory/HeapAllocator.h"
#include "Core/Firmware/ACPI.h"
#include "Core/Drivers/DriverManager.h"
#include <Core/ServiceManager.h>
#include "Core/Firmware/Hardware.h"
#include "Core/Time/HPET.h"
#include "Core/Time/Scheduler.h"
#include "Core/Time/TSC.h"
#include "Formats/SymbolLoader.h"
#include "IO/PCI.h"

struct KernelData {
    IO::SerialPort SerialPort {IO::Serial::COM1};
    Interrupts::PIC* Pic;
    Interrupts::IDT Idt {Pic};
    IO::FramebufferConsole Console;
    Core::Time::RTC Rtc {&Idt};
    Memory::PMM Pmm;
    Core::CPU Cpu;
    Memory::Paging Paging {&Pmm};
    Memory::HeapAllocator HeapAllocator {&Pmm, &Paging, Paging.GetKernelPagingState()};
    Core::Firmware::ACPI Acpi;
    Core::Firmware::Hardware Hardware {&Acpi};
    Core::Time::HPET Hpet {&Acpi, &Paging, &Idt};
    Interrupts::APIC *Apic;
    Core::Time::TSC Tsc {&Hpet, &Cpu};
    FileSystem::InitRam* InitRamFS;
    Formats::SymbolLoader *KernelSymbols;
    Core::ServiceManager *ServiceManager;
    Core::Drivers::DriverManager *DriverManager;
    Core::Time::Scheduler *DefaultScheduler; // Core 0 scheduler. Other cores should have their own scheduler instance.
    IO::PCI Pci {&Paging};
};

#endif //BOREALOS_KERNELDATA_H
