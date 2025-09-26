#ifndef BOREALOS_ACPI_H
#define BOREALOS_ACPI_H

#include <Definitions.h>
#include "ACPI.h"

#define RSDP_SEARCH_REGION_START 0x000E0000
#define RSDP_SEARCH_REGION_END 0x000FFFFF
#define RSDP_TABLE_LEN 20
#define RSDP_SEARCH_STR "RSD PTR "

PACKED struct RSDP_t {
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;
};

PACKED struct XSDP_t {
 char Signature[8];
 uint8_t Checksum;
 char OEMID[6];
 uint8_t Revision;
 uint32_t RsdtAddress;      // deprecated since version 2.0

 uint32_t Length;
 uint64_t XsdtAddress;
 uint8_t ExtendedChecksum;
 uint8_t reserved[3];
};

struct GenericAddr
{
  uint8_t AddressSpace;
  uint8_t BitWidth;
  uint8_t BitOffset;
  uint8_t AccessSize;
  uint64_t Address;
};

struct ACPISDTHeader {
  char Signature[4];
  uint32_t Length;
  uint8_t Revision;
  uint8_t Checksum;
  char OEMID[6];
  char OEMTableID[8];
  uint32_t OEMRevision;
  uint32_t CreatorID;
  uint32_t CreatorRevision;
};

struct FADT_t
{
    struct   ACPISDTHeader SDTHeader;
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
    struct GenericAddr ResetReg;

    uint8_t  ResetValue;
    uint8_t  Reserved3[3];
  
    // 64bit pointers - Available on ACPI 2.0+
    uint64_t                X_FirmwareControl;
    uint64_t                X_Dsdt;

    struct GenericAddr X_PM1aEventBlock;
    struct GenericAddr X_PM1bEventBlock;
    struct GenericAddr X_PM1aControlBlock;
    struct GenericAddr X_PM1bControlBlock;
    struct GenericAddr X_PM2ControlBlock;
    struct GenericAddr X_PMTimerBlock;
    struct GenericAddr X_GPE0Block;
    struct GenericAddr X_GPE1Block;
};

struct RSDT_t {
  struct ACPISDTHeader SDTHeader;
  uint32_t PointerToOtherSDT[];
};

struct XSDT_t {
  struct ACPISDTHeader SDTHeader;
  uint64_t PointerToOtherSDT[];
};

typedef struct ACPIState {
    struct RSDP_t* RSDP;
    struct XSDP_t* XSDP;
    struct RSDT_t* RSDT;
    struct XSDT_t* XSDT;
    struct FADT_t* FADT;
    bool ValidPowerMGMTMode;
    bool Initialized;
} ACPIState;

extern ACPIState ACPI;
extern const char* PowerProfileStrings[8];

Status ACPIInit(uint32_t InfoPtr);

#endif //BOREALOS_ACPI_H