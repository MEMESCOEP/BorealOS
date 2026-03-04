#include "APIC.h"

#include "../IO/Serial.h"

namespace Interrupts {
    APIC::APIC(Core::ACPI* acpi, Core::CPU* cpu, PIC* pic, Memory::Paging* paging, IDT* idt) : _IRQSrcOverrides(256), _IOAPICEntries(256), _IOAPICs(256) {
        _paging = paging;
        _acpi = acpi;
        _cpu = cpu;
        _pic = pic;
        _idt = idt;
    }

    Core::ACPI::MADTIRQSrcOverride* APIC::GetIRQSrcOverride(uint8_t irqNum) {
        for (uint8_t i = 0; i < _IRQSrcOverrides.Size(); i++) {
            if (_IRQSrcOverrides[i]->IRQSource == irqNum) {
                return _IRQSrcOverrides[i];
            }
        }

        return nullptr;
    }

    APIC::IOAPICData* APIC::GetIOAPICFromIRQ(uint32_t irqNum) {
        for (uint8_t i = 0; i < _IOAPICs.Size(); i++) {
            IOAPICData* ioapic = _IOAPICs[i];
            if (irqNum >= ioapic->gsiBase && irqNum < ioapic->gsiBase + ioapic->redirectionEntryCount) {
                return ioapic;
            }
        }

        return nullptr;
    }

    void APIC::WriteLAPICRegister(uint32_t regOffset, uint32_t value) {
        MMIOLAPICAddr[regOffset / 4] = value;
    }

    uint32_t APIC::ReadLAPICRegister(uint32_t regOffset) {
        return MMIOLAPICAddr[regOffset / 4];
    }

    void APIC::WriteIOAPICRegister(volatile uint32_t* base, uint8_t regOffset, uint32_t value) {
        base[0] = regOffset;        // IOREGSEL
        base[4] = value;            // IOWIN (0x10 / 4 = index 4)
    }

    uint32_t APIC::ReadIOAPICRegister(volatile uint32_t* base, uint8_t regOffset) {
        base[0] = regOffset;        // IOREGSEL
        return base[4];             // IOWIN
    }

    void APIC::MaskLVTEntry(uint32_t regOffset) {
        WriteLAPICRegister(regOffset, ReadLAPICRegister(regOffset) | (1 << 16));
    }

    void APIC::UnmaskLVTEntry(uint32_t regOffset) {
        WriteLAPICRegister(regOffset, ReadLAPICRegister(regOffset) & ~(1 << 16));
    }

    void APIC::MaskIRQ(uint8_t irqNum) {
        // Check if there is an override available for this IRQ
        Core::ACPI::MADTIRQSrcOverride* IRQOverride = GetIRQSrcOverride(irqNum);
        uint32_t gsi = IRQOverride ? IRQOverride->globalSysInterrupt : irqNum;
        
        // Check if any IOAPIC chips are set up to handle this IRQ
        IOAPICData* APICData = GetIOAPICFromIRQ(gsi);
        if (!APICData) {
            LOG_ERROR("No IOAPICs are set up to handle IRQ #%u8", irqNum);
            PANIC("No IOAPIC was found for the IRQ!");
        }        

        // Now we can actually mask the IRQ
        uint8_t entry = gsi - APICData->gsiBase;
        uint8_t reg = 0x10 + (entry * 2);
        WriteIOAPICRegister(APICData->base, reg, ReadIOAPICRegister(APICData->base, reg) | (1 << 16));
    }

    void APIC::UnmaskIRQ(uint8_t irqNum) {
        // Check if there is an override available for this IRQ
        Core::ACPI::MADTIRQSrcOverride* IRQOverride = GetIRQSrcOverride(irqNum);
        uint32_t gsi = IRQOverride ? IRQOverride->globalSysInterrupt : irqNum;
        
        // Check if any IOAPIC chips are set up to handle this IRQ
        IOAPICData* APICData = GetIOAPICFromIRQ(gsi);
        if (!APICData) {
            LOG_ERROR("No IOAPICs are set up to handle IRQ #%u8", irqNum);
            PANIC("No IOAPIC was found for the IRQ!");
        }        

        // Now we can actually unmask the IRQ
        uint8_t entry = gsi - APICData->gsiBase;
        uint8_t reg = 0x10 + (entry * 2);
        WriteIOAPICRegister(APICData->base, reg, ReadIOAPICRegister(APICData->base, reg) & ~(1 << 16));
    }

    void APIC::SendEOI(uint8_t irqNum) {
        // Vector 0xFF must not receive an EOI
        if (irqNum == 0xFF) return;

        WriteLAPICRegister(EOI_REG_OFFSET, 0x00);
    }

    void APIC::MapIRQ(uint8_t irqNum, uint8_t irqVector) {
        // Check if there is an override available for this IRQ
        Core::ACPI::MADTIRQSrcOverride* IRQOverride = GetIRQSrcOverride(irqNum);
        uint32_t gsi = IRQOverride ? IRQOverride->globalSysInterrupt : irqNum;

        // ISA PIC defaults to active high and edge triggered modes
        uint8_t polarity = 0;
        uint8_t triggerMode = 0;

        if (IRQOverride) {
            // Make sure the override IRQ actually matches this IRQ
            if (IRQOverride->IRQSource != irqNum) {
                LOG_ERROR("IRQ override source (%u8) does not match IRQ number (%u8)!", IRQOverride->IRQSource, irqNum);
                PANIC("Invalid IRQ source override!");
            }

            uint8_t polarityFlags = IRQOverride->flags & 0x03;
            uint8_t triggerFlags = (IRQOverride->flags >> 2) & 0x03;

            if (polarityFlags == 0x3) polarity = 1;   // 0x01 means active high, 0x03 means active low
            if (triggerFlags == 0x3) triggerMode = 1; // 0x01 means edge triggered, 0x03 means level triggered
        }

        IOAPICData* APICData = GetIOAPICFromIRQ(gsi);
        if (!APICData) {
            LOG_ERROR("No IOAPICs are set up to handle IRQ #%u8", irqNum);
            PANIC("No IOAPIC was found for the IRQ!");
        }

        uint8_t entry = gsi - APICData->gsiBase;
        uint32_t low =
            irqVector
            | (0 << 8)            // Fixed delivery mode
            | (0 << 11)           // Physical destination mode
            | (polarity << 13)    // Polarity
            | (triggerMode << 15) // Trigger mode
            | (1 << 16);          // Masked

        // High 32 bits: destination LAPIC ID in bits 56-59 (bits 24-27 of the high register)
        uint32_t high = ((_LAPICID & 0xFF) << 24);

        WriteIOAPICRegister(APICData->base, 0x10 + (entry * 2), low);
        WriteIOAPICRegister(APICData->base, 0x11 + (entry * 2), high);
    }

    // NOTE: This APIC init is uniprocessor only, it doesn't yet initialize other per-core APIC chips
    void APIC::Initialize() {
        // Check if this system has APIC
        if (!_cpu->HasFeature(Core::CPUFeatures::APIC)) {
            PANIC("This system does not support APIC!");
        }

        // Disable interrupts until APIC is fully initialized
        asm volatile ("cli");

        // --- LAPIC ---
        // Extract the physical APIC base address (bits 12 to 35)
        uint64_t APICBaseIA32 = _cpu->ReadMSR(MSR_IA32_APIC_BASE);
        uint64_t MMIOLAPICPhysAddr = APICBaseIA32 & 0xFFFFFFFFFFFFF000ULL;
        MMIOLAPICAddr = reinterpret_cast<volatile uint32_t*>(MMIOLAPICPhysAddr);
        LOG_DEBUG("MMIO LAPIC address is %p.", MMIOLAPICAddr);

        // Check if the system supports x2APIC (bit 10) and that APIC is globally enabled (bit 11)
        // NOTE: Set up xAPIC if/when it becomes necessary
        if (APICBaseIA32 & (1ULL << 10)) PANIC("x2APIC mode is enabled, MMIO is not possible!");
        if ((APICBaseIA32 & (1ULL << 11)) == 0) PANIC("APIC is globally disabled!");

        // Map the LAPIC MMIO region
        if (MMIOLAPICPhysAddr % Architecture::KernelPageSize != 0) PANIC("LAPIC base address is not page aligned!");
        _paging->MapPage(
            MMIOLAPICPhysAddr,
            MMIOLAPICPhysAddr,
            Memory::PageFlags::ReadWrite | Memory::PageFlags::NoExecute | Memory::PageFlags::CacheDisable
        );        

        // Find the MADT and LAPIC ID
        _madt = (Core::ACPI::MADT*)_acpi->GetTable("APIC");
        if (!_madt) PANIC("Failed to find the MADT!");
        _LAPICID = (ReadLAPICRegister(LAPIC_ID_REG_OFFSET) >> 24) & 0xFF;

        // Now we need to configure the Spurious Interrupt Vector Register
        WriteLAPICRegister(SPIRV_REG_OFFSET, SPIRV_VECTOR | (1 << 8));
        if ((ReadLAPICRegister(SPIRV_REG_OFFSET) & (1 << 8)) == 0) PANIC("Failed to configure LAPIC spurious vector register, bit 8 (software enable) of LAPIC SPIRV is not set!");

        // We should mask all of the Local Vector Table entries to put the LAPIC into a controlled state, this disables interrupts during initialization
        MaskLVTEntry(LVT_TIMER_OFFSET);
        MaskLVTEntry(LVT_LINT0_OFFSET);
        MaskLVTEntry(LVT_LINT1_OFFSET);
        MaskLVTEntry(LVT_ERROR_OFFSET);

        // The 8259 PIC chip MUST be disabled before APIC can be used
        _pic->Disable();

        // We need to clear the Error Status Register by writing to it *TWICE*. After this we can issue an initial End Of Interrupt by writing 0x0 to offset 0xB0
        WriteLAPICRegister(ERROR_STATUS_REG_OFFSET, 0x00);
        WriteLAPICRegister(ERROR_STATUS_REG_OFFSET, 0x00);
        WriteLAPICRegister(EOI_REG_OFFSET, 0x00);

        // Now we can set up a vector for the LVT timer, we'll use 0x40 to avoid IRQ conflicts with vectors 0x21-0x2F
        WriteLAPICRegister(LVT_TIMER_OFFSET, 
            LVT_VECTOR
            | (0 << 8)  // Fixed delivery
            | (0 << 16) // Unmasked
            | (1 << 17) // Periodic mode
        );

        // We set the division configuration register to 16, and the initial count register to 10^7; this triggers a very fast interrupt
        // NOTE: The DivConf register does not take a literal integer divisor, it uses a specific encoding
        //      see figure 11-10 in section 11.5.4 for the correct values (Intel 64 and IA-32 Software Developer's Manual, Volume 3A, page 401)
        WriteLAPICRegister(DIVIDE_CONFIG_REG_OFFSET, 0b011);
        WriteLAPICRegister(INITIAL_COUNT_REG_OFFSET, 10000000);

        // Finally, set the task priority register (TPR) to 0 so no interrupts are blocked
        // NOTE: Interrupts below <TPR value> are blocked, so we set it to 0 becasue there are no interrupts less than 0
        WriteLAPICRegister(TPR_REG_OFFSET, MINIMUM_IRQ_NUM);



        // --- IOAPIC ---
        // Search the MADT's entries to find any IOAPIC (0x01) and ISO (0x02) entries
        uint8_t* VLRecordsPtr = (uint8_t*)_madt + MADT_VLRECORDS_OFFSET;
        uint8_t* MADTEnd = (uint8_t*)_madt + _madt->sdt.length;

        while (VLRecordsPtr < MADTEnd) {
            Core::ACPI::MADTEntryHeader* entryHeader = (Core::ACPI::MADTEntryHeader*)VLRecordsPtr;
            
            switch(entryHeader->type) {
                // IOAPIC descriptor
                case IOAPIC_ENTRY_TYPE: {
                    _IOAPICEntries.Add((Core::ACPI::MADTIOAPIC*)VLRecordsPtr);
                    break;
                }

                // IRQ Source Override (ISO)
                case IRQ_SRCOVR_ENTRY_TYPE: {
                    _IRQSrcOverrides.Add((Core::ACPI::MADTIRQSrcOverride*)VLRecordsPtr);
                    break;
                }

                // We don't care about other entry types right now
                default:
                    break;
            }

            // Advance to the next entry
            VLRecordsPtr += entryHeader->length;
        }

        uint64_t IOAPICEntryCount = _IOAPICEntries.Size();
        LOG_DEBUG("Found %u64 IOAPIC %s.", IOAPICEntryCount, IOAPICEntryCount == 1 ? "entry" : "entries");
        LOG_DEBUG("Found %u64 IRQ source override(s).", _IRQSrcOverrides.Size());

        // Now that we've found each IOAPIC, we have to initialize them all
        for (uint8_t IOAPICIndex = 0; IOAPICIndex < IOAPICEntryCount; IOAPICIndex++) {
            Core::ACPI::MADTIOAPIC* entry = _IOAPICEntries[IOAPICIndex];

            // Map the IOAPIC MMIO region
            if (entry->ioApicAddress % Architecture::KernelPageSize != 0) {
                LOG_ERROR("IOAPIC base address %p is not aligned to page size %u64!", entry->ioApicAddress, Architecture::KernelPageSize);
                PANIC("IOAPIC base address is not page aligned!");
            }

            _paging->MapPage(
                entry->ioApicAddress,
                entry->ioApicAddress,
                Memory::PageFlags::ReadWrite | Memory::PageFlags::NoExecute | Memory::PageFlags::CacheDisable
            );

            volatile uint32_t* baseAddress = reinterpret_cast<volatile uint32_t*>(entry->ioApicAddress);
            uint32_t rawIOAPICVer = ReadIOAPICRegister(baseAddress, IOAPIC_VERSION_REG_OFFSET);
            uint8_t IOAPICVersion = rawIOAPICVer & 0xFF;
            uint8_t redirectionEntryCount = ((rawIOAPICVer >> 16) & 0xFF) + 1;

            // Mask all redirection entries
            for (uint8_t redirEntryIndex = 0; redirEntryIndex < redirectionEntryCount; redirEntryIndex++) {
                WriteIOAPICRegister(baseAddress, 0x10 + (redirEntryIndex * 2), 1 << 16); // Mask
                WriteIOAPICRegister(baseAddress, 0x11 + (redirEntryIndex * 2), 0);       // Clear high bits
            }

            // Finally, store the data for this IOAPIC in the list
            IOAPICData* data = new IOAPICData();
            data->base = baseAddress;
            data->gsiBase = entry->globalSystemInterruptBase;
            data->redirectionEntryCount = redirectionEntryCount;

            _IOAPICs.Add(data);
        }



        // --- IRQ SETUP ---
        // Map the first 16 IRQs, these are 1:1 from the legacy PIC IRQs we chose already
        // NOTE: Drivers and other programs are responsible for setting up IRQ handlers and unmasking IRQs
        for (uint8_t irqNum = MINIMUM_IRQ_NUM; irqNum < 16; irqNum++) {
            MapIRQ(irqNum, IRQ_OFFSET + irqNum);
        }

        // Tell the IDT that IRQs are handled via the APIC now and not the legacy PIC
        _idt->SetInterruptController(this);

        // Finally, enable interrupts again so this whole init process has a purpose and isn't here for shits and giggles
        asm volatile ("sti");
    }
}
