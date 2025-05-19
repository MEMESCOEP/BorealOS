/* LIBRARIES */
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <Limine.h>
#include "Utilities/StrUtils.h"
#include "Core/Graphics/Terminal.h"
#include "Core/IO/Memory.h"
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
typedef struct {
    uint8_t Signature[8];
    uint8_t Checksum;
    uint8_t OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;
} __attribute__ ((packed)) RSDP_t;

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

//struct RSDP_t* RSDPTable;
uint64_t RSDPAddress = 0x00;
uint64_t HHOffset = 0x00;
float ACPIRevision = 0.0f;
bool ACPIInitialized = false;


/* FUNCTIONS */
#include <stdint.h>
//#include <string.h>

void InitACPI()
{
    // --- STEP 1 ---
    // Validate limine responses
    if (RSDPRequest.response == NULL)
    {
        KernelPanic(0, "No response was given for the RSDP request!");
    }

    if (HHDMRequest.response == NULL)
    {
        KernelPanic(0, "No response was given for the HHDM request!");
    }

    // HHOffset is added to convert the returned RSDP address to a virtual address (limine sets up paging)
    HHOffset = HHDMRequest.response->offset;
    RSDPAddress = RSDPRequest.response->address;

    

    // --- STEP 2 ---
    // Check if ACPI responses are valid for BIOS and UEFI
    // TODO: implement UEFI checking
    if (StrCmp(FirmwareType, "BIOS") == 0)
    {
        TerminalDrawMessage("Checking if RSDP address is valid for BIOS firmware...\n\r", INFO);

        if (RSDPRequest.response->address < 0x000E0000 || RSDPRequest.response->address > 0x000FFFFF)
        {
            KernelPanic(0, "RSDP address is out of range for BIOS firmware (RANGE IS 0x000E0000-0x000FFFFF)!");
        }

        TerminalDrawMessage("RSDP address is within the valid range for BIOS firmware (0x000E0000-0x000FFFFF).\n\r", INFO);
    }
    else if (StrCmp(FirmwareType, "64-bit UEFI") == 0)
    {
        TerminalDrawMessage("Checking if RSDP address is valid for 64-bit UEFI firmware...\n\r", INFO);
    }
    else
    {
        KernelPanic(0, "No ACPI support is available yet for non-BIOS / 64-bit UEFI firmwares.");
    }

    

    // --- STEP 3 ---
    // Get the RSDP address and the higher half offset
    char FixedRSDPAddrBuffer[32] = "";
    char HHOffsetBuffer[32] = "";
    char RSDPAddrBuffer[16] = "";

    IntToStr(RSDPAddress + HHOffset, FixedRSDPAddrBuffer, 16);
    IntToStr(RSDPRequest.response->address, RSDPAddrBuffer, 16);
    IntToStr(HHOffset, HHOffsetBuffer, 16);

    // Sometimes Limine doesn't provide this on 64-bit UEFI firmware
    if (HHOffsetBuffer[0] == '\0')
    {
        TerminalDrawMessage("Higher half offset was not provided from bootloader.\n\r", WARNING);
    }
    else
    {
        TerminalDrawMessage("Higher half offset (provided from bootloader): 0x", INFO);
        TerminalDrawString(HHOffsetBuffer);
        TerminalDrawString("\n\r");
    }

    TerminalDrawMessage("ACPI RSDP address (provided from bootloader): 0x", INFO);
    TerminalDrawString(RSDPAddrBuffer);
    TerminalDrawString("\n\r");

    TerminalDrawMessage("Offset ACPI RSDP address: 0x", INFO);
    TerminalDrawString(FixedRSDPAddrBuffer);
    TerminalDrawString("\n\r");



    // --- STEP 4 ---
    // Get the ACPI revision
    TerminalDrawMessage("ACPI revision: ", INFO);
    TerminalDrawChar(RSDPRequest.response->revision + '0', true);
    TerminalDrawString("\n\r");
    ACPIRevision = RSDPRequest.response->revision + 1.0f;

    

    // --- STEP 5 ---
    // Load the RSDP table into a description struct
    TerminalDrawMessage("Loading ACPI RSDP table into description struct...\n\r", INFO);
    RSDP_t* RSDPTable = (RSDP_t *)(uintptr_t)(RSDPAddress);

    TerminalDrawMessage("Can this please work\n\r", INFO);
    for (int CharIndex = 0; CharIndex < 7; CharIndex++)
    {
        TerminalDrawChar(RSDPTable->Signature[CharIndex], true);
    }

    TerminalDrawString("\n\r");
    ACPIInitialized = true;
}

bool ChecksumValidateSDT(ACPISDTHeader* SDTTableHeader)
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