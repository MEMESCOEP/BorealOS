#include <Definitions.h>
#include "ACPI.h"
#include "Utility/SerialOperations.h"
#include "Utility/StringTools.h"
#include "Boot/multiboot.h"
#include "Boot/MBParser.h"
#include "Core/Kernel.h"

#define ACPI_PWR_MGMT_ENABLE_TIMEOUT 1000000

ACPIState KernelACPI = {};
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
    KernelACPI.Initialized = false;

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

    LOG(LOG_INFO, "ACPI SDP found!\n");
    PRINTF("\t* *SDP address is 0x%x\n", (uint64_t)(uintptr_t)SDPAddr);
    PRINTF("\t* ACPI %s structs will be used.\n\n", useNewACPI == true ? "2.0+" : "1.0");

    // We need to set up the correct *SDT table
    if (useNewACPI == true)
    {
        KernelACPI.XSDP = (XSDP_t*)SDPAddr;
        LOG(LOG_INFO, "Validating XSDP table...\n");
        
        if (ValidateXSDPTable(KernelACPI.XSDP) == false)
        {
            LOG(LOG_ERROR, "XSDP table checksum failed!");
            return STATUS_FAILURE;
        }

        // Now we need to validate both the RSDT and XSDT
        PRINTF("\t* RSDT address is 0x%x.\n", (uint64_t)(uintptr_t)KernelACPI.XSDP->RsdtAddress);
        PRINTF("\t* XSDT address is 0x%x.\n\n", (uint64_t)(uintptr_t)KernelACPI.XSDP->XsdtAddress);
        KernelACPI.RSDT = (RSDT_t*)KernelACPI.XSDP->RsdtAddress;
        KernelACPI.XSDT = (XSDT_t*)(uintptr_t)KernelACPI.XSDP->XsdtAddress;
    }
    else
    {
        // Cast and validate the RSDP table
        KernelACPI.RSDP = (RSDP_t*)SDPAddr;
        LOG(LOG_INFO, "Validating RSDP table...\n");

        if (ValidateSDPSDTChecksum(KernelACPI.RSDP, RSDP_TABLE_LEN) == false)
        {
            LOG(LOG_ERROR, "RSDP table checksum failed!");
            return STATUS_FAILURE;
        }
        
        // Now we need to validate the RSDT
        PRINTF("\t* RSDT address is 0x%x.\n\n", (uint64_t)(uintptr_t)KernelACPI.RSDP->RsdtAddress);
        KernelACPI.RSDT = (RSDT_t*)KernelACPI.RSDP->RsdtAddress;
        LOG(LOG_INFO, "Validating RSDT...\n");

        if (ValidateSDPSDTChecksum(KernelACPI.RSDT, KernelACPI.RSDT->SDTHeader.Length) == false)
        {
            LOG(LOG_ERROR, "RSDT table checksum failed!");
            return STATUS_FAILURE;
        }
    }

    // Find the FADT
    LOG(LOG_INFO, "Finding FADT...\n");
    KernelACPI.FADT = FindFADT(KernelACPI.RSDT);

    if (!KernelACPI.FADT)
    {
        LOG(LOG_WARNING, "Failed to get ACPI FADT address!\n");
        return STATUS_FAILURE;
    }

    PRINTF("\t* FADT address is 0x%x.\n\n", (uint64_t)(uintptr_t)KernelACPI.FADT);

    // Enable ACPI power management mode if we can
    if (KernelACPI.FADT->SMI_CommandPort != 0 && KernelACPI.FADT->AcpiEnable != 0)
    {
        LOG(LOG_INFO, "Enabling ACPI power management events...\n");
        outb(KernelACPI.FADT->SMI_CommandPort, KernelACPI.FADT->AcpiEnable);

        // Wait for ACPI to be enabled by polling the PM1aControlBlock register
        int Timeout = ACPI_PWR_MGMT_ENABLE_TIMEOUT;
        while (true)
        {
            if (Timeout <= 0)
            {
                LOG(LOG_ERROR, "Failed to enable ACPI power management events!\n");
                return STATUS_TIMEOUT;
            }

            if (inw(KernelACPI.FADT->PM1aControlBlock) & (1 << 0))
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
    if (KernelACPI.FADT->PreferredPowerManagementProfile < 8)
    {
        PRINTF("\t* Device preferred power management profile is \"%s\" (profile #%u)\n\n", PowerProfileStrings[KernelACPI.FADT->PreferredPowerManagementProfile], KernelACPI.FADT->PreferredPowerManagementProfile);
        KernelACPI.ValidPowerMGMTMode = true;
    }
    else
    {
        LOGF(LOG_WARNING, "Profile #%u is not a valid power management profile, values must be 0-7.\n", KernelACPI.FADT->PreferredPowerManagementProfile);
        KernelACPI.ValidPowerMGMTMode = false;
    }
    
    KernelACPI.Initialized = true;
    return STATUS_SUCCESS;
}

int ACPIGetRevision() {
    return useNewACPI ? (int)KernelACPI.XSDP->Revision : (int)KernelACPI.RSDP->Revision;
}

void MapRegion(uintptr_t address, size_t length) {
    uint32_t begin = ALIGN_DOWN(address, PMM_PAGE_SIZE);
    uint32_t end = ALIGN_UP(address + length, PMM_PAGE_SIZE);
    size_t numPages = (end - begin) / PMM_PAGE_SIZE;
    for (size_t i = 0; i < numPages; i++) {
        PagingMapPage(&Kernel.Paging, (void*)(begin + i * PMM_PAGE_SIZE), (void*)(begin + i * PMM_PAGE_SIZE), true, false, 0);
    }
}

void ACPIMapTables() {
    // Make sure all the ACPI tables are mapped into the kernel's virtual address space
    if (!KernelACPI.Initialized) {
        LOG(LOG_WARNING, "ACPI not initialized, cannot map tables!\n");
        return;
    }

    if (useNewACPI) {
        uint32_t xsdtLength = KernelACPI.XSDT->SDTHeader.Length;
        MapRegion((uintptr_t)KernelACPI.XSDT, xsdtLength);
        uint32_t xsdpLength = KernelACPI.XSDP->Length;
        MapRegion((uintptr_t)KernelACPI.XSDP, xsdpLength);

        // Now map every item in the XSDT
        int entries = (KernelACPI.XSDT->SDTHeader.Length - sizeof(KernelACPI.XSDT->SDTHeader)) / 8;
        for (int i = 0; i < entries; i++) {
            ACPISDTHeader* SDTHeader = (ACPISDTHeader*) (uint32_t)KernelACPI.XSDT->PointerToOtherSDT[i]; // WILDLY UNSAFE CAST
            MapRegion((uintptr_t)SDTHeader, SDTHeader->Length);
        }
    } else {
        uint32_t rsdtLength = KernelACPI.RSDT->SDTHeader.Length;
        MapRegion((uintptr_t)KernelACPI.RSDT, rsdtLength);
        uint32_t rsdpLength = RSDP_TABLE_LEN;
        MapRegion((uintptr_t)KernelACPI.RSDP, rsdpLength);

        // Now map every item in the RSDT
        int entries = (KernelACPI.RSDT->SDTHeader.Length - sizeof(KernelACPI.RSDT->SDTHeader)) / 4;
        for (int i = 0; i < entries; i++) {
            ACPISDTHeader* SDTHeader = (ACPISDTHeader*) KernelACPI.RSDT->PointerToOtherSDT[i];
            MapRegion((uintptr_t)SDTHeader, SDTHeader->Length);
        }
    }

    // Map the FADT
    MapRegion((uintptr_t)KernelACPI.FADT, KernelACPI.FADT->SDTHeader.Length);

    // Get the size of the DSDT and map it in
    if (KernelACPI.FADT->Dsdt) {
        ACPISDTHeader* DSDTHeader = (ACPISDTHeader*)(uintptr_t)KernelACPI.FADT->Dsdt;
        MapRegion((uintptr_t)DSDTHeader, DSDTHeader->Length);
    }
}

Status ACPIGetTableBySignature(const char *signature, size_t sigLen, void **outTable) {
    if (!KernelACPI.Initialized || !outTable || sigLen == 0 || sigLen > 4) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!strncmp(signature, "DSDT", sigLen)) {
        // DSDT is a special case, it's pointed to by the FADT
        if (!KernelACPI.FADT) {
            return STATUS_FAILURE;
        }

        *outTable = (void*)(uintptr_t)KernelACPI.FADT->Dsdt;
        return STATUS_SUCCESS;
    }

    if (useNewACPI) {
        int entries = (KernelACPI.XSDT->SDTHeader.Length - sizeof(KernelACPI.XSDT->SDTHeader)) / 8;
        for (int i = 0; i < entries; i++) {
            ACPISDTHeader* SDTHeader = (ACPISDTHeader*) (uint32_t)KernelACPI.XSDT->PointerToOtherSDT[i]; // WILDLY UNSAFE CAST
            if (!strncmp(SDTHeader->Signature, signature, sigLen)) {
                *outTable = SDTHeader; return STATUS_SUCCESS;
            }
        }
    } else {
        int entries = (KernelACPI.RSDT->SDTHeader.Length - sizeof(KernelACPI.RSDT->SDTHeader)) / 4;
        for (int i = 0; i < entries; i++) {
            ACPISDTHeader* SDTHeader = (ACPISDTHeader*) KernelACPI.RSDT->PointerToOtherSDT[i];
            if (!strncmp(SDTHeader->Signature, signature, sigLen)) {
                *outTable = SDTHeader; return STATUS_SUCCESS;
            }
        }
    }
    return STATUS_FAILURE;
}
