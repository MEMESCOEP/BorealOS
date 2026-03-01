#include "APIC.h"

namespace Interrupts {
    APIC::APIC(Core::ACPI* acpi, Core::CPU* cpu, PIC* pic, Memory::Paging* paging) {
        _paging = paging;
        _acpi = acpi;
        _cpu = cpu;
        _pic = pic;
    }

    void APIC::WriteRegister(uint32_t regOffset, uint32_t value) {
        MMIOLAPICAddr[regOffset / 4] = value;
    }

    uint32_t APIC::ReadRegister(uint32_t regOffset) {
        return MMIOLAPICAddr[regOffset / 4];
    }

    void APIC::MaskLVTEntry(uint32_t regOffset) {
        WriteRegister(regOffset, ReadRegister(regOffset) | (1 << 16));
    }

    void APIC::UnmaskLVTEntry(uint32_t regOffset) {
        WriteRegister(regOffset, ReadRegister(regOffset) & ~(1 << 16));
    }

    // NOTE: This APIC init is uniprocessor only, it doesn't yet initialize other per-core APIC chips
    void APIC::Initialize() {
        // Check if this system has APIC
        if (!_cpu->CPUHasFeature(Core::CPUFeatures::APIC)) {
            PANIC("This system does not support APIC!");
        }

        // --- LAPIC ---
        // Extract the physical APIC base address (bits 12 to 35)
        uint64_t APICBaseIA32 = _cpu->ReadMSR(MSR_IA32_APIC_BASE);
        uint64_t MMIOLAPICPhysAddr = APICBaseIA32 & 0xFFFFFFFFFFFFF000ULL;
        MMIOLAPICAddr = reinterpret_cast<volatile uint32_t*>(MMIOLAPICPhysAddr);
        LOG_DEBUG("MMIO LAPIC address is %p.", MMIOLAPICAddr);

        // Check if the system supports x2APIC (bit 10) and that APIC is globally enabled (bit 11)
        if (APICBaseIA32 & (1ULL << 10)) PANIC("x2APIC mode is enabled, MMIO is not possible!");
        if ((APICBaseIA32 & (1ULL << 11)) == 0) PANIC("APIC is globally disabled!");

        // Map the APIC MMIO region
        if (MMIOLAPICPhysAddr % Architecture::KernelPageSize != 0) PANIC("LAPIC base address is not page aligned!");
        _paging->MapPage(
            MMIOLAPICPhysAddr,
            MMIOLAPICPhysAddr,
            Memory::PageFlags::ReadWrite | Memory::PageFlags::NoExecute | Memory::PageFlags::CacheDisable
        );        

        // Find the MADT
        _madt = (Core::ACPI::MADT*)_acpi->GetTable("APIC");
        if (!_madt) PANIC("Failed to find the MADT!");

        // The 8259 PIC chip MUST be disabled before APIC can be used
        _pic->Disable();

        uint32_t LAPICID = (ReadRegister(LAPIC_ID_REG_OFFSET) >> 24) & 0xFF;
        LOG_DEBUG("LAPIC ID: %u32", LAPICID);

        // Now we need to configure the Spurious Interrupt Vector Register
        WriteRegister(SPIRV_REG_OFFSET, SPIRV_VECTOR | (1 << 8));
        if ((ReadRegister(SPIRV_REG_OFFSET) & (1 << 8)) == 0) PANIC("Failed to configure LAPIC spurious vector register, bit 8 (software enable) of LAPIC SPIRV is not set!");

        // We should mask all of the Local Vector Table entries to put the LAPIC into a controlled state, this disables interrupts during initialization
        MaskLVTEntry(LVT_TIMER_OFFSET);
        MaskLVTEntry(LVT_LINT0_OFFSET);
        MaskLVTEntry(LVT_LINT1_OFFSET);
        MaskLVTEntry(LVT_ERROR_OFFSET);

        // We need to clear the Error Status Register by writing to it *TWICE*. After this we can issue an initial End Of Interrupt by writing 0x0 to offset 0xB0
        WriteRegister(ERROR_STATUS_REG_OFFSET, 0x00);
        WriteRegister(ERROR_STATUS_REG_OFFSET, 0x00);
        WriteRegister(EOI_REG_OFFSET, 0x00);



        // --- IOAPIC ---
        // Search the MADT's entries to find any IOAPIC (0x01) and ISO (0x02) entries
        uint8_t* VLRecordsPtr = (uint8_t*)_madt + MADT_VLRECORDS_OFFSET;
        uint8_t* MADTEnd = (uint8_t*)_madt + _madt->sdt.length;

        while (VLRecordsPtr < MADTEnd) {
            Core::ACPI::MADTEntryHeader* entryHeader = (Core::ACPI::MADTEntryHeader*)VLRecordsPtr;
            
            switch(entryHeader->type) {
                // IOAPIC descriptor
                case IOAPIC_ENTRY_TYPE: {
                    Core::ACPI::MADTIOAPIC* IOAPICEntry = (Core::ACPI::MADTIOAPIC*)VLRecordsPtr;
                    LOG_DEBUG("Found IOAPIC entry at %p:\n\r  * Physical address: %p\n\r  * ID: %u8\n\r  * GSI base: %p\n\r",
                        VLRecordsPtr,
                        IOAPICEntry->ioApicAddress,
                        IOAPICEntry->ioApicId,
                        IOAPICEntry->globalSystemInterruptBase
                    );
                    break;
                }

                // IRQ Source Override (ISO)
                case IRQ_SRCOVR_ENTRY_TYPE: {
                    Core::ACPI::MADTIOSrcOverride* overrideEntry = (Core::ACPI::MADTIOSrcOverride*)VLRecordsPtr;
                    LOG_DEBUG("Found IRQ src override entry at %p:\n\r  * Bus source: %u8\n\r  * Global system interrupt: %u32\n\r  * IRQ source: %u8\n\r",
                        VLRecordsPtr,
                        overrideEntry->busSource,
                        overrideEntry->globalSysInterrupt,
                        overrideEntry->IRQSource
                    );
                    break;
                }

                // We don't care about other entry types yet
                default:
                    break;
            }

            VLRecordsPtr += entryHeader->length;
        }
    }
}