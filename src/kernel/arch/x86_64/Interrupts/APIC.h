#ifndef BOREALOS_APIC_H
#define BOREALOS_APIC_H

#include <Definitions.h>
#include "Utility/MemoryUtilities.h"
#include "../Core/ACPI.h"
#include "../Core/CPU.h"
#include "../Memory/Paging.h"
#include "PIC.h"

namespace Interrupts {
    class APIC {
        public:
            explicit APIC(Core::ACPI* acpi, Core::CPU* cpu, PIC* pic, Memory::Paging* paging);
            void Initialize();
            static constexpr uintptr_t APICHigherHalfOffset = 0xF8F8EFFE000;

        private:
            Memory::Paging* _paging;
            Core::ACPI::MADT* _madt;
            Core::ACPI* _acpi;
            Core::CPU* _cpu;
            PIC* _pic;
    };
}

#endif