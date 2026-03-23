#include "ACPI.h"
#include "../../IO/Serial.h"
#include "lai_include.h"

namespace Core::Firmware {
    void ACPI::WriteByteCommand(uint8_t command) {
        if (!(IO::Serial::inw(_fadt->PM1aControlBlock) & 1)) {
            IO::Serial::outb(_fadt->SMI_CommandPort, command);

            while (!(IO::Serial::inw(_fadt->PM1aControlBlock) & 1));
        }
    }

    void* ACPI::FindFACP(void* rootSDT) {
        SDTHeader* header = reinterpret_cast<SDTHeader*>(rootSDT);
        size_t entryCount = 0;
        bool isXSDT = false;

        size_t entrySize = (header->length - sizeof(SDTHeader)) / 8;
        if (entrySize * 8 + sizeof(SDTHeader) == header->length) {
            isXSDT = true;
            entryCount = entrySize;
        }
        else {
            entryCount = (header->length - sizeof(SDTHeader)) / 4;
        }

        for (size_t i = 0; i < entryCount; i++) {
            SDTHeader* table = nullptr;
            if (isXSDT) {
                uint64_t* entries = reinterpret_cast<uint64_t*>(
                    reinterpret_cast<char*>(header) + sizeof(SDTHeader)
                );

                table = reinterpret_cast<SDTHeader*>(entries[i] + hhdm_request.response->offset);
            }
            else {
                uint32_t* entries = reinterpret_cast<uint32_t*>(
                    reinterpret_cast<char*>(header) + sizeof(SDTHeader)
                );

                table = reinterpret_cast<SDTHeader*>(entries[i] + hhdm_request.response->offset);
            }

            if (memcmp(table->signature, "FACP", 4) == 0)
                return table;
        }

        return nullptr;
    }

    bool ACPI::ValidateRSDP(RSDP* rsdp) {
        if (!rsdp) return false;

        uint8_t sum = 0;
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(rsdp);
        
        for (int i = 0; i < 20; i++) {
            sum += bytes[i];
        }

        if (sum != 0) return false;
        return true;
    }

    bool ACPI::ValidateXSDP(XSDP* xsdp) {
        if (!xsdp) return false;
        if (!ValidateRSDP(&xsdp->rsdp)) return false;

        // ACPI 2.0+ requires length to be at least the size of the XSDP struct
        if (xsdp->length < sizeof(XSDP)) return false;

        // Compute the extended checksum over the full table
        uint8_t sum = 0;
        uint8_t* bytes = reinterpret_cast<uint8_t*>(xsdp);

        for (uint32_t i = 0; i < xsdp->length; i++) {
            sum += bytes[i];
        }

        if (sum != 0) return false;
        return true;
    }

    bool ACPI::ValidateSDT(SDTHeader* sdt) {
        unsigned char sum = 0;

        for (uint64_t i = 0; i < sdt->length; i++)
        {
            sum += ((char *) sdt)[i];
        }

        return sum == 0;
    }

    bool ACPI::ACPISupported() { return _systemHasACPI; }

    void ACPI::Initialize() {
        char OEMID[7];
        char Signature[9];

        // Check if limine provided a response for the RSDP table; we'll determine the exact ACPI revision later
        if (!rsdp_request.response) {
            PANIC("Limine did not provide a response for the RSDP table, ACPI is likely not supported!");
        }

        // Get the response
        _rsdp_response = rsdp_request.response;
        _rsdp = (RSDP*)_rsdp_response->address;
        LOG_DEBUG("_SDP table was found at address %p, with early revision %u64 (%s).", _rsdp_response->address, _rsdp->revision, ACPI::SDPRevisionStrings[_rsdp->revision]);

        // Now that we have the SDP, we need to verify its checksum based on its type
        if (_rsdp->revision > 0) {
            _xsdp = (XSDP*)_rsdp_response->address;

            if (!ValidateXSDP(_xsdp)) {
                LOG_WARNING("XSDP checksum is invalid!");
                return;
            }

            _xsdt = reinterpret_cast<XSDT*>(_xsdp->XSDTAddress + hhdm_request.response->offset);
        }
        else {
            _rsdt = reinterpret_cast<RSDT*>(_rsdp->RSDTAddress + hhdm_request.response->offset);

            if (!ValidateSDT(&_rsdt->sdt)) {
                LOG_WARNING("RSDT checksum is invalid!");
                return;
            }
        }

        // Get the OEMID and signature and trim trailing spaces
        memcpy(OEMID, _rsdp->OEMID, 6);
        memcpy(Signature, _rsdp->signature, 8);
        Utility::StringFormatter::TrimTrailingSpaces(OEMID, 6);
        Utility::StringFormatter::TrimTrailingSpaces(Signature, 8);

        LOG_DEBUG("_SDP Signature: \"%s\"", Signature);
        LOG_DEBUG("_SDP OEMID: \"%s\"", OEMID);

        // Now find the *SDT
        bool SDTType = _rsdp->revision > 0;
        const char* SDTStr = SDTType ? "XSDT" : "RSDT";
        uint64_t sdtAddrPhysical = (_rsdp->revision > 0) ? _xsdp->XSDTAddress : _rsdp->RSDTAddress;
        uint64_t sdtAddrVirtual = sdtAddrPhysical + hhdm_request.response->offset;
        SDTHeader* sdt = reinterpret_cast<SDTHeader*>(sdtAddrVirtual);

        LOG_DEBUG("%s address: %p (offset from physical address %p)", SDTStr, sdtAddrVirtual, sdtAddrPhysical);

        // Verify the SDT
        if (!ValidateSDT(sdt)) {
            LOG_WARNING("%s checksum failed!", SDTStr);
            return;
        }

        // Find the FACP
        _facp = FindFACP(sdt);
        if (!_facp) {
            LOG_WARNING("Failed to find a FACP entry!");
            return;
        }

        LOG_DEBUG("FACP address: %p (offset from physical address %p)", _facp, (uintptr_t)_facp - hhdm_request.response->offset);

        // Get the FADT
        _fadt = (FADT*)_facp;
        LOG_DEBUG("FADT address: %p (offset from physical address %p)", _fadt, (uintptr_t)_fadt - hhdm_request.response->offset);

        // Retrieve the DSDT
        uint64_t DSDTAddrPhysical = 0;

        if (_fadt->sdt.length >= offsetof(FADT, X_Dsdt) + sizeof(uint64_t) && _fadt->X_Dsdt != 0) DSDTAddrPhysical = _fadt->X_Dsdt;
        else DSDTAddrPhysical = _fadt->Dsdt;

        if (!DSDTAddrPhysical) {
            LOG_WARNING("Failed to find the DSDT!");
            return;
        }

        _dsdt = (void*)(DSDTAddrPhysical + hhdm_request.response->offset);
        LOG_DEBUG("DSDT address: %p (offset from physical address %p)", _dsdt, DSDTAddrPhysical);

        // Enable ACPI mode
        LOG_DEBUG("Writing 0x%x8 to PM1a control block (address is %p) to enable ACPI...", _fadt->AcpiEnable, _fadt->PM1aControlBlock);
        WriteByteCommand(_fadt->AcpiEnable);

        // Get the system's preferred power management profile
        PowerProfile = _fadt->PreferredPowerManagementProfile;
        if (PowerProfile <= 7) LOG_DEBUG("The device has a preferred power management profile of \"%s\" (profile ID %u8).", powerProfileStrings[PowerProfile], PowerProfile);
        else LOG_WARNING("ACPI power profile ID %u8 is invalid.", PowerProfile);
        _systemHasACPI = true;
    }

    void ACPI::LoadLAI() {
        laihost_init();

        // lai_enable_tracing(LAI_TRACE_IO | LAI_TRACE_NS | LAI_TRACE_OP); SLOW! Takes like 8 minutes to boot with this enabled on real hardware.
        lai_set_acpi_revision(_rsdp->revision);
        asm volatile("cli"); // Disable interrupts while we set up the ACPI namespace to avoid

        lai_create_namespace();
        asm volatile("sti");

        lai_enable_acpi(1);
        _laiLoaded = true;
    }

    void* ACPI::GetTable(const char* signature, uint64_t index) {
        if (strcmp(signature, "DSDT") == 0) return _dsdt;
        if (strcmp(signature, "FACP") == 0 || strcmp(signature, "FADT") == 0) return _facp;
        if (!_systemHasACPI) return nullptr;

        bool isXSDT = _rsdp->revision > 0;
        SDTHeader* header = isXSDT ? &_xsdt->sdt : &_rsdt->sdt;
        uintptr_t ptrStart = reinterpret_cast<uintptr_t>(header) + sizeof(SDTHeader);

        size_t entrySize = isXSDT ? 8 : 4;
        size_t entries = (header->length - sizeof(SDTHeader)) / entrySize;

        for (size_t i = 0; i < entries; i++) {
            uint64_t targetAddr = 0;
            if (isXSDT) {
                targetAddr = reinterpret_cast<uint64_t*>(ptrStart)[i];
            } else {
                targetAddr = reinterpret_cast<uint32_t*>(ptrStart)[i];
            }

            auto table = reinterpret_cast<SDTHeader*>(targetAddr + hhdm_request.response->offset);
            if (memcmp(table->signature, signature, 4) == 0) {
                if (index == 0) return table;
                index--;
            }
        }

        return nullptr;
    }
}
