#include "HPET.h"

#include "Kernel.h"
#include "../../KernelData.h"

namespace Core::Time {
    HPET::HPET(ACPI *acpi, Memory::Paging *paging, Interrupts::IDT* idt) : _acpi(acpi), _paging(paging), _idt(idt), _totalTicks(0) {
        _hpetTable = reinterpret_cast<HPETTable*>(_acpi->GetTable("HPET"));
    }

    void HPET::Initialize() {
        if (!_hpetTable) {
            PANIC("No HPET table found in ACPI, cannot initialize HPET! (Are you running on a system pre 2005? or a virtual machine that doesn't support HPET?)");
        }

        uint64_t mmioAddr = _hpetTable->Address.Address;

        // Map the HPET MMIO region
        _paging->MapPage(
            mmioAddr,
            mmioAddr,
            Memory::PageFlags::ReadWrite | Memory::PageFlags::NoExecute | Memory::PageFlags::CacheDisable
        );

        _hpetMappedAddress = reinterpret_cast<void*>(mmioAddr);
        volatile auto hpetRegs = static_cast<HPETRegisters*>(_hpetMappedAddress);
        volatile auto capabilities = reinterpret_cast<HPETCapabilities*>(&hpetRegs->GeneralCapabilitiesID);

        _hpetFrequency = FemtoSecond / capabilities->CounterClockPeriod;
        _timerCount = capabilities->NumTimersCap + 1;

        LOG_DEBUG("HPET capabilities:");
        LOG_DEBUG("  Counter Clock Period: %u64hz", _hpetFrequency);
        LOG_DEBUG("  Number of Timers: %u64", _timerCount);
        LOG_DEBUG("  Legacy Replacement Route: %s", capabilities->LegacyRouteCap ? "Yes" : "No");
        LOG_DEBUG("  Counter Size: %s", capabilities->CountSizeCap ? "64-bit" : "32-bit");

        asm volatile ("cli"); // Disable interrupts while configuring HPET

        // Disable HPET before configuring it
        volatile auto configReg = reinterpret_cast<uint64_t *>(static_cast<char *>(_hpetMappedAddress) + 0x10);
        *configReg = 0;

        volatile auto timer0Config = reinterpret_cast<uint64_t *>(static_cast<char *>(_hpetMappedAddress) + 0x100); // Timer 0 config is at the same offset as its capabilities
        volatile auto timer0Comparator = reinterpret_cast<uint32_t *>(static_cast<char *>(_hpetMappedAddress) + 0x108); // Timer 0 comparator is at offset 0x108

        *timer0Config = 0;

        // https://wiki.osdev.org/HPET
        SET_BIT(*timer0Config, 2); // Setting this bit to 1 enables triggering of interrupts. Even if this bit is 0, this timer will still set Tn_INT_STS.
        SET_BIT(*timer0Config, 3); // If Tn_PER_INT_CAP is 1, then writing 1 to this field enables periodic timer and writing 0 enables non-periodic mode. Otherwise, this bit will be ignored and reading it will always return 0.
        SET_BIT(*timer0Config, 6); // This field is used to allow software to directly set periodic timer's accumulator. Detailed explanation is provided further in the article.

        *timer0Comparator = _hpetFrequency * 30; // Should be safe enough to not cause an integer overflow on 32 bit counters.

        _idt->RegisterIRQHandler(0, []() {
            auto* hpet = &Kernel<KernelData>::GetInstance()->ArchitectureData->Hpet;
            hpet->Tick();
        });

        _idt->ClearIRQMask(0);

        SET_BIT(*configReg, 1); // Enable legacy replacement mode. TODO: When using the modern APIC, replace this.
        SET_BIT(*configReg, 0); // Enable the HPET main counter

        asm volatile ("sti"); // Re-enable interrupts after configuring HPET
        _lastCounter = GetCounter();
    }

    uint64_t HPET::GetCounter() const {
        volatile auto hpetRegs = static_cast<HPETRegisters*>(_hpetMappedAddress);
        return hpetRegs->MainCounterValue;
    }

    uint64_t HPET::GetNanoseconds() const {
        uint64_t total;
        uint32_t last;
        asm volatile("cli");
        total = _totalTicks;
        last = _lastCounter;
        asm volatile("sti");

        auto current = static_cast<uint32_t>(GetCounter());
        uint32_t delta = current - last;

        return (total + delta) * 1'000'000'000ULL / _hpetFrequency;
    }

    void HPET::BusyWait(uint64_t nanoseconds) const {
        uint64_t startTime = GetNanoseconds();
        while (GetNanoseconds() - startTime < nanoseconds) {
        }
    }

    void HPET::Tick() {
        auto current32 = static_cast<uint32_t>(GetCounter());
        auto last32 = static_cast<uint32_t>(_lastCounter);
        auto delta = current32 - last32; // Standard 32-bit wrap-around math
        _totalTicks += delta;
        _lastCounter = current32;
    }
}
