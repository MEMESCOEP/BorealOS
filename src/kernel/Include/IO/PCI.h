#ifndef BOREALOS_PCI_H
#define BOREALOS_PCI_H

#include <Definitions.h>
#include "Utility/List.h"
#include "Memory/Paging.h"
#include "IO/Serial.h"

namespace IO {

    /// The PCI interface is common, though implementation differs from architecture to architecture.
    class PCI {
    public:
        struct PCIDeviceHeader {
            // Location
            uint8_t bus;
            uint8_t slot;
            uint8_t function;

            // Header fields (offsets 0x00-0x0F)
            uint16_t vendorID;       // 0x00
            uint16_t deviceID;       // 0x02
            uint16_t command;        // 0x04
            uint16_t status;         // 0x06
            uint8_t  revisionID;     // 0x08
            uint8_t  progIF;         // 0x09
            uint8_t  subclass;       // 0x0A
            uint8_t  classCode;      // 0x0B
            uint8_t  cacheLineSize;  // 0x0C
            uint8_t  latencyTimer;   // 0x0D
            uint8_t  headerType;     // 0x0E
            uint8_t  bist;           // 0x0F
        };

        struct PCI_BAR {
            bool     isIOPort;
            bool     is64Bit;
            bool     isPrefetchable;
            uint64_t address;
            uint64_t size;
        };

        struct MSIXTableEntry {
            uint32_t messageAddrLow;
            uint32_t messageAddrHigh;
            uint32_t messageData;
            uint32_t vectorControl; // Bit 0 is masked
        };

        // Common PCI header flags
        static constexpr uint16_t PCI_HEADER_FLAG_MULTIFUNCTION = 0x80;
        static constexpr uint16_t PCI_HEADER_FLAG_HEADER_TYPE   = 0x0E;
        static constexpr uint16_t PCI_HEADER_FLAG_CACHE_WORD    = 0x0C;
        static constexpr uint16_t PCI_HEADER_FLAG_CLASS_CODE    = 0x0A;
        static constexpr uint16_t PCI_HEADER_FLAG_VENDOR_ID     = 0x00;
        static constexpr uint16_t PCI_HEADER_FLAG_DEVICE_ID     = 0x02;
        static constexpr uint16_t PCI_HEADER_FLAG_REVISION      = 0x08;
        static constexpr uint16_t PCI_HEADER_FLAG_COMMAND       = 0x04;
        static constexpr uint16_t PCI_HEADER_FLAG_STATUS        = 0x06;

        // Capabilities
        static constexpr uint16_t PCI_CAPABILITIES_POINTER = 0x34;
        static constexpr uint8_t PCI_CAPABILITY_MSI_X      = 0x11;
        static constexpr uint8_t PCI_CAPABILITY_MSI        = 0x05;

        // Offsets
        static constexpr uint8_t PCI_OFFSET_SECONDARY_BUS = 0x18;

        // Header types
        static constexpr uint8_t PCI_HEADER_TYPE_PCI_CARDBUS_BRIDGE = 0x02;
        static constexpr uint8_t PCI_HEADER_TYPE_PCI_PCI_BRIDGE     = 0x01;
        static constexpr uint8_t PCI_HEADER_TYPE_GENERIC            = 0x00;

        // Class and subclass types
        // Unclassified (0x00)
        static constexpr uint8_t PCI_CLASS_UNCLASSIFIED                    = 0x00;
        static constexpr uint8_t PCI_SUBCLASS_NON_VGA_UNCLASSIFIED         = 0x00;
        static constexpr uint8_t PCI_SUBCLASS_VGA_UNCLASSIFIED             = 0x01;

        // Mass Storage Controller (0x01)
        static constexpr uint8_t PCI_CLASS_MASS_STORAGE                    = 0x01;
        static constexpr uint8_t PCI_SUBCLASS_MASS_STORAGE_OTHER           = 0x80;
        static constexpr uint8_t PCI_SUBCLASS_SCSI_BUS                     = 0x00;
        static constexpr uint8_t PCI_SUBCLASS_IPI_BUS                      = 0x03;
        static constexpr uint8_t PCI_SUBCLASS_FLOPPY                       = 0x02;
        static constexpr uint8_t PCI_SUBCLASS_RAID                         = 0x04;
        static constexpr uint8_t PCI_SUBCLASS_SATA                         = 0x06;
        static constexpr uint8_t PCI_SUBCLASS_IDE                          = 0x01;
        static constexpr uint8_t PCI_SUBCLASS_ATA                          = 0x05;
        static constexpr uint8_t PCI_SUBCLASS_SAS                          = 0x07;
        static constexpr uint8_t PCI_SUBCLASS_NVM                          = 0x08;

        // Network Controller (0x02)
        static constexpr uint8_t PCI_CLASS_NETWORK                         = 0x02;
        static constexpr uint8_t PCI_SUBCLASS_INFINIBAND_NET               = 0x07;
        static constexpr uint8_t PCI_SUBCLASS_NETWORK_OTHER                = 0x80;
        static constexpr uint8_t PCI_SUBCLASS_TOKEN_RING                   = 0x01;
        static constexpr uint8_t PCI_SUBCLASS_WORLDFIP                     = 0x05;
        static constexpr uint8_t PCI_SUBCLASS_ETHERNET                     = 0x00;
        static constexpr uint8_t PCI_SUBCLASS_FABRIC                       = 0x08;
        static constexpr uint8_t PCI_SUBCLASS_PICMG                        = 0x06;
        static constexpr uint8_t PCI_SUBCLASS_ISDN                         = 0x04;
        static constexpr uint8_t PCI_SUBCLASS_FDDI                         = 0x02;
        static constexpr uint8_t PCI_SUBCLASS_ATM                          = 0x03;

        // Display Controller (0x03)
        static constexpr uint8_t PCI_CLASS_DISPLAY                         = 0x03;
        static constexpr uint8_t PCI_SUBCLASS_VGA_COMPATIBLE               = 0x00;
        static constexpr uint8_t PCI_SUBCLASS_DISPLAY_OTHER                = 0x80;
        static constexpr uint8_t PCI_SUBCLASS_XGA                          = 0x01;
        static constexpr uint8_t PCI_SUBCLASS_3D                           = 0x02;

        // Multimedia Controller (0x04)
        static constexpr uint8_t PCI_CLASS_MULTIMEDIA                      = 0x04;
        static constexpr uint8_t PCI_SUBCLASS_COMPUTER_TELEPHONY           = 0x02;
        static constexpr uint8_t PCI_SUBCLASS_MULTIMEDIA_OTHER             = 0x80;
        static constexpr uint8_t PCI_SUBCLASS_MULTIMEDIA_VIDEO             = 0x00;
        static constexpr uint8_t PCI_SUBCLASS_MULTIMEDIA_AUDIO             = 0x01;
        static constexpr uint8_t PCI_SUBCLASS_AUDIO_DEVICE                 = 0x03;

        // Memory Controller (0x05)
        static constexpr uint8_t PCI_CLASS_MEMORY                          = 0x05;
        static constexpr uint8_t PCI_SUBCLASS_MEMORY_OTHER                 = 0x80;
        static constexpr uint8_t PCI_SUBCLASS_FLASH                        = 0x01;
        static constexpr uint8_t PCI_SUBCLASS_RAM                          = 0x00;

        // Bridge (0x06)
        static constexpr uint8_t PCI_CLASS_BRIDGE                          = 0x06;
        static constexpr uint8_t PCI_SUBCLASS_SEMI_TRANSPARENT_PCI_BRIDGE  = 0x09;
        static constexpr uint8_t PCI_SUBCLASS_INFINIBAND_PCI_BRIDGE        = 0x0A;
        static constexpr uint8_t PCI_SUBCLASS_RACEWAY_BRIDGE               = 0x08;
        static constexpr uint8_t PCI_SUBCLASS_PCMCIA_BRIDGE                = 0x05;
        static constexpr uint8_t PCI_SUBCLASS_CARDBUS_BRIDGE               = 0x07;
        static constexpr uint8_t PCI_SUBCLASS_PCI_PCI_BRIDGE               = 0x04;
        static constexpr uint8_t PCI_SUBCLASS_BRIDGE_OTHER                 = 0x80;
        static constexpr uint8_t PCI_SUBCLASS_NUBUS_BRIDGE                 = 0x06;
        static constexpr uint8_t PCI_SUBCLASS_HOST_BRIDGE                  = 0x00;
        static constexpr uint8_t PCI_SUBCLASS_EISA_BRIDGE                  = 0x02;
        static constexpr uint8_t PCI_SUBCLASS_ISA_BRIDGE                   = 0x01;
        static constexpr uint8_t PCI_SUBCLASS_MCA_BRIDGE                   = 0x03;

        // Simple Communication Controller (0x07)
        static constexpr uint8_t PCI_CLASS_SIMPLE_COMM                     = 0x07;
        static constexpr uint8_t PCI_SUBCLASS_SIMPLE_COMM_OTHER            = 0x80;
        static constexpr uint8_t PCI_SUBCLASS_MULTIPORT_SERIAL             = 0x02;
        static constexpr uint8_t PCI_SUBCLASS_SMART_CARD                   = 0x05;
        static constexpr uint8_t PCI_SUBCLASS_PARALLEL                     = 0x01;
        static constexpr uint8_t PCI_SUBCLASS_SERIAL                       = 0x00;
        static constexpr uint8_t PCI_SUBCLASS_MODEM                        = 0x03;
        static constexpr uint8_t PCI_SUBCLASS_GPIB                         = 0x04;

        // Base System Peripheral (0x08)
        static constexpr uint8_t PCI_CLASS_BASE_SYSTEM                     = 0x08;
        static constexpr uint8_t PCI_SUBCLASS_BASE_SYSTEM_OTHER            = 0x80;
        static constexpr uint8_t PCI_SUBCLASS_PCI_HOT_PLUG                 = 0x04;
        static constexpr uint8_t PCI_SUBCLASS_SD_HOST                      = 0x05;
        static constexpr uint8_t PCI_SUBCLASS_IOMMU                        = 0x06;
        static constexpr uint8_t PCI_SUBCLASS_TIMER                        = 0x02;
        static constexpr uint8_t PCI_SUBCLASS_DMA                          = 0x01;
        static constexpr uint8_t PCI_SUBCLASS_RTC                          = 0x03;
        static constexpr uint8_t PCI_SUBCLASS_PIC                          = 0x00;

        // Input Device Controller (0x09)
        static constexpr uint8_t PCI_CLASS_INPUT                           = 0x09;
        static constexpr uint8_t PCI_SUBCLASS_DIGITIZER_PEN                = 0x01;
        static constexpr uint8_t PCI_SUBCLASS_INPUT_OTHER                  = 0x80;
        static constexpr uint8_t PCI_SUBCLASS_KEYBOARD                     = 0x00;
        static constexpr uint8_t PCI_SUBCLASS_GAMEPORT                     = 0x04;
        static constexpr uint8_t PCI_SUBCLASS_SCANNER                      = 0x03;
        static constexpr uint8_t PCI_SUBCLASS_MOUSE                        = 0x02;

        // Docking Station (0x0A)
        static constexpr uint8_t PCI_CLASS_DOCKING_STATION                 = 0x0A;
        static constexpr uint8_t PCI_SUBCLASS_DOCKING_GENERIC              = 0x00;
        static constexpr uint8_t PCI_SUBCLASS_DOCKING_OTHER                = 0x80;

        // Processor (0x0B)
        static constexpr uint8_t PCI_CLASS_PROCESSOR                       = 0x0B;
        static constexpr uint8_t PCI_SUBCLASS_PROC_COPROCESSOR             = 0x40;
        static constexpr uint8_t PCI_SUBCLASS_PROC_PENTIUM_PRO             = 0x03;
        static constexpr uint8_t PCI_SUBCLASS_PROC_PENTIUM                 = 0x02;
        static constexpr uint8_t PCI_SUBCLASS_PROC_POWERPC                 = 0x20;
        static constexpr uint8_t PCI_SUBCLASS_PROC_ALPHA                   = 0x10;
        static constexpr uint8_t PCI_SUBCLASS_PROC_OTHER                   = 0x80;
        static constexpr uint8_t PCI_SUBCLASS_PROC_MIPS                    = 0x30;
        static constexpr uint8_t PCI_SUBCLASS_PROC_486                     = 0x01;
        static constexpr uint8_t PCI_SUBCLASS_PROC_386                     = 0x00;

        // Serial Bus Controller (0x0C)
        static constexpr uint8_t PCI_CLASS_SERIAL_BUS                      = 0x0C;
        static constexpr uint8_t PCI_SUBCLASS_INFINIBAND_SERIAL            = 0x06;
        static constexpr uint8_t PCI_SUBCLASS_SERIAL_BUS_OTHER             = 0x80;
        static constexpr uint8_t PCI_SUBCLASS_FIBRE_CHANNEL                = 0x04;
        static constexpr uint8_t PCI_SUBCLASS_ACCESS_BUS                   = 0x01;
        static constexpr uint8_t PCI_SUBCLASS_FIREWIRE                     = 0x00;
        static constexpr uint8_t PCI_SUBCLASS_CANBUS                       = 0x09;
        static constexpr uint8_t PCI_SUBCLASS_SERCOS                       = 0x08;
        static constexpr uint8_t PCI_SUBCLASS_SMBUS                        = 0x05;
        static constexpr uint8_t PCI_SUBCLASS_IPMI                         = 0x07;
        static constexpr uint8_t PCI_SUBCLASS_USB                          = 0x03;
        static constexpr uint8_t PCI_SUBCLASS_SSA                          = 0x02;

        // Wireless Controller (0x0D)
        static constexpr uint8_t PCI_CLASS_WIRELESS                        = 0x0D;
        static constexpr uint8_t PCI_SUBCLASS_ETHERNET_802_1A              = 0x20;
        static constexpr uint8_t PCI_SUBCLASS_ETHERNET_802_1B              = 0x21;
        static constexpr uint8_t PCI_SUBCLASS_WIRELESS_OTHER               = 0x80;
        static constexpr uint8_t PCI_SUBCLASS_CONSUMER_IR                  = 0x01;
        static constexpr uint8_t PCI_SUBCLASS_BLUETOOTH                    = 0x11;
        static constexpr uint8_t PCI_SUBCLASS_BROADBAND                    = 0x12;
        static constexpr uint8_t PCI_SUBCLASS_IRDA                         = 0x00;
        static constexpr uint8_t PCI_SUBCLASS_RF                           = 0x10;

        // Intelligent Controller (0x0E)
        static constexpr uint8_t PCI_CLASS_INTELLIGENT                     = 0x0E;
        static constexpr uint8_t PCI_SUBCLASS_I20                          = 0x00;

        // Satellite Communication Controller (0x0F)
        static constexpr uint8_t PCI_CLASS_SATELLITE                       = 0x0F;
        static constexpr uint8_t PCI_SUBCLASS_SATELLITE_AUDIO              = 0x02;
        static constexpr uint8_t PCI_SUBCLASS_SATELLITE_VOICE              = 0x03;
        static constexpr uint8_t PCI_SUBCLASS_SATELLITE_DATA               = 0x04;
        static constexpr uint8_t PCI_SUBCLASS_SATELLITE_TV                 = 0x01;

        // Encryption Controller (0x10)
        static constexpr uint8_t PCI_CLASS_ENCRYPTION                      = 0x10;
        static constexpr uint8_t PCI_SUBCLASS_ENTERTAINMENT_ENCRYPTION     = 0x10;
        static constexpr uint8_t PCI_SUBCLASS_NETWORK_ENCRYPTION           = 0x00;
        static constexpr uint8_t PCI_SUBCLASS_ENCRYPTION_OTHER             = 0x80;

        // Signal Processing Controller (0x11)
        static constexpr uint8_t PCI_CLASS_SIGNAL_PROCESSING               = 0x11;
        static constexpr uint8_t PCI_SUBCLASS_SIGNAL_PROCESSING_OTHER      = 0x80;
        static constexpr uint8_t PCI_SUBCLASS_PERF_COUNTERS                = 0x01;
        static constexpr uint8_t PCI_SUBCLASS_SIGNAL_MGMT                  = 0x20;
        static constexpr uint8_t PCI_SUBCLASS_COMM_SYNC                    = 0x10;
        static constexpr uint8_t PCI_SUBCLASS_DPIO                         = 0x00;

        // Miscellaneous
        static constexpr uint8_t PCI_CLASS_NON_ESSENTIAL_INSTRUMENTATION   = 0x13;
        static constexpr uint8_t PCI_CLASS_PROCESSING_ACCELERATOR          = 0x12;
        static constexpr uint8_t PCI_CLASS_VENDOR_SPECIFIC                 = 0xFF;
        static constexpr uint8_t PCI_CLASS_COPROCESSOR                     = 0x40;

        // Misc
        static constexpr uint16_t PCI_VENDOR_ID_UNPOPULATED = 0xFFFF;
        static constexpr uint16_t PCI_CONFIG_ADDRESS = 0xCF8;
        static constexpr uint16_t PCI_CONFIG_DATA    = 0xCFC;
    
        PCI::PCIDeviceHeader GetDeviceHeader(uint8_t bus, uint8_t slot, uint8_t function);
        PCI::PCI_BAR ReadBAR(const PCI::PCIDeviceHeader& device, uint8_t barIndex);
        uint32_t ReadConfig(const PCI::PCIDeviceHeader& device, uint8_t offset);
        uint8_t FindCapability(const PCI::PCIDeviceHeader& device, uint8_t capabilityID);
        bool EnableMSIX(const PCI::PCIDeviceHeader& device, uint64_t messageAddr, uint32_t messageData, uint16_t entryIndex);
        bool EnableMSI(const PCI::PCIDeviceHeader& device, uint64_t messageAddr, uint32_t messageData);
        void WriteConfig(const PCI::PCIDeviceHeader& device, uint8_t offset, uint32_t value);
        void FindDevicesByClass(uint8_t classCode, uint8_t subclass, Utility::List<PCI::PCIDeviceHeader*>& results);
        void FindDevicesByID(uint16_t vendorID, uint16_t deviceID, Utility::List<PCI::PCIDeviceHeader*>& results);
        void Initialize();
        bool initialized = false;

        explicit PCI(Memory::Paging* paging);

    private:
        uint32_t ReadConfigDWord(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset);
        uint16_t ReadConfigWord(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset);
        uint16_t GetHeaderType(uint8_t bus, uint8_t slot, uint8_t function);
        uint16_t GetClassCode(uint8_t bus, uint8_t slot, uint8_t function);
        uint16_t GetVendorID(uint8_t bus, uint8_t slot, uint8_t function);
        uint16_t GetDeviceID(uint8_t bus, uint8_t slot, uint8_t function);
        void WriteConfigDWord(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset, uint32_t value);
        void CheckFunction(uint8_t bus, uint8_t slot, uint8_t function);
        void CheckSlot(uint8_t bus, uint8_t slot);
        void CheckBus(uint8_t bus);
        Utility::List<PCI::PCIDeviceHeader> _PCIDevices;
        Memory::Paging* _paging = nullptr;
    };
}

#endif //BOREALOS_PCI_H