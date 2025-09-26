#ifndef BOREALOS_ACPI_H
#define BOREALOS_ACPI_H

#include <Definitions.h>
#include "ACPI.h"

#define RSDP_SEARCH_REGION_START 0x000E0000
#define RSDP_SEARCH_REGION_END 0x000FFFFF
#define RSDP_TABLE_LEN 20
#define RSDP_SEARCH_STR "RSD PTR "

// These structs were provided by the OSDEV wiki, and have been edited to comply with this project's code standards
typedef struct {
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;
} PACKED RSDP_t;

typedef struct  {
 char Signature[8];
 uint8_t Checksum;
 char OEMID[6];
 uint8_t Revision;
 uint32_t RsdtAddress;      // deprecated since version 2.0

 uint32_t Length;
 uint64_t XsdtAddress;
 uint8_t ExtendedChecksum;
 uint8_t reserved[3];
} PACKED XSDP_t;

typedef struct {
  uint8_t AddressSpace;
  uint8_t BitWidth;
  uint8_t BitOffset;
  uint8_t AccessSize;
  uint64_t Address;
} GenericAddr;

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
    GenericAddr ResetReg;

    uint8_t  ResetValue;
    uint8_t  Reserved3[3];
  
    // 64bit pointers - Available on ACPI 2.0+
    uint64_t                X_FirmwareControl;
    uint64_t                X_Dsdt;

    GenericAddr X_PM1aEventBlock;
    GenericAddr X_PM1bEventBlock;
    GenericAddr X_PM1aControlBlock;
    GenericAddr X_PM1bControlBlock;
    GenericAddr X_PM2ControlBlock;
    GenericAddr X_PMTimerBlock;
    GenericAddr X_GPE0Block;
    GenericAddr X_GPE1Block;
} FADT_t;

typedef struct {
  ACPISDTHeader SDTHeader;
  uint32_t PointerToOtherSDT[];
} RSDT_t;

typedef struct {
  ACPISDTHeader SDTHeader;
  uint64_t PointerToOtherSDT[];
} XSDT_t;

typedef struct {
    RSDP_t* RSDP;
    XSDP_t* XSDP;
    RSDT_t* RSDT;
    XSDT_t* XSDT;
    FADT_t* FADT;
    bool ValidPowerMGMTMode;
    bool Initialized;
} ACPIState;

extern ACPIState ACPI;
extern const char* PowerProfileStrings[8];

Status ACPIInit(uint32_t InfoPtr);

#endif //BOREALOS_ACPI_H