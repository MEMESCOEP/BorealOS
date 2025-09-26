#include <Definitions.h>
#include "ACPI.h"
#include "Utility/SerialOperations.h"
#include "Utility/StringTools.h"
#include "Boot/multiboot.h"
#include "Boot/MBParser.h"
#include "Core/Kernel.h"

#define ACPI_PWR_MGMT_ENABLE_TIMEOUT 1000000

ACPIState ACPI = {};
const char* PowerProfileStrings[8] = {"Unspecified", "Desktop", "Mobile", "Workstation", "Enterprise Server", "SOHO Server", "Appliance PC", "Performance Server"};
void* SDPAddr = NULL;
bool useNewACPI = false;

FADT_t* FindFADT(RSDT_t* RSDT)
{
    int entries = (RSDT->SDTHeader.Length - sizeof(RSDT->SDTHeader)) / 4;

    for (int i = 0; i < entries; i++)
    {
        ACPISDTHeader* SDTHeader = (ACPISDTHeader*) RSDT->PointerToOtherSDT[i];

        if (!strncmp(SDTHeader->Signature, "FACP", 4))
        {
            return (FADT_t*)SDTHeader;
        }
    }

    // No FADT was found
    return NULL;
}

bool ValidateSDPSDTChecksum(void* table, size_t length) {
    // The sum of all the fields should be zero
    uint8_t* bytes = (uint8_t*)table;
    uint8_t sum = 0;

    for (size_t i = 0; i < length; i++)
    {
        sum += bytes[i];
    }

    return sum == 0;
}

bool ValidateXSDPTable(XSDP_t* xsdp) {
    // Check the first 20 bytes as if it were an RSDP struct
    if (!ValidateSDPSDTChecksum(xsdp, RSDP_TABLE_LEN))
    {
        return false;
    }

    // Now we have to check the entire extended structure
    if (!ValidateSDPSDTChecksum(xsdp, xsdp->Length)) {
        return false;
    }

    return true;
}

Status ACPIInit(uint32_t MB2InfoPtr) {
    ACPI.Initialized = false;

    // Attempt to get the *SDP via ACPI 2.0+; if this fails, try again with ACPI 1.0
    struct multiboot_tag_new_acpi* newACPITag = (struct multiboot_tag_new_acpi*)MBGetTag(MB2InfoPtr, MULTIBOOT_TAG_TYPE_ACPI_NEW);
    if (newACPITag)
    {
        SDPAddr = (void*)newACPITag->rsdp;
        useNewACPI = true;
    }
    else
    {
        // The tag for ACPI 2.0+ wasn't found, we should try looking for the ACPI 1.0 tag
        struct multiboot_tag_old_acpi* oldACPITag = (struct multiboot_tag_old_acpi*)MBGetTag(MB2InfoPtr, MULTIBOOT_TAG_TYPE_ACPI_OLD);

        if (oldACPITag)
        {
            SDPAddr = (void*)oldACPITag->rsdp;
        }
    }

    // Make sure we got an SDP address
    if (!SDPAddr)
    {
        LOG(LOG_WARNING, "Failed to get ACPI *SDP address, ACPI initialization cannot continue.\n");
        return STATUS_FAILURE;
    }

    // Make sure the address we got is valid
    if ((uintptr_t)SDPAddr >= RSDP_SEARCH_REGION_START && (uintptr_t)SDPAddr <= RSDP_SEARCH_REGION_END)
    {
        LOGF(LOG_WARNING, "ACPI *SDP address 0x%x is out of range (0x%x-0x%x)!\n", (uint64_t)(uintptr_t)SDPAddr, (uint64_t)RSDP_SEARCH_REGION_START, (uint64_t)RSDP_SEARCH_REGION_END);
        return STATUS_FAILURE;
    }

    LOGF(LOG_INFO, "*SDP address is 0x%x\n", (uint64_t)(uintptr_t)SDPAddr);
    LOGF(LOG_INFO, "ACPI %s structs will be used.\n", useNewACPI == true ? "2.0+" : "1.0");

    // We need to set up the correct *SDT table
    if (useNewACPI == true)
    {
        ACPI.XSDP = (XSDP_t*)SDPAddr;
        LOG(LOG_INFO, "Validating XSDP table...\n");
        
        if (ValidateXSDPTable(ACPI.XSDP) == false)
        {
            LOG(LOG_ERROR, "XSDP table checksum failed!");
            return STATUS_FAILURE;
        }

        // Now we need to validate both the RSDT and XSDT
        LOGF(LOG_INFO, "RSDT address is 0x%x.\n", (uint64_t)(uintptr_t)ACPI.XSDP->RsdtAddress);
        LOGF(LOG_INFO, "XSDT address is 0x%x.\n", (uint64_t)(uintptr_t)ACPI.XSDP->XsdtAddress);
        ACPI.RSDT = (RSDT_t*)ACPI.XSDP->RsdtAddress;
        ACPI.XSDT = (XSDT_t*)(uintptr_t)ACPI.XSDP->XsdtAddress;
    }
    else
    {
        // Cast and validate the RSDP table
        ACPI.RSDP = (RSDP_t*)SDPAddr;
        LOG(LOG_INFO, "Validating RSDP table...\n");

        if (ValidateSDPSDTChecksum(ACPI.RSDP, RSDP_TABLE_LEN) == false)
        {
            LOG(LOG_ERROR, "RSDP table checksum failed!");
            return STATUS_FAILURE;
        }
        
        // Now we need to validate the RSDT
        LOGF(LOG_INFO, "RSDT address is 0x%x.\n", (uint64_t)(uintptr_t)ACPI.RSDP->RsdtAddress);
        ACPI.RSDT = (RSDT_t*)ACPI.RSDP->RsdtAddress;
        LOG(LOG_INFO, "Validating RSDT...\n");

        if (ValidateSDPSDTChecksum(ACPI.RSDT, ACPI.RSDT->SDTHeader.Length) == false)
        {
            LOG(LOG_ERROR, "RSDT table checksum failed!");
            return STATUS_FAILURE;
        }
    }

    // Find the FADT
    ACPI.FADT = FindFADT(ACPI.RSDT);

    if (!ACPI.FADT)
    {
        LOG(LOG_WARNING, "Failed to get ACPI FADT address!\n");
        return STATUS_FAILURE;
    }

    LOGF(LOG_INFO, "FADT address is 0x%x.\n", (uint64_t)(uintptr_t)ACPI.FADT);

    // Enable ACPI power management mode if we can
    if (ACPI.FADT->SMI_CommandPort != 0 && ACPI.FADT->AcpiEnable != 0)
    {
        LOG(LOG_INFO, "Enabling ACPI power management events...\n");
        outb(ACPI.FADT->SMI_CommandPort, ACPI.FADT->AcpiEnable);

        // Wait for ACPI to be enabled by polling the PM1aControlBlock register
        int Timeout = ACPI_PWR_MGMT_ENABLE_TIMEOUT;
        while (true)
        {
            if (Timeout <= 0)
            {
                LOG(LOG_ERROR, "Failed to enable ACPI power management events!\n");
                return STATUS_TIMEOUT;
            }

            if (inw(ACPI.FADT->PM1aControlBlock) & (1 << 0))
            {
                break;
            }

            Timeout--;
        }
    }
    else
    {
        LOG(LOG_INFO, "ACPI power management events are either unsupported or are already enabled.\n");
    }

    // We should store the power profile for later
    if (ACPI.FADT->PreferredPowerManagementProfile < 8)
    {
        LOGF(LOG_INFO, "Device preferred power management profile is \"%s\" (profile #%u)\n", PowerProfileStrings[ACPI.FADT->PreferredPowerManagementProfile], ACPI.FADT->PreferredPowerManagementProfile);
        ACPI.ValidPowerMGMTMode = true;
    }
    else
    {
        LOGF(LOG_WARNING, "Profile #%u is not a valid power management profile, values must be 0-7.\n", ACPI.FADT->PreferredPowerManagementProfile);
        ACPI.ValidPowerMGMTMode = false;
    }
    
    ACPI.Initialized = true;
    return STATUS_SUCCESS;
}