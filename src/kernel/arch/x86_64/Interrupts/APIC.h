#ifndef BOREALOS_APIC_H
#define BOREALOS_APIC_H

#include <Definitions.h>
#include "Utility/MemoryUtilities.h"
#include "../Core/ACPI.h"
#include "../Core/CPU.h"
#include "../Memory/Paging.h"
#include "IDT.h"
#include "PIC.h"

namespace Interrupts {
    class APIC {
        public:
            explicit APIC(Core::ACPI* acpi, Core::CPU* cpu, PIC* pic, Memory::Paging* paging);
            void LAPIC_Spurious_Handler();
            void Initialize();
            static constexpr uint8_t MSR_IA32_APIC_BASE = 0x1B;

        private:
            Memory::Paging* _paging;
            Core::ACPI::MADT* _madt;
            Core::ACPI* _acpi;
            Core::CPU* _cpu;
            PIC* _pic;
            volatile uint32_t* MMIOLAPICAddr;
    };
}

#endif