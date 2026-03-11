#include "HPET.h"

#include "Kernel.h"
#include "../../KernelData.h"

namespace Core::Time {
    HPET::HPET(Firmware::ACPI *acpi, Memory::Paging *paging, Interrupts::IDT* idt) : _acpi(acpi), _paging(paging), _idt(idt), _totalTicks(0) {
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
    LOG_DEBUG("  Counter Size: %s", capabilities->CountSizeCap ? "64-bit" : "32-bit");

    asm volatile ("cli"); // Disable interrupts while configuring HPET

    // Disable HPET before configuring it
    volatile auto configReg = reinterpret_cast<uint64_t *>(static_cast<char *>(_hpetMappedAddress) + 0x10);
    *configReg = 0;

    volatile auto timer0Config = reinterpret_cast<uint64_t *>(static_cast<char *>(_hpetMappedAddress) + 0x100); // Timer 0 config is at the same offset as its capabilities
    volatile auto timer0Comparator = reinterpret_cast<uint64_t *>(static_cast<char *>(_hpetMappedAddress) + 0x108); // Timer 0 comparator is at offset 0x108
    uint32_t allowedGsis = static_cast<uint32_t>(*timer0Config >> 32);
    uint8_t selectedGsi = 0;
        for (uint64_t i = 0; i < _timerCount; i++) {
            volatile auto timerCfg = reinterpret_cast<uint64_t*>(
                static_cast<char*>(_hpetMappedAddress) + 0x100 + (i * 0x20)
            );
            uint64_t rawCaps = *timerCfg;
            bool periodic = (rawCaps >> 32) & (1 << 4);
            uint32_t gsiMask = static_cast<uint32_t>(rawCaps >> 32);
            LOG_DEBUG("Timer %u64: periodic=%s, GSI mask=0x%x32", i, periodic ? "yes" : "no", gsiMask);
        }
    // Find the first available GSI in the capability bitmask
    for (uint8_t i = 0; i < 32; i++) {
        if (allowedGsis & (1 << i)) {
            selectedGsi = i;
            break;
        }
    }
        uint64_t rawCaps = *timer0Config;
        bool periodicSupported = (rawCaps >> 32) & (1 << 4); // bit 36 overall
        LOG_DEBUG("Periodic mode supported: %s", periodicSupported ? "yes" : "no");
    // https://wiki.osdev.org/HPET
    // We need to set timer 0 to a periodic interrupt firing every second, to call our Tick() function.
    *timer0Config = 0;
    SET_BIT(*timer0Config, 3); // Set periodic mode
    SET_BIT(*timer0Config, 2); // Enable interrupts from this timer
    CLEAR_BIT(*timer0Config, 1); // Set interrupt type to edge-triggered

    // Set bits 9-12 to the selected GSI for this timer
    *timer0Config |= (static_cast<uint64_t>(selectedGsi) << 9);

    SET_BIT(*timer0Config, 6);
    *timer0Comparator = _hpetFrequency * 1;

    // Register the HPET interrupt handler for timer 0.
    _idt->RegisterIRQHandler(2, []() {
        auto *hpet = &Kernel<KernelData>::GetInstance()->ArchitectureData->Hpet;
        static uint64_t lastTime = 0;
        uint64_t currentTime = hpet->GetNanoseconds();
        uint64_t delta = currentTime - lastTime;
        uint64_t msDelta = delta / 1'000'000ULL;
        LOG_DEBUG("Tick! Time since last tick: %u64ms", msDelta);
        lastTime = currentTime;
    });

    auto *apic = Kernel<KernelData>::GetInstance()->ArchitectureData->Apic;
    apic->MapGSI(selectedGsi, 2 + 0x20, false, true, false); // Get the raw vector, by adding 32
    _idt->UnmaskIRQ(selectedGsi);

    // Reset the main counter.
    volatile auto mainCounter = reinterpret_cast<uint64_t*>(static_cast<char*>(_hpetMappedAddress) + 0xF0);
    *mainCounter = 0;

    // Finally, enable HPET
    SET_BIT(*configReg, 0);

    asm volatile ("sti");
    while (true);
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

    uint64_t HPET::GetFrequency() const {
        return _hpetFrequency;
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
