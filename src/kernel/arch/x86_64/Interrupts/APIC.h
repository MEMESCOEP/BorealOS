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
            static constexpr uintptr_t APIC_HIGHER_HALF_OFFSET = 0xF8F8EFFE000;
            static constexpr uint32_t SVR_ENABLE_BIT = 1 << 8;
            static constexpr uint32_t LVT_MASK_BIT = 1 << 16;
            static constexpr uint16_t LVT_TIMER_OFFSET = 0x320;
            static constexpr uint16_t LVT_LINT0_OFFSET = 0x350;
            static constexpr uint16_t LVT_LINT1_OFFSET = 0x360;
            static constexpr uint16_t LVT_ERROR_OFFSET = 0x370;
            static constexpr uint8_t MSR_IA32_APIC_BASE = 0x1B;
            static constexpr uint8_t LAPIC_TPR_OFFSET = 0x80;
            static constexpr uint8_t LAPIC_SVR_OFFSET = 0xF0;
            static constexpr uint8_t SIV_OFFSET = 0x20;

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