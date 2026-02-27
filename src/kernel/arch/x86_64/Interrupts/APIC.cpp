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

        // Extract the physical APIC base address (bits 12 to 35)
        uint64_t APICBaseIA32 = _cpu->ReadMSR(MSR_IA32_APIC_BASE);
        uint64_t MMIOLAPICPhysAddr = APICBaseIA32 & 0xFFFFFFFFFFFFF000ULL;
        uint64_t MMIOLAPICVirtAddr = HIGHER_HALF(MMIOLAPICPhysAddr);
        MMIOLAPICAddr = reinterpret_cast<volatile uint32_t*>(MMIOLAPICVirtAddr);

        // Check if the system supports x2APIC (bit 10) and that APIC is globally enabled (bit 11)
        if (APICBaseIA32 & (1ULL << 10)) PANIC("x2APIC mode is enabled, MMIO is not possible!");
        if ((APICBaseIA32 & (1ULL << 11)) == 0) PANIC("APIC is globally disabled!");

        // Map the APIC MMIO region
        if (MMIOLAPICVirtAddr % Architecture::KernelPageSize != 0) PANIC("LAPIC base address is not page aligned!");
        _paging->MapPage(
            MMIOLAPICVirtAddr,
            MMIOLAPICPhysAddr,
            Memory::PageFlags::ReadWrite | Memory::PageFlags::NoExecute | Memory::PageFlags::CacheDisable
        );

        LOG_DEBUG("MMIO LAPIC address is %p.", MMIOLAPICAddr);

        // Find the MADT
        _madt = (Core::ACPI::MADT*)_acpi->GetTable("APIC");
        if (!_madt) PANIC("Failed to find the MADT!");

        // The 8259 PIC chip MUST be disabled before APIC can be used
        if (_madt->flags & 1) LOG_WARNING("One or more legacy PIC chips were detected, they must be disabled for APIC to work properly.");
        _pic->Disable();

        uint32_t LAPICID = MMIOLAPICAddr[0x20 / 4];
        LOG_DEBUG("LAPIC ID: %u32", LAPICID);
    }
}