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
#include "Memory/HeapAllocator.h"

struct KernelData {
    IO::SerialPort SerialPort {IO::Serial::COM1};
    Interrupts::PIC Pic {0x20, 0x28};
    Interrupts::IDT Idt {&Pic};
    IO::FramebufferConsole Console;
    Memory::PMM Pmm;
    Memory::Paging Paging {&Pmm};
    Memory::HeapAllocator HeapAllocator {&Pmm, &Paging, Paging.GetKernelPagingState()};
};

#endif //BOREALOS_KERNELDATA_H