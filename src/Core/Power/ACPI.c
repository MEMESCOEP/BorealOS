/* LIBRARIES */
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <Limine.h>
#include "Utilities/StrUtils.h"
#include "Core/Graphics/Terminal.h"
#include "Kernel.h"
#include "ACPI.h"


/* VARIABLES */
__attribute__((used, section(".limine_requests")))
struct limine_rsdp_request RSDPRequest = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request HHDMRequest = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
};

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

// RSDP for ACPI 1.0
struct RSDP_t {
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RSDTAddress;
} __attribute__ ((packed));

// RSDP for ACPI 2.0 and later
struct XSDP_t {
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RSDTAddress;      // deprecated since version 2.0

    uint32_t Length;
    uint64_t XSDTAddress;
    uint8_t ExtendedChecksum;
    uint8_t reserved[3];
} __attribute__ ((packed));

uint64_t HHOffset = 0x00;
uint64_t RSDPAddress = 0x00;
bool ACPIInitialized = false;


/* FUNCTIONS */
void InitACPI()
{
    if (RSDPRequest.response == NULL)
    {
        KernelPanic(0, "No response was given for the RSDP request!");
    }

    if (HHDMRequest.response == NULL)
    {
        KernelPanic(0, "No response was given for the HHDM request!");
    }

    HHOffset = HHDMRequest.response->offset;
    RSDPAddress = RSDPRequest.response->address + HHOffset;

    // Ensure that the HHDM and RSDP values are valid
    if (StrCmp(FirmwareType, "BIOS") == 0)
    {
        TerminalDrawString("[INFO] >> Checking if RSDP address is valid for BIOS firmware...\n\r");

        if (RSDPRequest.response->address < 0xE0000 || RSDPRequest.response->address > 0xFFFFF)
        {
            KernelPanic(0, "RSDP address is out of range for BIOS firmware (RANGE IS 0xE0000-0xFFFFF)!");
        }

        TerminalDrawString("[INFO] >> RSDP address is withing the valid range for BIOS firmware (0xE0000-0xFFFFF).\n\r");
    }
    else if (StrCmp(FirmwareType, "64-bit UEFI") == 0)
    {
        TerminalDrawString("[INFO] >> Checking if RSDP address is valid for 64-bit UEFI firmware...\n\r");
    }
    else
    {
        KernelPanic(0, "No ACPI support is available yet for non-BIOS / 64-bit UEFI firmwares.");
    }

    char FixedRSDPAddrBuffer[32] = "";
    char HHOffsetBuffer[32] = "";
    char RSDPAddrBuffer[16] = "";

    IntToStr(RSDPAddress, FixedRSDPAddrBuffer, 16);
    IntToStr(RSDPRequest.response->address, RSDPAddrBuffer, 16);
    IntToStr(HHOffset, HHOffsetBuffer, 16);

    // Sometimes Limine doesn't provide this on 64-bit UEFI firmware
    if (HHOffsetBuffer[0] == '\0')
    {
        TerminalDrawString("[WARN] >> Higher half offset was notprovided from bootloader.\n\r");
    }
    else
    {
        TerminalDrawString("[INFO] >> Higher half offset (provided from bootloader): 0x");
        TerminalDrawString(HHOffsetBuffer);
        TerminalDrawString("\n\r");
    }

    TerminalDrawString("[INFO] >> ACPI RSDP address (provided from bootloader): 0x");
    TerminalDrawString(RSDPAddrBuffer);
    TerminalDrawString("\n\r");

    TerminalDrawString("[INFO] >> Offset ACPI RSDP address: 0x");
    TerminalDrawString(FixedRSDPAddrBuffer);
    TerminalDrawString("\n\r");

    TerminalDrawString("[INFO] >> ACPI revision: ");

    if (RSDPRequest.response->revision == 0)
    {
        TerminalDrawString("1.0\n\r");
    }
    else
    {
        TerminalDrawString("2.0+\n\r");
    }

    TerminalDrawString("[INFO] >> Loading ACPI RSDP table into description struct...\n\r");
    struct RSDP_t* RSDPTable = (struct RSDP_t*)RSDPAddress;

    ACPIInitialized = true;

    //TerminalDrawChar(RSDPTable->Signature[0], true);
}

bool ChecksumValidateSDT(ACPISDTHeader *SDTTableHeader)
{
    unsigned char Sum = 0;

    for (uint32_t i = 0; i < SDTTableHeader->Length; i++)
    {
        Sum += ((char *) SDTTableHeader)[i];
    }

    return Sum == 0;
}

void ACPIShutdown()
{
    KernelPanic(0, "ACPI shutdown is not implemented yet.");
}

void ACPIReboot()
{
    KernelPanic(0, "ACPI reboot is not implemented yet.");
}