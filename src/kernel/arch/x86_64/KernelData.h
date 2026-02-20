#ifndef BOREALOS_KERNELDATA_H
#define BOREALOS_KERNELDATA_H

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
#include "File/Systems/InitRam.h"
#include "Memory/HeapAllocator.h"
#include "Core/ACPI.h"
#include "Core/Time/HPET.h"

struct KernelData {
    IO::SerialPort SerialPort {IO::Serial::COM1};
    Interrupts::PIC Pic {0x20, 0x28};
    Interrupts::IDT Idt {&Pic};
    IO::FramebufferConsole Console;
    Core::Time::RTC Rtc {&Idt};
    Memory::PMM Pmm;
    Core::CPU Cpu;
    Memory::Paging Paging {&Pmm};
    Memory::HeapAllocator HeapAllocator {&Pmm, &Paging, Paging.GetKernelPagingState(), Memory::HeapAllocator::HeapHigherHalf};
    Core::ACPI Acpi;
    Core::Time::HPET Hpet {&Acpi, &Paging, &Idt};
    File::Systems::InitRam InitRamFS {nullptr, &HeapAllocator};
};

#endif //BOREALOS_KERNELDATA_H