#include <Definitions.h>
#include "ACPI.h"

#include <lai/core.h>
#include <lai/helpers/sci.h>

#include "Utility/SerialOperations.h"
#include "Utility/StringTools.h"
#include "Boot/multiboot.h"
#include "Boot/MBParser.h"
#include "Core/Kernel.h"
#include "Drivers/CPU.h"

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

void MapRegion(uintptr_t physAddr, size_t length) {
    // Map a physical region into the ACPI paging state
    for (uintptr_t addr = ALIGN_DOWN(physAddr, PMM_PAGE_SIZE); addr < ALIGN_UP(physAddr + length, PMM_PAGE_SIZE); addr += PMM_PAGE_SIZE) {
        if (PagingMapPage(&Kernel.Paging, (void*)addr, (void*)addr, true, false, 0) != STATUS_SUCCESS) {
            LOGF(LOG_ERROR, "Failed to map ACPI table page at %p!\n", addr);
        }
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

        LOGF(LOG_DEBUG, "Mapping XSDT at address %p with length %u\n", (void*)(uintptr_t)KernelACPI.XSDT, xsdtLength);
        LOGF(LOG_DEBUG, "Mapping XSDP at address %p with length %u\n", (void*)(uintptr_t)KernelACPI.XSDP, xsdpLength);

        // Now map every item in the XSDT
        int entries = (KernelACPI.XSDT->SDTHeader.Length - sizeof(KernelACPI.XSDT->SDTHeader)) / 8;
        for (int i = 0; i < entries; i++) {
            ACPISDTHeader* SDTHeader = (ACPISDTHeader*) (uint32_t)KernelACPI.XSDT->PointerToOtherSDT[i]; // WILDLY UNSAFE CAST
            char safe[5] = {0};
            memcpy(safe, SDTHeader->Signature, 4);
            LOGF(LOG_DEBUG, "Mapping ACPI table with signature %s at address %p\n", safe, (void*)(uintptr_t)SDTHeader);
            MapRegion((uintptr_t)SDTHeader, SDTHeader->Length);
        }
    } else {
        uint32_t rsdtLength = KernelACPI.RSDT->SDTHeader.Length;
        MapRegion((uintptr_t)KernelACPI.RSDT, rsdtLength);
        uint32_t rsdpLength = RSDP_TABLE_LEN;
        MapRegion((uintptr_t)KernelACPI.RSDP, rsdpLength);

        LOGF(LOG_DEBUG, "Mapping RSDT at address %p with length %u\n", (void*)(uintptr_t)KernelACPI.RSDT, rsdtLength);
        LOGF(LOG_DEBUG, "Mapping RSDP at address %p with length %u\n", (void*)(uintptr_t)KernelACPI.RSDP, rsdpLength);

        // Now map every item in the RSDT
        int entries = (KernelACPI.RSDT->SDTHeader.Length - sizeof(KernelACPI.RSDT->SDTHeader)) / 4;
        for (int i = 0; i < entries; i++) {
            ACPISDTHeader* SDTHeader = (ACPISDTHeader*) KernelACPI.RSDT->PointerToOtherSDT[i];
            char safe[5] = {0};
            memcpy(safe, SDTHeader->Signature, 4);
            LOGF(LOG_DEBUG, "Mapping ACPI table with signature %s at address %p\n", safe, (void*)(uintptr_t)SDTHeader);
            MapRegion((uintptr_t)SDTHeader, SDTHeader->Length);
        }
    }

    // Map the FADT
    MapRegion((uintptr_t)KernelACPI.FADT, KernelACPI.FADT->SDTHeader.Length);

    // Get the size of the DSDT and map it in
    if (KernelACPI.FADT->Dsdt) {
        ACPISDTHeader* DSDTHeader = (ACPISDTHeader*)(uintptr_t)KernelACPI.FADT->Dsdt;
        LOGF(LOG_DEBUG, "Mapping DSDT at address %p with length %u\n", (void*)(uintptr_t)DSDTHeader, DSDTHeader->Length);
        MapRegion((uintptr_t)DSDTHeader, DSDTHeader->Length);
    }
}

static void SCIInterrupt(uint8_t irqNumber, RegisterState* state) {
    (void)state;
    // Handle ACPI SCI interrupts later, for now just acknowledge
    LOGF(LOG_DEBUG, "Received ACPI SCI interrupt on IRQ %u\n", irqNumber);
}

Status ACPIInitLAI() {
    // lai_enable_tracing(LAI_TRACE_OP | LAI_TRACE_IO | LAI_TRACE_NS);
    lai_set_acpi_revision(KernelACPI.RSDP->Revision);
    ASM ("cli"); // Disable interrupts for safety.
    lai_create_namespace();
    ASM ("sti"); // Re-enable interrupts.

    IDTSetIRQHandler(KernelACPI.FADT->SCI_Interrupt, SCIInterrupt);

    // lai_enable_acpi(0); TODO: enable after creating PCI.
    // We can now do stuff like sleeping!

    // THIS TOOK LIKE 2 WEEKS OF TRAIL AND ERROR...
    // BUT FINALLY! IT WORKS ON QEMU!

    return STATUS_SUCCESS;
}

Status ACPIGetTableBySignature(const char *signature, size_t index, void **outTable) {
    if (!KernelACPI.Initialized || !outTable || !signature) {
        return STATUS_INVALID_PARAMETER;
    }
    *outTable = NULL;  // default if not found
    size_t matchCount = 0;

    if (!strncmp(signature, "DSDT", 4)) {
        if (!KernelACPI.FADT) return STATUS_FAILURE;
        if (index == 0) {
            *outTable = (void*)(uintptr_t)KernelACPI.FADT->Dsdt;
            return STATUS_SUCCESS;
        } else {
            return STATUS_FAILURE; // no more than one DSDT
        }
    }

    if (!strncmp(signature, "FADT", 4)) {
        if (!KernelACPI.FADT) return STATUS_FAILURE;
        if (index == 0) {
            *outTable = (void*)KernelACPI.FADT;
            return STATUS_SUCCESS;
        } else {
            return STATUS_FAILURE;
        }
    }

    if (useNewACPI && KernelACPI.XSDT) {
        int entries = (KernelACPI.XSDT->SDTHeader.Length - sizeof(KernelACPI.XSDT->SDTHeader)) / 8;
        for (int i = 0; i < entries; i++) {
            uint64_t addr = KernelACPI.XSDT->PointerToOtherSDT[i];  // 64-bit physical
            ACPISDTHeader* SDTHeader = (ACPISDTHeader*)(uintptr_t)addr;
            if (!strncmp(SDTHeader->Signature, signature, 4)) {
                if (matchCount == index) {
                    *outTable = SDTHeader;
                    return STATUS_SUCCESS;
                }
                matchCount++;
            }
        }
    } else if (KernelACPI.RSDT) {
        int entries = (KernelACPI.RSDT->SDTHeader.Length - sizeof(KernelACPI.RSDT->SDTHeader)) / 4;
        for (int i = 0; i < entries; i++) {
            uint32_t addr = KernelACPI.RSDT->PointerToOtherSDT[i];
            ACPISDTHeader* SDTHeader = (ACPISDTHeader*)(uintptr_t)addr;
            if (!strncmp(SDTHeader->Signature, signature, 4)) {
                if (matchCount == index) {
                    *outTable = SDTHeader;
                    return STATUS_SUCCESS;
                }
                matchCount++;
            }
        }
    }

    return STATUS_FAILURE; // not found
}
