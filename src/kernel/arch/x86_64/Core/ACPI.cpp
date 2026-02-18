#include "ACPI.h"

namespace Core {
    void TrimTrailingSpaces(char* str, uint64_t strLength) {
        // Null terminate the string
        str[strLength] = '\0';

        // Check for non-space characters, starting from the end of the string
        for (int charIndex = strLength - 1; charIndex >= 0; charIndex--) {
            if (str[charIndex] != ' ') {
                str[charIndex + 1] = '\0';
                return;
            }
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

    void ACPI::Initialize() {
        char OEMID[7];
        char Signature[9];
        RSDP* rsdp;
        XSDP* xsdp;

        // Check if limine provided a response for the RSDP table; we'll determine the exact ACPI revision later
        if (!rsdp_request.response) {
            LOG_WARNING("Limine did not provide a response for the RSDP table, ACPI is likely not supported!");
            return;
        }

        // Get the response
        RSDPResponse = rsdp_request.response;
        rsdp = (RSDP*)RSDPResponse->address;
        LOG_DEBUG("_SDP table was found at address %p, with early revision %u64 (%s).", RSDPResponse->address, rsdp->revision, ACPI::SDPRevisionStrings[rsdp->revision]);

        // Now that we have the SDP, we need to verify its checksum based on its type
        if (rsdp->revision > 0) {
            xsdp = (XSDP*)RSDPResponse->address;

            if (!ValidateXSDP(xsdp)) {
                LOG_WARNING("XSDP checksum is invalid!");
                return;
            }
        }
        else if (!ValidateRSDP(rsdp)) {
            LOG_WARNING("RSDP checksum is invalid!");
            return;
        }

        // Get the OEMID and signature and trim trailing spaces
        memcpy(OEMID, rsdp->OEMID, 6);
        memcpy(Signature, rsdp->signature, 8);
        TrimTrailingSpaces(OEMID, 6);
        TrimTrailingSpaces(Signature, 8);

        LOG_DEBUG("_SDP Signature: \"%s\"", Signature);
        LOG_DEBUG("_SDP OEMID: \"%s\"", OEMID);

        // Now find the *SDT
        bool SDTType = rsdp->revision > 0;
        const char* SDTStr = SDTType ? "XSDT" : "RSDT";
        uint64_t sdtAddrPhysical = (rsdp->revision > 0) ? xsdp->XSDTAddress : rsdp->RSDTAddress;
        uint64_t sdtAddrVirtual = sdtAddrPhysical + hhdm_request.response->offset;
        SDTHeader* sdt = reinterpret_cast<SDTHeader*>(sdtAddrVirtual);

        LOG_DEBUG("%s address: %p (offset from physical address %p)", SDTStr, sdtAddrVirtual, sdtAddrPhysical);

        // Verify the SDT
        if (!ValidateSDT(sdt)) {
            LOG_WARNING("%s checksum failed!", SDTStr);
            return;
        }

        // Find the FACP
        void* facp = FindFACP(sdt);
        if (!facp) {
            LOG_WARNING("Failed to find a FACP entry!");
            return;
        }

        LOG_DEBUG("FACP address: %p", facp);
    }
}