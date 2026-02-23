#include "APIC.h"

namespace Interrupts {
    APIC::APIC(Core::ACPI* acpi, Core::CPU* cpu, PIC* pic, Memory::Paging* paging) {
        _paging = paging;
        _acpi = acpi;
        _cpu = cpu;
        _pic = pic;
    }

    // This initialization must not fail, as we rely on APIC. If it does go wrong, we have to stop the kernel!
    void APIC::Initialize() {
        // Check if this system has APIC
        if (!_cpu->CPUHasFeature(Core::CPUFeatures::APIC)) {
            PANIC("This system does not support APIC!");
        }

        // Get the MADT table and the LAPIC address. Since it's MMIO, we need to map the pages correctly
        _madt = (Core::ACPI::MADT*)_acpi->GetTable("APIC");
        
        if (!_madt) {
            PANIC("Failed to find APIC MADT table!");
        }

        // Map the APIC MMIO region
        _paging->MapPage(
            _madt->localAPICAddr + APICHigherHalfOffset,
            _madt->localAPICAddr,
            Memory::PageFlags::ReadWrite | Memory::PageFlags::NoExecute | Memory::PageFlags::CacheDisable
        );

        volatile uint32_t* MMIOLAPICAddr = reinterpret_cast<volatile uint32_t*>(_madt->localAPICAddr + APICHigherHalfOffset);
        LOG_DEBUG("APIC MADT table is at address %p.", _madt);
        LOG_DEBUG("Local APIC address is %p (physical address is %p).", MMIOLAPICAddr, _madt->localAPICAddr);

        // The 8259 PIC chip MUST be disabled before APIC can be used
        // Note that if this APIC init fails, the PIC will be left disabled!
        if (_madt->flags & 1) LOG_WARNING("One or more legacy PIC chips were detected, they must be disabled for APIC to work properly.");
        _pic->Disable();

        // Now we need to enable APIC mode
        uint64_t APICBaseIA32 = _cpu->ReadMSR(0x1B);
        SET_BIT(APICBaseIA32, 11);
        _cpu->WriteMSR(0x1B, APICBaseIA32);

        // Verify the write to bit 11
        APICBaseIA32 = _cpu->ReadMSR(0x1B);

        if (!(APICBaseIA32 & (1ULL << 11))) PANIC("Enabling APIC mode failed!");
        LOG_DEBUG("IA32 APIC base is %p.", APICBaseIA32);
    }
}