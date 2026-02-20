#ifndef BOREALOS_HPET_H
#define BOREALOS_HPET_H

#include <Definitions.h>

#include "../ACPI.h"
#include "../../Memory/Paging.h"
#include "../../Interrupts/IDT.h"
#include "../../Memory/HeapAllocator.h"

namespace Core::Time {
    class HPET {
    public:
        explicit HPET(ACPI* acpi, Memory::Paging *paging, Interrupts::IDT* idt);
        void Initialize();

        [[nodiscard]] uint64_t GetCounter() const;
        [[nodiscard]] uint64_t GetNanoseconds() const;

        void BusyWait(uint64_t nanoseconds) const;

    private:
        ACPI* _acpi;
        Memory::Paging* _paging;
        Interrupts::IDT* _idt;

        static constexpr uint64_t FemtoSecond = 1'000'000'000'000'000ULL; // 1 second in femtoseconds (10 ^ 15)

        struct HPETTable
        {
            ACPI::SDTHeader Header;
            uint32_t EventTimerBlockID;
            ACPI::GenericAddr Address;
            uint8_t HPETNumber;
            uint16_t MinimumTick;
            uint8_t PageProtection;
        } PACKED;

        struct HPETTimer {
            uint64_t ConfigAndCapabilities; // Timer configuration and capabilities
            uint64_t ComparatorValue; // Timer comparator value
            uint64_t FSBInterruptRoute; // FSB interrupt route (if supported)
            uint64_t _reserved; // Reserved for future use
        } PACKED;

        struct HPETRegisters {
            uint64_t GeneralCapabilitiesID; // 0x00
            uint64_t _padding1;
            uint64_t GeneralConfiguration; // 0x10
            uint64_t _padding2;
            uint64_t GeneralInterruptStatus; // 0x20
            // Reserve up to 0xF0 for the main counter and timers
            char _reserved[0xC8];
            uint64_t MainCounterValue; // 0xF0
            uint64_t _reserved3;
            HPETTimer Timers[32]; // maximum 32 timers
        } PACKED;

        struct HPETCapabilities {
            uint64_t RevisionId : 8; // Bits 0-7: Implementation revision
            uint64_t NumTimersCap : 5; // Bits 8-12: Number of timers - 1
            uint64_t CountSizeCap : 1; // Bit 13: 1 = 64-bit counter support
            uint64_t Reserved : 1; // Bit 14: Reserved
            uint64_t LegacyRouteCap : 1; // Bit 15: 1 = Legacy replacement capable
            uint64_t VendorId : 16; // Bits 16-31: PCI-style Vendor ID
            uint64_t CounterClockPeriod : 32;// Bits 32-63: Tick period in femtoseconds
        } PACKED;

        HPETTable* _hpetTable;
        void* _hpetMappedAddress;
        uint64_t _hpetFrequency;
        uint64_t _timerCount;
        uint64_t _totalTicks;
        uint64_t _lastCounter;
        void Tick();
    };
}

#endif //BOREALOS_HPET_H
