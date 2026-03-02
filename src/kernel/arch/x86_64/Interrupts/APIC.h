#ifndef BOREALOS_APIC_H
#define BOREALOS_APIC_H

#include <Definitions.h>
#include "Utility/MemoryUtilities.h"
#include "Utility/List.h"
#include "../Core/ACPI.h"
#include "../Core/CPU.h"
#include "../Memory/Paging.h"
#include "IDT.h"
#include "PIC.h"

// APIC information was provided by the Intel 64 and IA-32 Software Developer's Manual, Volume 3A, Chapter 11
// See Table 11-1 on pages 390-391 for a table of LAPIC register offsets and their properties (only the second "half" of the address is used)
namespace Interrupts {
    class APIC {
        public:
            explicit APIC(Core::ACPI* acpi, Core::CPU* cpu, PIC* pic, Memory::Paging* paging);
            void Initialize();

            // MSRs
            static constexpr uint32_t MSR_IA32_APIC_BASE = 0x1B;

            // Register offsets
            static constexpr uint32_t LAPIC_ID_REG_OFFSET = 0x20;
            static constexpr uint32_t MADT_VLRECORDS_OFFSET = 0x2C;
            static constexpr uint32_t ERROR_STATUS_REG_OFFSET = 0x280;
            static constexpr uint32_t SPIRV_REG_OFFSET = 0xF0;
            static constexpr uint32_t EOI_REG_OFFSET = 0xB0;
            static constexpr uint32_t TPR_REG_OFFSET = 0x80;
            static constexpr uint32_t INITIAL_COUNT_REG_OFFSET = 0x380;
            static constexpr uint32_t DIVIDE_CONFIG_REG_OFFSET = 0x3E0;

            // LVT offsets
            static constexpr uint32_t LVT_TIMER_OFFSET = 0x320;
            static constexpr uint32_t LVT_LINT0_OFFSET = 0x350;
            static constexpr uint32_t LVT_LINT1_OFFSET = 0x360;
            static constexpr uint32_t LVT_ERROR_OFFSET = 0x370;

            // Vectors
            static constexpr uint32_t SPIRV_VECTOR = 0xFE;

            // Entry types
            static constexpr uint8_t IRQ_SRCOVR_ENTRY_TYPE = 0x02;
            static constexpr uint8_t IOAPIC_ENTRY_TYPE = 0x01;

            // Misc
            static constexpr uint8_t MINIMUM_IRQ_NUM = 0x00;

        private:
            void WriteRegister(uint32_t regOffset, uint32_t value);
            uint32_t ReadRegister(uint32_t regOffset);
            void UnmaskLVTEntry(uint32_t regOffset);
            void MaskLVTEntry(uint32_t regOffset);
            
            Utility::List<Core::ACPI::MADTIRQSrcOverride*> _IRQSrcOverrides;
            Utility::List<Core::ACPI::MADTIOAPIC*> _IOAPICEntries;
            Memory::Paging* _paging;
            Core::ACPI::MADT* _madt;
            Core::ACPI* _acpi;
            Core::CPU* _cpu;
            PIC* _pic;

            volatile uint32_t* MMIOLAPICAddr;
    };
}

#endif