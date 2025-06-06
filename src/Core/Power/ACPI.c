/* LIBRARIES */
#include <Utilities/StrUtils.h>
#include <Core/Interrupts/IDT.h>
#include <Core/Multiboot/MB2Parser.h>
#include <Core/Graphics/Console.h>
#include <Core/Hardware/Firmware.h>
#include <Core/Memory/Memory.h>
#include <Core/Kernel/Panic.h>
#include <Core/Power/ACPI.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "Core/Memory/PagingManager.h"


/* VARIABLES */
RSDPDescriptor20* SDPDescriptor;
RSDPTag* NewSDP;
FADT* FADTStruct;
RSDT* RSDTStruct;
XSDT* XSDTStruct;
uint64_t SDTEntryCount = 0;
uint64_t FACPAddress = 0x00;
uint64_t RSDPAddress = 0x00;
uint8_t ACPIRevision = 0;
bool ACPIInitialized = false;


/* FUNCTIONS */
bool ValidateSDP();

void InitACPI(void* MB2InfoPtr)
{
    // --- STEP 1 ---
    // Load the RSDP or XSDP into the right descriptor for the correct ACPI revision
    LOG_KERNEL_MSG("\tAttempting to load SDP from ACPI rev 2.0+ multiboot tags...\n\r", NONE);
    NewSDP = (RSDPTag*)FindMB2Tag(MULTIBOOT_TAG_TYPE_ACPI_NEW, MB2InfoPtr);

    // Instead of implementing a firmware type check, this method is more robust because some BIOS systems return ACPI revisions of 2 or greater, while some return 1. A firmware check would limit BIOS to ACPI 1.0, whereas this method
    // allows ACPI 2.0 to be recognized on all systems where it's present
    if (NewSDP == NULL)
    {
        LOG_KERNEL_MSG("\tMULTIBOOT_TAG_TYPE_ACPI_NEW returned null, falling back to ACPI rev 1.0...\n\r", WARN);
        NewSDP = (RSDPTag*)FindMB2Tag(MULTIBOOT_TAG_TYPE_ACPI_OLD, MB2InfoPtr);
    }

    if (NewSDP == NULL)
    {
        KernelPanic(0, "ACPI is not supported on this system!");
    }
    
    // Now we need to convert this to a descriptor that makes things easier to access for us. We also need to check if the descriptor is null here because a null descriptor means that this system does not support ACPI
    LOG_KERNEL_MSG("\tCreating SDP dscriptor...\n\r", NONE);
    SDPDescriptor = (RSDPDescriptor20*)NewSDP->SDP;

    // Now we verify the SDP descriptor to see if it can be relied on
    ValidateSDP();



    // --- STEP 2 ---
    // Load the RSDT or XSDT depending on the ACPI revision. Revisions 2.0 and above use the XSDT, while revision 1 uses the RSDT
    if (SDPDescriptor->Revision >= 2)
    {
        LOG_KERNEL_MSG("\tLoading XSDT from address 0x", NONE);
        PrintUnsignedNum(SDPDescriptor->XSDTAddress, 16);
        ConsolePutString("...\n\r");
        XSDTStruct = (XSDT*)SDPDescriptor->XSDTAddress;

        // Validate the XSDT now
        LOG_KERNEL_MSG("\tValidating XSDT...\n\r", NONE);

        // Map the XSDT into the kernel's address space
        MapPage((uintptr_t)XSDTStruct, SDPDescriptor->XSDTAddress, PAGE_FLAGS_PRESENT | PAGE_FLAGS_RW);

        if (MemCmp(XSDTStruct->SDTHeader.Signature, "XSDT", 4) == false)
        {
            KernelPanic(0, "XSDT checksum verification failed, signature is not \"XSDT\"!");
        }

        // Now we need to find the number of entries in the RSDT
        SDTEntryCount = (XSDTStruct->SDTHeader.Length - sizeof(ACPISDTHeader)) / sizeof(uint64_t);
    }
    else
    {
        LOG_KERNEL_MSG("\tLoading RSDT from address 0x", NONE);
        PrintUnsignedNum(SDPDescriptor->RSDTAddress, 16);
        ConsolePutString("...\n\r");

        RSDTStruct = (RSDT*)SDPDescriptor->RSDTAddress;

        // Map the RSDT into the kernel's address space
        MapPage((uintptr_t)RSDTStruct, SDPDescriptor->RSDTAddress, PAGE_FLAGS_PRESENT | PAGE_FLAGS_RW);

        // Validate the RSDT now
        LOG_KERNEL_MSG("\tValidating RSDT...\n\r", NONE);

        if (MemCmp(RSDTStruct->SDTHeader.Signature, "RSDT", 4) == false)
        {
            KernelPanic(0, "RSDT checksum verification failed, signature is not \"RSDT\"!");
        }

        // Now we need to find the number of entries in the RSDT
        SDTEntryCount = (RSDTStruct->SDTHeader.Length - sizeof(ACPISDTHeader)) / sizeof(uint32_t);
    }

    LOG_KERNEL_MSG("\tThe SDT contains ", NONE);
    PrintSignedNum(SDTEntryCount, 10);
    ConsolePutString(" entries.\n\r");



    // --- STEP 3 ---
    // Find the FADT from the relavant SDT
    if (SDPDescriptor->Revision >= 2)
    {
        for (int EntryIndex = 0; EntryIndex < SDTEntryCount; EntryIndex++)
        {
            ACPISDTHeader* SDTHeader = (ACPISDTHeader*)XSDTStruct->PointerToOtherSDT[EntryIndex];

            MapPage((uintptr_t)SDTHeader, (uintptr_t)SDTHeader, PAGE_FLAGS_PRESENT | PAGE_FLAGS_RW);

            if (MemCmp(SDTHeader->Signature, "FACP", 4) == true)
            {
                FACPAddress = (uint64_t*)SDTHeader;

                LOG_KERNEL_MSG("\tFound FACP at address 0x", NONE);
                PrintUnsignedNum(FACPAddress, 16);
                ConsolePutString(" and at entry #");
                PrintSignedNum(EntryIndex, 10);
                ConsolePutString(".\n\r");
                break;
            }
        }
    }
    else
    {
        for (int EntryIndex = 0; EntryIndex < SDTEntryCount; EntryIndex++)
        {
            ACPISDTHeader* SDTHeader = (ACPISDTHeader*)RSDTStruct->PointerToOtherSDT[EntryIndex];

            if (MemCmp(SDTHeader->Signature, "FACP", 4) == true)
            {
                FACPAddress = (uint64_t*)SDTHeader;

                LOG_KERNEL_MSG("\tFound FACP at address 0x", NONE);
                PrintUnsignedNum(FACPAddress, 16);
                ConsolePutString(" and at entry #");
                PrintSignedNum(EntryIndex, 10);
                ConsolePutString(".\n\r");
                break;
            }
        }
    }

    // Make sure the FACP was found
    if (FACPAddress == 0x00)
    {
        KernelPanic(0, "The FACP could not be found!");
    }

    // Now take the address we just found and create a FADT struct with it
    FADTStruct = (FADT*)FACPAddress;

    LOG_KERNEL_MSG("\tPreferred power management profile mode is \"", NONE);
    ConsolePutString(ConvertPPMPToStr(FADTStruct->PreferredPowerManagementProfile));
    ConsolePutString("\" (mode #");
    PrintUnsignedNum(FADTStruct->PreferredPowerManagementProfile, 10);
    ConsolePutString(").\n\r");



    // --- STEP 4 ---
    // Set up the IRQ handler for the system control interrupt (sometimes called SCI)
    LOG_KERNEL_MSG("\tSCI IRQ pin is ", NONE);
    PrintUnsignedNum(FADTStruct->SCI_Interrupt, 10);
    ConsolePutString(".\n\r");

    LOG_KERNEL_MSG("\tSetting up IRQ handler for SCI...\n\r", NONE);
    IDTSetIRQHandler(FADTStruct->SCI_Interrupt, ACPIShutdown);
    PICClearIRQMask(FADTStruct->SCI_Interrupt);



    // --- STEP 5 ---
    // Enable ACPI mode
    LOG_KERNEL_MSG("\tSCI command I/O port is 0x", NONE);
    PrintUnsignedNum(FADTStruct->SMI_CommandPort, 16);
    ConsolePutString(".\n\r");

    if (FADTStruct->SMI_CommandPort != 0 && FADTStruct->AcpiEnable != 0)
    {
        int Timeout = 1000000;
        LOG_KERNEL_MSG("\tSending ACPI enable command to SCI command port...\n\r", NONE);
        OutB(FADTStruct->SMI_CommandPort, FADTStruct->AcpiEnable);
        while ((InW(FADTStruct->PM1aControlBlock) & 1) == 0)
        {
            Timeout--;

            if (Timeout < 0)
            {
                LOG_KERNEL_MSG("\tACPI initialization failed, PM1aControlBlock timeout exceeded.\n\r", ERROR);
                return;
            }
        }
    }
    else
    {
        LOG_KERNEL_MSG("\tSMI command port or AcpiEnable data is 0, it's not safe to try enabling ACPI at this time.\n\r", WARN);
    }

    // Make the power button generate an IRQ what it gets pressed by clearing any pending power button events and then writing 1 to the PM1_EN power button status bit
    uint16_t Enable = InW(FADTStruct->PM1aEventBlock + 2);
    Enable |= PM1_EN_POWER_BUTTON_BIT;

    OutW(FADTStruct->PM1aEventBlock, PM1_EN_POWER_BUTTON_BIT);
    OutW(FADTStruct->PM1aEventBlock + 2, Enable);

    LOG_KERNEL_MSG("\tACPI initialized.\n\n\r", NONE);
    ACPIRevision = SDPDescriptor->Revision;
    ACPIInitialized = true;
}

bool ValidateSDP()
{
    uint8_t FieldSum = 0;

    // Validate the RSDP
    LOG_KERNEL_MSG("\tVerifying RSDP checksum...\n\r", NONE);

    // Check if the sum of all the byte fields in the RSDP & 0xFF is equal to 0
    for (int i = 0; i < 20; i++)
    {
        FieldSum += ((uint8_t*)SDPDescriptor)[i];
    }

    LOG_KERNEL_MSG("\tRSDP checksum is ", NONE);
    PrintUnsignedNum(FieldSum & 0xFF, 10);
    ConsolePutString(".\n\r");

    if ((FieldSum & 0xFF) != 0)
    {
        LOG_KERNEL_MSG("\tRSDP checksum verification failed, FieldSum & 0xFF is not 0.\n\r", WARN);
        return false;
    }

    // If the system implements ACPI revision 2.0+, validate the XSDP too
    if (SDPDescriptor->Revision >= 2)
    {
        FieldSum = 0;
        LOG_KERNEL_MSG("\tVerifying XSDP checksum...\n\r", NONE);

        for (uint32_t i = 0; i < SDPDescriptor->Length; i++)
        {
            FieldSum += ((uint8_t*)SDPDescriptor)[i];
        }

        LOG_KERNEL_MSG("\tXSDP checksum is ", NONE);
        PrintUnsignedNum(FieldSum & 0xFF, 10);
        ConsolePutString(".\n\r");

        if ((FieldSum & 0xFF) != 0)
        {
            LOG_KERNEL_MSG("\tXSDP checksum verification failed, FieldSum & 0xFF is not 0.\n\r", WARN);
            return false;
        }
    }

    return true;
}

// Convert the FADT's "PreferredPowerManagementProfile" value to a string
char* ConvertPPMPToStr(uint8_t PPMP)
{
    switch(PPMP)
    {
        case FADT_POWER_MGMT_MODE_UNSPECIFIED:
            return "Unspecified";

        case FADT_POWER_MGMT_MODE_DESKTOP:
            return "Desktop";

        case FADT_POWER_MGMT_MODE_MOBILE:
            return "Mobile";

        case FADT_POWER_MGMT_MODE_WORKSTATION:
            return "Workstation";

        case FADT_POWER_MGMT_MODE_ENTERPRISE_SERVER:
            return "Enterprise server";

        case FADT_POWER_MGMT_MODE_SOHO_SERVER:
            return "SOHO server";

        case FADT_POWER_MGMT_MODE_APPLIANCE_PC:
            return "Appliance PC";
    
        case FADT_POWER_MGMT_MODE_PERFORMANCE_SERVER:
            return "Performance server";

        default:
            return "Reserved or unknown";
    }
}

uint16_t GetBootArchFlags()
{
    return FADTStruct->BootArchitectureFlags;
}

void ACPIShutdown()
{
    KernelPanic(0, "ACPI shutdown is not implemented yet.");
}

void ACPIReboot()
{
    KernelPanic(0, "ACPI reboot is not implemented yet.");
}