#ifndef ACPI_H
#define ACPI_H

/* LIBRARIES */
#include <Utilities/AddrUtils.h>
#include <Core/Multiboot/MB2Parser.h>


/* DEFINITIONS */
#define FADT_POWER_MGMT_MODE_UNSPECIFIED        0
#define FADT_POWER_MGMT_MODE_DESKTOP            1
#define FADT_POWER_MGMT_MODE_MOBILE             2
#define FADT_POWER_MGMT_MODE_WORKSTATION        3
#define FADT_POWER_MGMT_MODE_ENTERPRISE_SERVER  4
#define FADT_POWER_MGMT_MODE_SOHO_SERVER        5
#define FADT_POWER_MGMT_MODE_APPLIANCE_PC       6
#define FADT_POWER_MGMT_MODE_PERFORMANCE_SERVER 7
#define PM1_EN_POWER_BUTTON_BIT                 (1 << 8)
#define MAX_MEMORY_REGIONS                      128
#define RSDP_RANGE_BOTTOM                       0x000E0000
#define RSDP_RANGE_TOP                          0x000FFFFF
#define E820_MEMORY_MAP                         0xE820


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

typedef struct
{
    ACPISDTHeader SDTHeader;
    uint32_t FirmwareCtrl;
    uint32_t Dsdt;

    // field used in ACPI 1.0; no longer in use, for compatibility only
    uint8_t  Reserved;

    uint8_t  PreferredPowerManagementProfile;
    uint16_t SCI_Interrupt;
    uint32_t SMI_CommandPort;
    uint8_t  AcpiEnable;
    uint8_t  AcpiDisable;
    uint8_t  S4BIOS_REQ;
    uint8_t  PSTATE_Control;
    uint32_t PM1aEventBlock;
    uint32_t PM1bEventBlock;
    uint32_t PM1aControlBlock;
    uint32_t PM1bControlBlock;
    uint32_t PM2ControlBlock;
    uint32_t PMTimerBlock;
    uint32_t GPE0Block;
    uint32_t GPE1Block;
    uint8_t  PM1EventLength;
    uint8_t  PM1ControlLength;
    uint8_t  PM2ControlLength;
    uint8_t  PMTimerLength;
    uint8_t  GPE0Length;
    uint8_t  GPE1Length;
    uint8_t  GPE1Base;
    uint8_t  CStateControl;
    uint16_t WorstC2Latency;
    uint16_t WorstC3Latency;
    uint16_t FlushSize;
    uint16_t FlushStride;
    uint8_t  DutyOffset;
    uint8_t  DutyWidth;
    uint8_t  DayAlarm;
    uint8_t  MonthAlarm;
    uint8_t  Century;

    // reserved in ACPI 1.0; used since ACPI 2.0+
    uint16_t BootArchitectureFlags;

    uint8_t  Reserved2;
    uint32_t Flags;

    // 12 byte structure; see below for details
    GenericAddressStructure ResetReg;

    uint8_t  ResetValue;
    uint8_t  Reserved3[3];
  
    // 64bit pointers - Available on ACPI 2.0+
    uint64_t                X_FirmwareControl;
    uint64_t                X_Dsdt;

    GenericAddressStructure X_PM1aEventBlock;
    GenericAddressStructure X_PM1bEventBlock;
    GenericAddressStructure X_PM1aControlBlock;
    GenericAddressStructure X_PM1bControlBlock;
    GenericAddressStructure X_PM2ControlBlock;
    GenericAddressStructure X_PMTimerBlock;
    GenericAddressStructure X_GPE0Block;
    GenericAddressStructure X_GPE1Block;
} __attribute__((packed)) FADT;

extern RSDPDescriptor20* SDPDescriptor;
extern RSDPTag* NewSDP;
extern FADT* FADTStruct;
extern RSDT* RSDTStruct;
extern XSDT* XSDTStruct;
extern uint64_t RSDPAddress;
extern uint64_t HHOffset;
extern uint8_t ACPIRevision;
extern bool ACPIInitialized;


/* FUNCTIONS */
uint16_t GetBootArchFlags();
char* ConvertPPMPToStr(uint8_t PPMP);
void InitACPI(void* MB2InfoPtr);
void ACPIShutdown();
void ACPIReboot();

#endif