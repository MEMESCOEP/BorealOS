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

        LOG_DEBUG("APIC MADT table is at address %p.", _madt);
        uint64_t LAPICAddressMADT = (uint64_t)_madt->localAPICAddr;

        // Extract the physical APIC base address (bits 12 to 35)
        uint64_t APICBaseIA32 = _cpu->ReadMSR(MSR_IA32_APIC_BASE);
        uint64_t LAPICBaseAddr = APICBaseIA32 & 0xFFFFFFFFFFFFF000ULL;
        if (LAPICAddressMADT != LAPICBaseAddr) LOG_WARNING("MADT LAPIC address %p and MSR LAPIC address %p do not match!", LAPICAddressMADT, LAPICBaseAddr);

        // Check if the system supports x2APIC (bit 10) and that APIC is globally enabled (bit 11)
        if (APICBaseIA32 & (1ULL << 10)) PANIC("x2APIC mode is enabled, MMIO is not possible!");
        if ((APICBaseIA32 & (1ULL << 11)) == 0) PANIC("APIC is globally disabled!");

        // Map the APIC MMIO region
        _paging->MapPage(
            LAPICBaseAddr + APIC_HIGHER_HALF_OFFSET,
            LAPICBaseAddr,
            Memory::PageFlags::ReadWrite | Memory::PageFlags::NoExecute | Memory::PageFlags::CacheDisable
        );

        MMIOLAPICAddr = reinterpret_cast<volatile uint32_t*>(LAPICBaseAddr + APIC_HIGHER_HALF_OFFSET);
        LOG_DEBUG("Local APIC address is %p (physical address is %p).", MMIOLAPICAddr, LAPICBaseAddr);

        // The 8259 PIC chip MUST be disabled before APIC can be used
        // Note that if this APIC init fails, the PIC will be left disabled!
        if (_madt->flags & 1) LOG_WARNING("One or more legacy PIC chips were detected, they must be disabled for APIC to work properly.");
        _pic->Disable();

        // Now that the PIC is disabled, we need to configure the spurious interrupt vector for the LAPIC. I will use the same offset as the standard PIC, which is 32
        MMIOLAPICAddr[LAPIC_SVR_OFFSET / 4] = SIV_OFFSET | SVR_ENABLE_BIT;
        if ((MMIOLAPICAddr[LAPIC_SVR_OFFSET / 4] & 0xFF) != SIV_OFFSET) PANIC("Failed to set spurious interrupt vector!");
        LOG_DEBUG("LAPIC spurious interrupt vector is now %p, IRQ numbers will now start from %u8.", SIV_OFFSET, SIV_OFFSET);
        
        // Clear the task priority register
        MMIOLAPICAddr[LAPIC_TPR_OFFSET / 4] = 0;

        // Mask unused LVT entries
        MMIOLAPICAddr[LVT_TIMER_OFFSET / 4] = LVT_MASK_BIT;
        MMIOLAPICAddr[LVT_LINT0_OFFSET / 4] = LVT_MASK_BIT;
        MMIOLAPICAddr[LVT_LINT1_OFFSET / 4] = LVT_MASK_BIT;
        MMIOLAPICAddr[LVT_ERROR_OFFSET / 4] = LVT_MASK_BIT;
    }
}