#include <IO/PCI.h>

namespace IO {
    PCI::PCIDeviceHeader PCI::GetDeviceHeader(uint8_t bus, uint8_t slot, uint8_t function) {
        PCIDeviceHeader header;

        header.bus      = bus;
        header.slot     = slot;
        header.function = function;
        header.vendorID = GetVendorID(bus, slot, function);
        header.deviceID = GetDeviceID(bus, slot, function);
        header.command  = ReadConfigWord(bus, slot, function, PCI_HEADER_FLAG_COMMAND);
        header.status   = ReadConfigWord(bus, slot, function, PCI_HEADER_FLAG_STATUS);

        // Revision ID and Prog IF share a word at 0x08
        uint16_t revisionWord = ReadConfigWord(bus, slot, function, PCI_HEADER_FLAG_REVISION);
        header.revisionID     = revisionWord & 0xFF;
        header.progIF         = (revisionWord >> 8) & 0xFF;

        // Class and subclass share a word at 0x0A
        uint16_t classWord = GetClassCode(bus, slot, function);
        header.subclass    = classWord & 0xFF;
        header.classCode   = (classWord >> 8) & 0xFF;

        // Cache line size and latency timer share a word at 0x0C
        uint16_t cacheWord   = ReadConfigWord(bus, slot, function, PCI_HEADER_FLAG_CACHE_WORD);
        header.cacheLineSize = cacheWord & 0xFF;
        header.latencyTimer  = (cacheWord >> 8) & 0xFF;

        // Header type and BIST share a word at 0x0E
        uint16_t headerWord = GetHeaderType(bus, slot, function);
        header.headerType   = headerWord & 0xFF;
        header.bist         = (headerWord >> 8) & 0xFF;

        return header;
    }

    // Returns the selected property of the 32-bit configuration word from device <bus>:<slot>:<func>
    uint16_t PCI::ReadConfigWord(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset) {
        uint32_t configAddress;

        // Convert to 32-bit unsigned integers
        uint32_t func32 = (uint32_t)function;
        uint32_t slot32 = (uint32_t)slot;
        uint32_t bus32 = (uint32_t)bus;
    
        // Create a configuration address based on Configuration Space Access Mechanism #1
        configAddress = (uint32_t)((bus32 << 16) | (slot32 << 11) |
                (func32 << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    
        // Write the config address to the PCI bridge
        Serial::outl(PCI::PCI_CONFIG_ADDRESS, configAddress);

        // Read in the data
        // NOTE: "(offset & 2) * 8) = 0" chooses the first word of the 32-bit register
        return (uint16_t)((Serial::inl(PCI::PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
    }

    uint16_t PCI::GetHeaderType(uint8_t bus, uint8_t slot, uint8_t function) {
        return ReadConfigWord(bus, slot, function, PCI_HEADER_FLAG_HEADER_TYPE);
    }
    
    uint16_t PCI::GetClassCode(uint8_t bus, uint8_t slot, uint8_t function) {
        return ReadConfigWord(bus, slot, function, PCI_HEADER_FLAG_CLASS_CODE);
    }

    uint16_t PCI::GetVendorID(uint8_t bus, uint8_t slot, uint8_t function) {
        return ReadConfigWord(bus, slot, function, PCI_HEADER_FLAG_VENDOR_ID);
    }

    uint16_t PCI::GetDeviceID(uint8_t bus, uint8_t slot, uint8_t function) {
        return ReadConfigWord(bus, slot, function, PCI_HEADER_FLAG_DEVICE_ID);
    }

    void PCI::CheckFunction(uint8_t bus, uint8_t slot, uint8_t function) {
        PCI::PCIDeviceHeader header = GetDeviceHeader(bus, slot, function);

        // Store the device
        //devices[deviceCount++] = header;

        LOG_DEBUG("PCI %u8:%u8.%u8:\n\r  * Vendor ID: 0x%x8\n\r  * Device ID: 0x%x8\n\r  * Class: 0x%x8\n\r  * Subclass: 0x%x8\n\r",
            header.bus, header.slot, header.function,
            header.vendorID, header.deviceID, header.classCode, header.subclass);

        // If this is a PCI-to-PCI bridge, scan the secondary bus it exposes
        if (header.classCode == PCI_CLASS_BRIDGE && header.subclass == PCI_SUBCLASS_PCI_PCI_BRIDGE) {
            // The secondary bus is at offset 0x18, upper byte of the word
            uint8_t secondaryBus = (ReadConfigWord(bus, slot, function, PCI_OFFSET_SECONDARY_BUS) >> 8) & 0xFF;
            CheckBus(secondaryBus);
        }
    }

    void PCI::CheckSlot(uint8_t bus, uint8_t slot) {
        if (GetVendorID(bus, slot, 0) == PCI_VENDOR_ID_UNPOPULATED)
            return;

        CheckFunction(bus, slot, 0);

        uint8_t headerType = GetHeaderType(bus, slot, 0) & 0xFF;
        if (headerType & PCI_HEADER_FLAG_MULTIFUNCTION) {
            // This is a multifunction slot, functions 1-7 may contain additional devices
            // NOTE: We already checked function 0 above
            for (uint8_t function = 1; function < 8; function++) {
                if (GetVendorID(bus, slot, function) != PCI_VENDOR_ID_UNPOPULATED)
                    CheckFunction(bus, slot, function);
            }
        }
    }

    void PCI::CheckBus(uint8_t bus) {
        // A PCI bus can have up to 32 slots
        for (uint8_t slot = 0; slot < 32; slot++)
            CheckSlot(bus, slot);
    }

    void PCI::Initialize() {
        // We start by enumerating the PCI bus via the recursive bus enumeration method
        // NOTE: This is more efficient than blindly scanning every single bus/slot combo, as we only check populated buses
        uint8_t headerType = GetHeaderType(0, 0, 0) & 0xFF;

        if (!(headerType & PCI_HEADER_FLAG_MULTIFUNCTION)) {
            // This is a single PCI host controller, only bus 0 exists at the root
            CheckBus(0);
        }
        else {
            // This is a multifunction host bridge, each function is a separate bus
            // NOTE: There can be up to 8 functions
            for (uint8_t function = 0; function < 8; function++) {
                // Functions are contiguous, so we have to break here instead of continuing
                if (GetVendorID(0, 0, function) == PCI_VENDOR_ID_UNPOPULATED)
                    break;

                CheckBus(function);
            }
        }

        while(true);
    }
}