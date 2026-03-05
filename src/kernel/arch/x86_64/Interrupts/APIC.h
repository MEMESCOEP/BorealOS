#ifndef BOREALOS_APIC_H
#define BOREALOS_APIC_H

#include <Definitions.h>
#include <Kernel.h>
#include "Utility/MemoryUtilities.h"
#include "Utility/List.h"
#include "../Core/Firmware/ACPI.h"
#include "../Core/CPU.h"
#include "../Memory/Paging.h"
#include "IDT.h"
#include "PIC.h"

// APIC information was provided by the Intel 64 and IA-32 Software Developer's Manual, Volume 3A, Chapter 11
// See Table 11-1 on pages 390-391 for a table of LAPIC register offsets and their properties (only the second "half" of the address is used)
namespace Interrupts {
    class APIC : public InterruptController {
        public:
            explicit APIC(Core::Firmware::ACPI* acpi, Core::CPU* cpu, PIC* pic, Memory::Paging* paging, IDT* idt);
            void Initialize() override;

            // Register offsets
            static constexpr uint32_t IOAPIC_VERSION_REG_OFFSET = 0x01;
            static constexpr uint32_t LAPIC_ID_REG_OFFSET       = 0x20;
            static constexpr uint32_t MADT_VLRECORDS_OFFSET     = 0x2C;
            static constexpr uint32_t ERROR_STATUS_REG_OFFSET   = 0x280;
            static constexpr uint32_t SPIRV_REG_OFFSET          = 0xF0;
            static constexpr uint32_t EOI_REG_OFFSET            = 0xB0;
            static constexpr uint32_t TPR_REG_OFFSET            = 0x80;
            static constexpr uint32_t INITIAL_COUNT_REG_OFFSET  = 0x380;
            static constexpr uint32_t DIVIDE_CONFIG_REG_OFFSET  = 0x3E0;

            // LVT offsets
            static constexpr uint32_t LVT_TIMER_OFFSET = 0x320;
            static constexpr uint32_t LVT_LINT0_OFFSET = 0x350;
            static constexpr uint32_t LVT_LINT1_OFFSET = 0x360;
            static constexpr uint32_t LVT_ERROR_OFFSET = 0x370;

            // Vectors
            static constexpr uint32_t SPIRV_VECTOR = 0xFF;
            static constexpr uint32_t LVT_VECTOR   = 0x40;

            // Entry types
            static constexpr uint8_t IRQ_SRCOVR_ENTRY_TYPE = 0x02;
            static constexpr uint8_t IOAPIC_ENTRY_TYPE     = 0x01;

            // Misc
            static constexpr uint32_t MSR_IA32_APIC_BASE = 0x1B;
            static constexpr uint8_t  MINIMUM_IRQ_NUM    = 0x00;
            static constexpr uint8_t  IRQ_OFFSET         = 0x20;

        private:
            struct IOAPICData {
                volatile uint32_t* base;
                uint32_t gsiBase;
                uint8_t redirectionEntryCount;
            };

            void WriteIOAPICRegister(volatile uint32_t* base, uint8_t regOffset, uint32_t value);
            void WriteLAPICRegister(uint32_t regOffset, uint32_t value);
            
            uint32_t ReadIOAPICRegister(volatile uint32_t* base, uint8_t regOffset);
            uint32_t ReadLAPICRegister(uint32_t regOffset);

            Core::Firmware::ACPI::MADTIRQSrcOverride* GetIRQSrcOverride(uint8_t irqNum);
            IOAPICData* GetIOAPICFromIRQ(uint32_t irqNum);
            void UnmaskLVTEntry(uint32_t regOffset);
            void MaskLVTEntry(uint32_t regOffset);
            void MaskIRQ(uint8_t irqNum) override;
            void UnmaskIRQ(uint8_t irqNum) override;
            void MapIRQ(uint8_t irqNum, uint8_t irqVector);
            void SendEOI(uint8_t irq) override;
            
            Utility::List<Core::Firmware::ACPI::MADTIRQSrcOverride*> _IRQSrcOverrides;
            Utility::List<Core::Firmware::ACPI::MADTIOAPIC*> _IOAPICEntries;
            Utility::List<IOAPICData*> _IOAPICs;
            Memory::Paging* _paging;
            Core::Firmware::ACPI::MADT* _madt;
            Core::Firmware::ACPI* _acpi;
            Core::CPU* _cpu;
            PIC* _pic;
            IDT* _idt;

            volatile uint32_t* MMIOLAPICAddr;
            uint32_t _LAPICID;
    };
}

#endif