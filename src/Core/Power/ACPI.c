/* LIBRARIES */
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <Utilities/StrUtils.h>
#include <Core/Multiboot/MB2Parser.h>
#include <Core/Graphics/Console.h>
#include <Core/Hardware/Firmware.h>
#include <Core/Memory/Memory.h>
#include <Core/Kernel/Panic.h>
#include <Core/Power/ACPI.h>


/* VARIABLES */
// These structs don't always follow the kernel code convention, but this makes it easier to load them from an address
typedef struct {
  char Signature[4];
  uint32_t Length;
  uint8_t Revision;
  uint8_t Checksum;
  char OEMID[6];
  char OEMTableID[8];
  uint32_t OEMRevision;
  uint32_t CreatorID;
  uint32_t CreatorRevision;
} ACPISDTHeader;

typedef struct {
    char     Signature[8];        // "RSD PTR "
    uint8_t  Checksum;            // ACPI 1.0 checksum
    char     OEMID[6];            // OEM ID
    uint8_t  Revision;            // 0 = ACPI 1.0, 2 = ACPI 2.0+
    uint32_t RSDTAddress;         // 32-bit RSDT pointer

    // ACPI 2.0+ fields
    uint32_t Length;              // Total length of this struct
    uint64_t XSDTAddress;         // 64-bit XSDT pointer
    uint8_t  ExtendedChecksum;    // Checksum of entire table
    uint8_t  Reserved[3];
} __attribute__((packed)) RSDPDescriptor20;

typedef struct {
    MB2Tag_t MBTag;
    uint8_t SDP[];
} RSDPTag;

typedef struct {
  ACPISDTHeader SDTHeader;
  uint32_t PointerToOtherSDT[];
} RSDT;

typedef struct {
  ACPISDTHeader SDTHeader;
  uint64_t PointerToOtherSDT[];
} XSDT;

RSDPDescriptor20* SDPDescriptor;
RSDPTag* NewSDP;
RSDT* RSDTStruct;
XSDT* XSDTStruct;
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
    
    // Now we need to convert this to a descriptor that makes things easier to access for us. We also need to check if the descriptor is null here because a null descriptor means that this system does not support ACPI
    LOG_KERNEL_MSG("\tCreating SDP dscriptor...\n\r", NONE);
    SDPDescriptor = (RSDPDescriptor20*)NewSDP->SDP;

    if (SDPDescriptor == NULL)
    {
        KernelPanic(0, "ACPI is not supported on this system!");
    }

    // Now we verify the SDP descriptor to see if it can be relied on
    ValidateSDP();



    // --- STEP 2 ---
    // Load the RSDT or XSDT depending on the ACPI revision. Revisions 2.0 and above use the XSDT, while revision 1 uses the RSDT
    if (SDPDescriptor->Revision >= 2)
    {
        LOG_KERNEL_MSG("\tLoading XSDT from address 0x", NONE);
        PrintNum(SDPDescriptor->XSDTAddress, 16);
        ConsolePutString("...\n\r");
        XSDTStruct = (XSDT*)SDPDescriptor->XSDTAddress;

        // Validate the XSDT now
        LOG_KERNEL_MSG("\tValidating XSDT...\n\r", NONE);

        if (MemCmp(XSDTStruct->SDTHeader.Signature, "XSDT", 4) == false)
        {
            KernelPanic(0, "XSDT checksum verification failed, signature is not \"XSDT\"!");
        }

        // Now we need to find the number of entries in the RSDT
        uint64_t EntryCount = (XSDTStruct->SDTHeader.Length - sizeof(ACPISDTHeader)) / sizeof(uint64_t);

        LOG_KERNEL_MSG("\tThe XSDT contains ", NONE);
        PrintNum(EntryCount, 10);
        ConsolePutString(" entries.\n\r");
    }
    else
    {
        LOG_KERNEL_MSG("\tLoading RSDT from address 0x", NONE);
        PrintNum(SDPDescriptor->RSDTAddress, 16);
        ConsolePutString("...\n\r");

        RSDTStruct = (RSDT*)SDPDescriptor->RSDTAddress;

        // Validate the RSDT now
        LOG_KERNEL_MSG("\tValidating RSDT...\n\r", NONE);

        if (MemCmp(RSDTStruct->SDTHeader.Signature, "RSDT", 4) == false)
        {
            KernelPanic(0, "RSDT checksum verification failed, signature is not \"RSDT\"!");
        }

        // Now we need to find the number of entries in the RSDT
        uint32_t EntryCount = (RSDTStruct->SDTHeader.Length - sizeof(ACPISDTHeader)) / sizeof(uint32_t);

        LOG_KERNEL_MSG("\tThe RSDT contains ", NONE);
        PrintNum(EntryCount, 10);
        ConsolePutString(" entries.\n\r");
    }

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
    PrintNum(FieldSum & 0xFF, 10);
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
        PrintNum(FieldSum & 0xFF, 10);
        ConsolePutString(".\n\r");

        if ((FieldSum & 0xFF) != 0)
        {
            LOG_KERNEL_MSG("\tXSDP checksum verification failed, FieldSum & 0xFF is not 0.\n\r", WARN);
            return false;
        }
    }

    return true;
}

void ACPIShutdown()
{
    KernelPanic(0, "ACPI shutdown is not implemented yet.");
}

void ACPIReboot()
{
    KernelPanic(0, "ACPI reboot is not implemented yet.");
}