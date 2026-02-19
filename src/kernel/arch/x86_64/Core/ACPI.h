#ifndef BOREALOS_ACPI_H
#define BOREALOS_ACPI_H

#include <Definitions.h>
#include "Boot/LimineDefinitions.h"
#include "Utility/StringFormatter.h"

// ACPI structs are from the OSDev wiki
namespace Core
{
    class ACPI {
        public:
        static constexpr const char* SDPRevisionStrings[] = {
            "ACPI 1.0",
            "Unknown",
            "ACPI 2.0+"
        };

        static constexpr const char* powerProfileStrings[] = {
            "Unspecified",
            "Desktop",
            "Mobile",
            "Workstation",
            "Enterprise Server",
            "SOHO Server",
            "Appliance PC",
            "Performance Server"
        };

        struct RSDP {
            char     signature[8];
            uint8_t  checksum;
            char     OEMID[6];
            uint8_t  revision;
            uint32_t RSDTAddress;
        } PACKED;

        struct XSDP {
            RSDP rsdp; // ACPI 1.0 fields

            uint32_t length;
            uint64_t XSDTAddress;
            uint8_t extendedChecksum;
            uint8_t reserved[3];
        } PACKED;

        struct SDTHeader {
            char signature[4];
            uint32_t length;
            uint8_t revision;
            uint8_t checksum;
            char OEMID[6];
            char OEMTableID[8];
            uint32_t OEMRevision;
            uint32_t creatorID;
            uint32_t creatorRevision;
        } PACKED;

        struct RSDT {
            SDTHeader sdt;
            uint32_t pointers[1]; // Placeholder for flexible array
        } PACKED;

        struct XSDT {
            SDTHeader sdt;
            uint64_t pointers[1]; // Placeholder for flexible array
        } PACKED;

        struct GenericAddr {
            uint8_t AddressSpace;
            uint8_t BitWidth;
            uint8_t BitOffset;
            uint8_t AccessSize;
            uint64_t Address;
        };

        struct FADT {
            SDTHeader sdt;
            uint32_t FirmwareCtrl;
            uint32_t Dsdt;

            // Field used in ACPI 1.0; no longer in use, for compatibility only
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

            // Reserved in ACPI 1.0; used since ACPI 2.0+
            uint16_t BootArchitectureFlags;

            uint8_t  Reserved2;
            uint32_t Flags;

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
        };

        void Initialize();
        bool ACPISupported();
        uint8_t powerProfile = 0;

        private:
        bool ValidateRSDP(RSDP* rsdp);
        bool ValidateXSDP(XSDP* xsdp);
        bool ValidateSDT(SDTHeader* sdt);
        void* FindFACP(void* rootSDT);
        void WriteByteCommand(uint8_t command);

        limine_rsdp_response* RSDPResponse;
        bool systemHasACPI = false;
        RSDP* rsdp;
        XSDP* xsdp;
        FADT* fadt;
        void* dsdt;
        void* facp;
    };
} // Core


#endif